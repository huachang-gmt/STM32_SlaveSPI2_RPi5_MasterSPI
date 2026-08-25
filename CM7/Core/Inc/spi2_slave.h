#ifndef SPI2_SLAVE_H
#define SPI2_SLAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/*
 * ============================================================================
 * SPI2 Slave Interface
 * ============================================================================
 *
 * STM32H755 SPI2 operates as SPI Slave.
 *
 * External SPI Master:
 *     Raspberry Pi CM5
 *
 * Notification:
 *     STM32 PE3 -> CM5 GPIO25
 *
 * SPI transaction:
 *     CM5 asserts CS
 *     CM5 generates SCK
 *     STM32 transmits packet data
 *
 * IMPORTANT:
 *     This is the ONLY application-level function that the EtherCAT
 *     producer should need to call.
 */

/**
 * @brief Transmit one complete packet to the SPI Master.
 *
 * @param data
 *        Pointer to the packet data buffer.
 *
 * @param length
 *        Number of bytes to transmit.
 *
 * @return
 *        HAL_OK      : SPI transaction completed successfully.
 *        HAL_TIMEOUT : SPI transaction timed out.
 *        HAL_BUSY    : SPI peripheral was busy.
 *        HAL_ERROR   : SPI transaction failed.
 *
 * @note
 *        Current implementation is blocking.
 *        This will be reviewed before integrating the 1 ms EtherCAT
 *        producer/timer path.
 */
HAL_StatusTypeDef SPI2_Slave_SendPacket(const uint8_t *data,
                                        uint16_t length);


/*
 * ============================================================================
 * Debug counters
 * ============================================================================
 *
 * These are mainly for bring-up/debugging.
 * They are NOT part of the normal application data path.
 */

extern volatile uint32_t SPI2_SlaveNotifyCount;
extern volatile uint32_t SPI2_SlaveTxCount;
extern volatile uint32_t SPI2_SlaveErrorCount;

extern volatile uint32_t SPI2_SlaveTxTimeoutCount;
extern volatile uint32_t SPI2_SlaveTxBusyCount;
extern volatile uint32_t SPI2_SlaveTxHalErrorCount;

extern volatile uint32_t SPI2_SlaveLastHalError;

#ifdef __cplusplus
}
#endif

#endif /* SPI2_SLAVE_H */