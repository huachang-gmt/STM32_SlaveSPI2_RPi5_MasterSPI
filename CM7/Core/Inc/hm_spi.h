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
 * @brief Start transmission of one committed packet.
 *
 * @return
 *        HAL_OK   : SPI transmission was successfully started.
 *        HAL_BUSY : No packet available or SPI is still busy.
 *        HAL_ERROR: SPI transmission could not be started.
 *
 * @note
 *        This function is NON-BLOCKING.
 *
 *        HAL_OK means that the SPI transmission has been started,
 *        not that the packet has already been transmitted.
 *
 *        The Ring Buffer slot is released only after
 *        HM_OnSpiTxComplete() is called from the SPI
 *        transmit-complete callback.
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
 * @brief Notify HM that the current SPI packet has completed.
 *
 * This function releases the Ring Buffer slot that was being
 * transmitted and advances the read pointer.
 *
 * It must be called only after the SPI transmission has completed
 * successfully.
 */
void HM_OnSpiTxComplete(void);


#ifdef __cplusplus
}
#endif

#endif /* HM_SPI_H */