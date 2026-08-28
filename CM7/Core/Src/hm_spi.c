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

HAL_StatusTypeDef HM_WritePacket(const uint8_t *data,
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


/*
 * ============================================================================
 * HM_Process
 * ============================================================================
 *
 * Non-blocking SPI transmission。
 *
 * 重要：
 *
 *     SPI2_Slave_SendPacket() 回傳 HAL_OK
 *     只代表 SPI transmission 已經「開始」。
 *
 *     並不代表 packet 已經傳送完成。
 *
 * 因此：
 *
 *     HM_Process()
 *         ↓
 *     SPI2_Slave_SendPacket()
 *         ↓
 *     HAL_SPI_Transmit_IT()
 *         ↓
 *     return HAL_OK
 *
 * 此時 Ring Buffer slot 仍然不能釋放。
 *
 * 必須等 SPI transmission 完成後，
 * 下一次 HM_Process() 才能確認前一筆 packet 已完成，
 * 然後釋放 Ring Buffer slot。
 *
 * ============================================================================
 */

HAL_StatusTypeDef HM_Process(void)
{
    HAL_StatusTypeDef status;

    const uint8_t *packet;

    /*
     * ------------------------------------------------------------------------
     * First consume a completed SPI transmission.
     * ------------------------------------------------------------------------
     *
     * HAL_SPI_TxCpltCallback() only reports that the SPI transmission
     * has completed.
     *
     * The Ring Buffer slot is released here.
     */
    if (SPI2_Slave_ConsumeTxComplete() != 0U)
    {
        HM_OnSpiTxComplete();
    }

    /*
     * ------------------------------------------------------------------------
     * 沒有待傳送 packet。
     * ------------------------------------------------------------------------
     */
    if (hm_count == 0U)
    {
        return HAL_BUSY;
    }


    /*
     * ------------------------------------------------------------------------
     * SPI transmission 尚未完成。
     * ------------------------------------------------------------------------
     *
     * 上一筆 packet 仍然使用 read_pointer 所指向的 Ring Buffer slot。
     *
     * 絕對不能覆蓋這個 buffer。
     * ------------------------------------------------------------------------
     */
    if (SPI2_Slave_IsBusy() != 0U)
    {
        return HAL_BUSY;
    }


    /*
     * ------------------------------------------------------------------------
     * SPI 已經 idle。
     *
     * 如果上一筆 packet 已經傳送完成，
     * 現在可以釋放上一個 Ring Buffer slot。
     *
     * 注意：
     *
     * 目前先由下一次 HM_Process() 的呼叫，
     * 在確認 SPI idle 後完成 pointer/count 更新。
     * ------------------------------------------------------------------------
     */

     /*
     * ------------------------------------------------------------------------
     * Inspect the packet currently pointed to by read_pointer.
     * ------------------------------------------------------------------------
     */
    packet = &hm_ring_buffer[hm_read_pointer][0];

    
    /*
     * ------------------------------------------------------------------------
     * Start non-blocking SPI transmission.
     * ------------------------------------------------------------------------
     */
    status =
        SPI2_Slave_SendPacket(
            packet,
            HM_PACKET_SIZE);

    /*
     * ------------------------------------------------------------------------
     * SPI transmission successfully started。
     * ------------------------------------------------------------------------
     *
     * IMPORTANT：
     *
     * 這裡不能立即：
     *
     *     hm_read_pointer++;
     *     hm_count--;
     *
     * 因為 SPI transmission 此時還沒有完成。
     *
     * Ring Buffer slot 必須保持有效，
     * 直到 HAL SPI transmission complete。
     * ------------------------------------------------------------------------
     */
    if (status == HAL_OK)
    {
        return HAL_OK;
    }


    /*
     * SPI transmission 沒有成功開始。
     *
     * Ring Buffer packet 仍然保留，
     * 因此不能移動 read_pointer，
     * 也不能減少 hm_count。
     */
    return status;
}


/*
 * ============================================================================
 * HM_OnSpiTxComplete
 * ============================================================================
 *
 * Called when SPI2 transmission of the current Ring Buffer packet
 * has completed successfully.
 *
 * SPI transmission completion is the only point at which the
 * current Ring Buffer slot may be released.
 * ============================================================================
 */

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



