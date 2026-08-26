#ifndef HM_SPI_H
#define HM_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"


/*
 * ============================================================================
 * HM SPI Application Interface
 * ============================================================================
 *
 * 本模組負責：
 *
 *     1. 管理 4 個 Ring Buffer
 *     2. 管理 write/read pointer
 *     3. 管理 buffer 使用狀態
 *     4. 管理 SPI packet sequence number
 *     5. 將 EtherCAT application data 放入 Ring Buffer
 *
 * 正式整合時，同事只需要呼叫：
 *
 *     HM_WritePacket(ethercat_buffer, length);
 *
 * 同事不需要知道：
 *
 *     - Ring Buffer 有幾個 buffer
 *     - write pointer
 *     - read pointer
 *     - sequence number
 *     - overflow
 *     - SPI 傳送細節
 *
 * ============================================================================
 */


/*
 * ============================================================================
 * Configuration
 * ============================================================================
 *
 * 一個完整 SPI packet 的總大小。
 *
 * 目前測試：
 *
 *     204 bytes
 *
 * 未來如果 EtherCAT packet 改成 800 bytes，
 * 只需要修改這個定義。
 */
#define HM_PACKET_SIZE        204U


/*
 * Packet 前兩個 bytes 保留給 HM 管理的 sequence number。
 */
#define HM_SEQUENCE_SIZE      2U


/*
 * Ring Buffer 數量。
 */
#define HM_RING_BUFFER_COUNT  4U


/*
 * EtherCAT application data 大小。
 *
 *     204 - 2 = 202 bytes
 */
#define HM_DATA_SIZE          (HM_PACKET_SIZE - HM_SEQUENCE_SIZE)


/**
 * @brief Initialize HM Ring Buffer.
 *
 * @return HAL_OK if initialization succeeds.
 */
HAL_StatusTypeDef HM_Init(void);


/**
 * @brief Write one EtherCAT packet into the HM Ring Buffer.
 *
 * @param data
 *        Pointer to the EtherCAT application data.
 *
 * @param length
 *        Number of EtherCAT application data bytes.
 *
 * @return
 *        HAL_OK    : data accepted.
 *        HAL_BUSY  : Ring Buffer is full.
 *        HAL_ERROR : invalid parameter.
 *
 * @note
 *        HM automatically adds the 16-bit sequence number
 *        to packet Byte 0 and Byte 1.
 */
HAL_StatusTypeDef HM_WritePacket(const uint8_t *data,
                                 uint16_t length);


/**
 * @brief Process one committed packet.
 *
 * Current implementation still uses the existing blocking
 * SPI2_Slave_SendPacket() path.
 *
 * Non-blocking transmission will be implemented later.
 *
 * @return
 *        HAL_OK      : packet transmitted successfully.
 *        HAL_BUSY    : no packet available.
 *        HAL_TIMEOUT : SPI timeout.
 *        HAL_ERROR   : SPI transmission failed.
 */
HAL_StatusTypeDef HM_Process(void);


/**
 * @brief Get current number of packets waiting for transmission.
 */
uint32_t HM_GetPendingCount(void);


/**
 * @brief Get Ring Buffer overflow count.
 */
uint32_t HM_GetOverflowCount(void);


/**
 * @brief Debug: Get current write pointer.
 *
 * 僅供 STM32 Ring Buffer 單元測試使用。
 * 正式 EtherCAT application 不需要使用。
 */
uint32_t HM_DebugGetWritePointer(void);


/**
 * @brief Debug: Get current read pointer.
 *
 * 僅供 STM32 Ring Buffer 單元測試使用。
 * 正式 EtherCAT application 不需要使用。
 */
uint32_t HM_DebugGetReadPointer(void);


/**
 * @brief Debug: Read one packet directly from Ring Buffer.
 *
 * 僅供 STM32 Ring Buffer 單元測試使用。
 *
 * 本 API 不執行 SPI 傳輸。
 * 只會將目前 read_pointer 所指向的完整 packet
 * 複製到 caller 提供的 buffer，並釋放該 Ring Buffer slot。
 *
 * @param data
 *        Caller 提供的接收 buffer。
 *
 * @param length
 *        接收 buffer 大小。
 *
 * @return
 *        HAL_OK   : 成功讀出一筆 packet。
 *        HAL_BUSY : Ring Buffer 目前沒有資料。
 *        HAL_ERROR: 參數錯誤。
 *
 * @note
 *        此 API 僅為目前 Ring Buffer 自我測試使用。
 *        正式 EtherCAT application 不需要使用。
 */
HAL_StatusTypeDef HM_DebugReadPacket(uint8_t *data,
                                     uint16_t length);
                                     

#ifdef __cplusplus
}
#endif

#endif /* HM_SPI_H */