#include "hm_spi.h"
#include "spi2_slave.h"
#include <string.h>

/*
 * ============================================================================
 * HM Ring Buffer
 * ============================================================================
 *
 * Ring Buffer:
 *
 *     Buffer 0
 *     Buffer 1
 *     Buffer 2
 *     Buffer 3
 *
 * Ring Buffer 的所有管理邏輯全部封裝在本檔案。
 *
 * EtherCAT application 不需要知道：
 *
 *     write_pointer
 *     read_pointer
 *     count
 *     sequence
 *     overflow
 *
 * ============================================================================
 */

/*
 * ============================================================================
 * Ring Buffer storage
 * ============================================================================
 *
 * 每個完整 packet：
 *
 *     Byte 0~1 :
 *         HM 自動產生的 sequence number
 *
 *     Byte 2~ :
 *         EtherCAT application data
 *
 * 注意：
 *
 *     本檔案不產生任何模擬 EtherCAT data。
 *
 *     所有 application data 都由 HM_WritePacket()
 *     的呼叫者提供。
 */
static uint8_t hm_ring_buffer
    [HM_RING_BUFFER_COUNT][HM_PACKET_SIZE];


/*
 * ============================================================================
 * Ring Buffer state
 * ============================================================================
 */

static uint32_t hm_write_pointer = 0U;
static uint32_t hm_read_pointer  = 0U;
static uint32_t hm_count         = 0U;


/*
 * HM sequence number。
 *
 * CM5 目前使用 Byte 0~1 驗證：
 *
 *     SEQ_ERR
 *     LOST
 *     DUP
 */
static uint16_t hm_sequence = 0U;


/*
 * Ring Buffer overflow counter。
 */
static uint32_t hm_overflow_count = 0U;

static HAL_StatusTypeDef HM_StartNextPacket(void);

/*
 * ============================================================================
 * HM_Init
 * ============================================================================
 */

HAL_StatusTypeDef HM_Init(void)
{
    hm_write_pointer = 0U;
    hm_read_pointer  = 0U;
    hm_count         = 0U;

    hm_sequence = 0U;

    hm_overflow_count = 0U;

    return HAL_OK;
}


/*
 * ============================================================================
 * HM_WritePacket
 * ============================================================================
 *
 * Public API provided to the EtherCAT application.
 *
 * 呼叫者只需要：
 *
 *     HM_WritePacket(ethercat_buffer, length);
 *
 * HM 內部負責：
 *
 *     1. 檢查 Ring Buffer 是否有空間
 *     2. 複製 EtherCAT data
 *     3. 加入 sequence number
 *     4. 更新 write pointer
 *     5. 更新 packet count
 *
 * 呼叫者完全不需要知道 Ring Buffer 的存在方式。
 */

static HAL_StatusTypeDef HM_WritePacket(const uint8_t *data,
                                 uint16_t length)
{
    uint8_t *packet;


    /*
     * ------------------------------------------------------------------------
     * Parameter validation
     * ------------------------------------------------------------------------
     */
    if ((data == NULL) ||
        (length != HM_DATA_SIZE))
    {
        return HAL_ERROR;
    }


    /*
     * ------------------------------------------------------------------------
     * Check Ring Buffer capacity
     * ------------------------------------------------------------------------
     *
     * 如果四個 buffer 都已經等待傳送，
     * 絕對不能覆蓋尚未送出的資料。
     */
    if (hm_count >= HM_RING_BUFFER_COUNT)
    {
        hm_overflow_count++;

        return HAL_BUSY;
    }


    /*
     * ------------------------------------------------------------------------
     * Current write buffer
     * ------------------------------------------------------------------------
     */
    packet = &hm_ring_buffer[hm_write_pointer][0];


    /*
     * ------------------------------------------------------------------------
     * Sequence number
     * ------------------------------------------------------------------------
     *
     * Byte 0 = LSB
     * Byte 1 = MSB
     *
     * 保持與目前 CM5 測試程式完全相同的格式。
     */
    packet[0] =
        (uint8_t)(hm_sequence & 0xFFU);

    packet[1] =
        (uint8_t)((hm_sequence >> 8U) & 0xFFU);


    /*
     * ------------------------------------------------------------------------
     * Copy EtherCAT application data
     * ------------------------------------------------------------------------
     *
     * EtherCAT application data 放在：
     *
     *     Byte 2 ~ Byte 203
     */
    memcpy(&packet[HM_SEQUENCE_SIZE],
           data,
           HM_DATA_SIZE);


    /*
     * 下一個 packet 使用下一個 sequence number。
     */
    hm_sequence++;


    /*
     * Packet ready。
     */
    hm_count++;

    /*
     * Move write pointer。
     */
    hm_write_pointer++;

    if (hm_write_pointer >= HM_RING_BUFFER_COUNT)
    {
        hm_write_pointer = 0U;
    }

    return HAL_OK;
}


static HAL_StatusTypeDef HM_StartNextPacket(void)
{
    if (hm_count == 0U)
    {
        return HAL_BUSY;
    }

    return SPI2_Slave_SendPacket(
        &hm_ring_buffer[hm_read_pointer][0],
        HM_PACKET_SIZE);
}


HAL_StatusTypeDef HM_SendPacket(const uint8_t *data,
                                uint16_t length)
{
    HAL_StatusTypeDef status;

    status = HM_WritePacket(data, length);

    if (status != HAL_OK)
    {
        return status;
    }

    if (SPI2_Slave_IsBusy() == 0U)
    {
        (void)HM_StartNextPacket();
    }

    return HAL_OK;
}


void HM_OnSpiTxComplete(void)
{
    if (hm_count == 0U)
    {
        return;
    }

    hm_read_pointer++;

    if (hm_read_pointer >= HM_RING_BUFFER_COUNT)
    {
        hm_read_pointer = 0U;
    }

    hm_count--;

    if (hm_count > 0U)
    {
        (void)HM_StartNextPacket();
    }
}


/*
 * ============================================================================
 * HM_GetPendingCount
 * ============================================================================
 */

uint32_t HM_GetPendingCount(void)
{
    return hm_count;
}


/*
 * ============================================================================
 * HM_GetOverflowCount
 * ============================================================================
 */

uint32_t HM_GetOverflowCount(void)
{
    return hm_overflow_count;
}



