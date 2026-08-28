#include "spi2_slave.h"
#include <stdio.h>

/* ============================================================================
 * External SPI2 handle
 * ============================================================================
 *
 * hspi2 is still initialized by CubeMX-generated main.c.
 *
 * We intentionally keep MX_SPI2_Init() in main.c so that CubeMX can continue
 * to regenerate the peripheral initialization safely.
 */

extern SPI_HandleTypeDef hspi2;

/* ============================================================================
 * SPI2 Slave Initialization
 * ============================================================================
 *
 * MX_SPI2_Init() is responsible for configuring the SPI2 peripheral.
 *
 * This function is responsible only for enabling the Cortex-M7
 * NVIC interrupt used by the SPI2 HAL interrupt-transfer mechanism.
 * ============================================================================ */

void SPI2_Slave_Init(void)
{
    /*
     * Configure SPI2 interrupt priority.
     *
     * Priority 5 is intentionally used for the current bring-up
     * stage. It provides normal interrupt service without giving
     * SPI2 an unnecessarily high priority over time-critical
     * system interrupts.
     */
    HAL_NVIC_SetPriority(SPI2_IRQn, 2U, 0U);

    /*
     * Enable SPI2 global interrupt in the Cortex-M7 NVIC.
     */
    HAL_NVIC_EnableIRQ(SPI2_IRQn);
}


/* ============================================================================
 * Configuration
 * ============================================================================ */

/*
 * STM32H755 PE3 is connected to CM5 GPIO25.
 *
 * PE3 HIGH:
 *     One complete packet is ready.
 *
 * PE3 LOW:
 *     SPI transaction has completed or failed.
 */

#define SPI2_SLAVE_NOTIFY_PORT    CM5_TRIG_GPIO_Port
#define SPI2_SLAVE_NOTIFY_PIN     CM5_TRIG_Pin

/*
 * SPI transmission currently active。
 *
 * 0 = idle
 * 1 = transmitting
 */
static volatile uint8_t spi2_slave_tx_busy = 0U;


/*
 * SPI transmission completed successfully.
 *
 * 0 = no completed transmission pending
 * 1 = current transmission completed
 *
 * This flag is set by HAL_SPI_TxCpltCallback().
 *
 * The application consumes this flag through:
 *
 *     SPI2_Slave_ConsumeTxComplete()
 */
static volatile uint8_t spi2_slave_tx_complete = 0U;


/* ============================================================================
 * Debug counters
 * 這些 counter 目前保留。
 *
 * 它們可以幫助我們確認 non-blocking SPI 是否正常運作。
 * ============================================================================ */

volatile uint32_t SPI2_SlaveNotifyCount = 0U;
volatile uint32_t SPI2_SlaveTxCount = 0U;
volatile uint32_t SPI2_SlaveErrorCount = 0U;

volatile uint32_t SPI2_SlaveTxTimeoutCount = 0U;
volatile uint32_t SPI2_SlaveTxBusyCount = 0U;
volatile uint32_t SPI2_SlaveTxHalErrorCount = 0U;

volatile uint32_t SPI2_SlaveLastHalError = 0U;
volatile uint32_t SPI2_SlaveDmaTxCpltCount = 0U;

/* ============================================================================
 * SPI2 Slave Send Packet
 * ============================================================================
 */

HAL_StatusTypeDef SPI2_Slave_SendPacket(const uint8_t *data,
                                        uint16_t length)
{
    HAL_StatusTypeDef status;


    /*
     * Basic parameter validation.
     */
    if ((data == NULL) || (length == 0U))
    {
        SPI2_SlaveErrorCount++;

        return HAL_ERROR;
    }

    /*
     * ------------------------------------------------------------------------
     * Check whether SPI2 is already transmitting.
     * ------------------------------------------------------------------------
     *
     * non-blocking 模式下：
     *
     *     不能在上一筆 packet 尚未完成時，
     *     又開始另一筆 SPI transmission。
     */
    if ((spi2_slave_tx_busy != 0U) ||
        (spi2_slave_tx_complete != 0U))
    {
        SPI2_SlaveTxBusyCount++;

        return HAL_BUSY;
    }


    /*
    * A new transmission is starting.
    *
    * Clear any previous completion notification and
    * mark SPI2 as busy before starting the HAL transfer.
    */
    spi2_slave_tx_complete = 0U;
    spi2_slave_tx_busy = 1U;


    /*
    * Start SPI transmission in interrupt mode first.
    *
    * IMPORTANT:
    *
    * The SPI peripheral and HAL interrupt-transfer state
    * must be ready before notifying CM5.
    */
    status =
        HAL_SPI_Transmit_DMA(&hspi2,
                            (uint8_t *)data,
                            length);


    /*
    * If SPI transmission could not be started,
    * do NOT notify CM5.
    */
    if (status != HAL_OK)
    {
        spi2_slave_tx_busy = 0U;
        spi2_slave_tx_complete = 0U;

        SPI2_SlaveErrorCount++;

        SPI2_SlaveLastHalError =
            HAL_SPI_GetError(&hspi2);

        if (status == HAL_BUSY)
        {
            SPI2_SlaveTxBusyCount++;
        }
        else if (status == HAL_TIMEOUT)
        {
            SPI2_SlaveTxTimeoutCount++;

            BSP_LED_Toggle(LED_YELLOW);
        }
        else if (status == HAL_ERROR)
        {
            SPI2_SlaveTxHalErrorCount++;

            BSP_LED_Toggle(LED_YELLOW);
        }

        return status;
    }


    /*
    * SPI2 transfer is now armed and ready.
    *
    * Only now notify CM5 that it may start generating SCK.
    */
    HAL_GPIO_WritePin(SPI2_SLAVE_NOTIFY_PORT,
                    SPI2_SLAVE_NOTIFY_PIN,
                    GPIO_PIN_SET);

    SPI2_SlaveNotifyCount++;


    return HAL_OK;

}


/* ============================================================================
 * HAL SPI TX Complete Callback
 * ============================================================================
 *
 * HAL 在 SPI2 interrupt transmission 完成後呼叫這個 callback。
 *
 * 這裡才代表：
 *
 *     整筆 packet 已經完成 SPI transmission。
 */

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    /*
     * 確認 callback 是 SPI2。
     */
    if (hspi != &hspi2)
    {
        return;
    }

    SPI2_SlaveDmaTxCpltCount++;

    /*
     * SPI transmission completed。
     */
    spi2_slave_tx_busy = 0U;

    /*
    * Record successful transmission completion.
    *
    * HM_Process() will consume this flag and release
    * the corresponding Ring Buffer slot.
    */
    spi2_slave_tx_complete = 1U;


    /*
     * Packet transmission successful。
     */
    SPI2_SlaveTxCount++;


    /*
     * Release CM5 notification。
     *
     * PE3 HIGH -> LOW
     *
     * 表示這一筆 SPI transaction 已經完成。
     */
    HAL_GPIO_WritePin(SPI2_SLAVE_NOTIFY_PORT,
                      SPI2_SLAVE_NOTIFY_PIN,
                      GPIO_PIN_RESET);

}


/* ============================================================================
 * HAL SPI Error Callback
 * ============================================================================
 *
 * SPI interrupt transmission 發生錯誤時，
 * HAL 會呼叫這個 callback。
 */

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    /*
     * 確認 callback 是 SPI2。
     */
    if (hspi != &hspi2)
    {
        return;
    }


    /*
     * Capture HAL SPI error code BEFORE changing state.
     */
    SPI2_SlaveLastHalError =
        HAL_SPI_GetError(&hspi2);


    /*
     * SPI transmission failed。
     */
    spi2_slave_tx_busy = 0U;
    spi2_slave_tx_complete = 0U;


    SPI2_SlaveErrorCount++;


    /*
     * Release CM5 notification。
     */
    HAL_GPIO_WritePin(SPI2_SLAVE_NOTIFY_PORT,
                      SPI2_SLAVE_NOTIFY_PIN,
                      GPIO_PIN_RESET);


    /*
     * Yellow LED indicates SPI error。
     */
    SPI2_SlaveTxHalErrorCount++;

    BSP_LED_Toggle(LED_YELLOW);
}


/* ============================================================================
 * SPI2 Slave Busy State
 * ============================================================================
 *
 * Return the current non-blocking SPI transmission state.
 *
 * This function does not wait.
 * ============================================================================ */

uint8_t SPI2_Slave_IsBusy(void)
{
    if ((spi2_slave_tx_busy != 0U) ||
        (spi2_slave_tx_complete != 0U))
    {
        return 1U;
    }

    return 0U;
}


/* ============================================================================
 * SPI2 Slave Transmission Complete
 * ============================================================================
 *
 * Return and consume the successful transmission-complete notification.
 *
 * This function is intentionally implemented as a "consume" operation:
 *
 *     1 = one completed transmission was waiting
 *
 * After returning 1, the completion flag is cleared.
 *
 *     0 = no completed transmission is waiting
 *
 * This prevents HM_Process() from processing the same completion
 * more than once.
 * ============================================================================ */

uint8_t SPI2_Slave_ConsumeTxComplete(void)
{
    if (spi2_slave_tx_complete == 0U)
    {
        return 0U;
    }


    spi2_slave_tx_complete = 0U;

    return 1U;
}
