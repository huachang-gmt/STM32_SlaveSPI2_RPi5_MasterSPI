#ifndef HM_SPI_H
#define HM_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"


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
 * @brief Submit one EtherCAT data packet for SPI transmission.
 *
 * This is the only API that the EtherCAT application needs to call.
 *
 * @param data
 *        Pointer to EtherCAT application data.
 *
 * @param length
 *        Number of EtherCAT application data bytes.
 *
 * @return
 *        HAL_OK    : data accepted.
 *        HAL_BUSY  : Ring Buffer is full or SPI is temporarily busy.
 *        HAL_ERROR : invalid parameter.
 *
 * @note
 *        This function is NON-BLOCKING.
 *
 *        It does not wait for SPI transmission to complete.
 *
 *        The HM module internally handles:
 *            - Ring Buffer
 *            - sequence number
 *            - SPI transmission
 *            - DMA completion
 */
HAL_StatusTypeDef HM_SendPacket(const uint8_t *data,
                                uint16_t length);

 
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