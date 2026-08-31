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
 */


/**
 * @brief Initialize SPI2 Slave interrupt handling.
 *
 * This function enables the Cortex-M7 NVIC interrupt for SPI2.
 *
 * The SPI2 peripheral itself is initialized by CubeMX-generated
 * MX_SPI2_Init().
 */
void SPI2_Slave_Init(void);


/**
 * @brief Start one SPI packet transmission to the CM5 SPI Master.
 *
 * @param data
 *        Pointer to the packet data buffer.
 *
 * @param length
 *        Number of bytes to transmit.
 *
 * @return
 *        HAL_OK   : SPI transmission was successfully started.
 *        HAL_BUSY : SPI peripheral is already transmitting.
 *        HAL_ERROR: Invalid parameter or SPI start error.
 *
 * @note
 *        This function is NON-BLOCKING.
 *
 *        It starts the SPI transmission and returns immediately.
 *
 *        The actual transmission completion is handled by the
 *        HAL SPI transmit-complete callback.
 *
 *        The caller must NOT release or modify the packet buffer
 *        until the transmission has completed.
 */
HAL_StatusTypeDef SPI2_Slave_SendPacket(const uint8_t *data,
                                        uint16_t length);

/**
  * @return
 *        1 : SPI2 transmission is active.
 *        0 : SPI2 transmission is idle.
 */
uint8_t SPI2_Slave_IsBusy(void);
          

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