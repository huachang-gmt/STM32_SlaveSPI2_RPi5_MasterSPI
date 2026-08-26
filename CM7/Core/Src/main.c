/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "spi2_slave.h"
#include "hm_spi.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* DUAL_CORE_BOOT_SYNC_SEQUENCE: Define for dual core boot synchronization    */
/*                             demonstration code based on hardware semaphore */
/* This define is present in both CM7/CM4 projects                            */
/* To comment when developping/debugging on a single core                     */
#define DUAL_CORE_BOOT_SYNC_SEQUENCE

#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;

SPI_HandleTypeDef hspi2;

/* USER CODE BEGIN PV */


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */
/* USER CODE BEGIN Boot_Mode_Sequence_0 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
  int32_t timeout;
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_0 */

/* USER CODE BEGIN Boot_Mode_Sequence_1 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
  /* Wait until CPU2 boots and enters in stop mode or timeout*/
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
  Error_Handler();
  }
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_1 */
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();
/* USER CODE BEGIN Boot_Mode_Sequence_2 */
#if defined(DUAL_CORE_BOOT_SYNC_SEQUENCE)
/* When system initialization is finished, Cortex-M7 will release Cortex-M4 by means of
HSEM notification */
/*HW semaphore Clock enable*/
__HAL_RCC_HSEM_CLK_ENABLE();
/*Take HSEM */
HAL_HSEM_FastTake(HSEM_ID_0);
/*Release HSEM in order to notify the CPU2(CM4)*/
HAL_HSEM_Release(HSEM_ID_0,0);
/* wait until CPU2 wakes up from stop mode */
timeout = 0xFFFF;
while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
if ( timeout < 0 )
{
Error_Handler();
}
#endif /* DUAL_CORE_BOOT_SYNC_SEQUENCE */
/* USER CODE END Boot_Mode_Sequence_2 */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */

  /*
   * ============================================================
   * HM SPI Ring Buffer 初始化
   * ============================================================
   *
   * 正式整合時：
   *     HM_Init() 由本模組初始化流程執行。
   *
   * 目前尚未接入正式 EtherCAT application，
   * 因此由 main.c 進行初始化。
   * ============================================================
   */
  if (HM_Init() != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_YELLOW);
  BSP_LED_Init(LED_RED);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  HAL_Delay(8000U);// 延後啟動，與 CM5 取得同步

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {


    static uint32_t last_test_tick = 0U;
    uint32_t now = HAL_GetTick();

    /*
     * ================================================================
     * STM32 HM Ring Buffer Unit Test
     * ================================================================
     *
     * 目前尚未接入正式 EtherCAT application。
     *
     * 因此以下資料與測試流程全部屬於「模擬測試」。
     *
     * 測試週期：
     *
     *     每 1 秒執行一次
     *
     * TEST 1:
     *     寫入 4 個 Ring Buffer
     *
     * TEST 2:
     *     讀出 4 個 Ring Buffer
     *
     * TEST 3:
     *     寫滿 4 個 Ring Buffer 後，
     *     再寫入第 5 筆，驗證 overflow protection。
     *
     * 最後：
     *     清除測試資料，確保下一輪測試從 pending=0 開始。
     *
     * 注意：
     *
     *     這裡不呼叫 HM_Process()。
     *
     *     因為 HM_Process() 目前會進入 blocking SPI
     *     傳輸，而這一階段只驗證 Ring Buffer。
     * ================================================================
     */

    //if ((now - last_test_tick) >= 1U)
    if ((now - last_test_tick) >= 1000U)
    {
        last_test_tick = now;



        /*
         * ====================================================================
         * Test 1
         *
         * Write 4 packets.
         *
         * ====================================================================
         */

        printf("\r\n");
        printf("========================================\r\n");
        printf("HM Ring Buffer Test\r\n");
        printf("========================================\r\n");

        /*
         * ============================================================
         * TEST 1
         *
         * 模擬同事的 EtherCAT application。
         *
         * 同事未來會在自己的 1 ms timer / interrupt 流程中取得
         * EtherCAT data，然後呼叫：
         *
         *     HM_WritePacket(ethercat_buffer, HM_DATA_SIZE);
         *
         * 目前尚未接入同事程式，
         * 所以這裡使用 ethercat_buffer 模擬。
         *
         * 每一筆資料使用不同的 pattern，
         * 方便後面的 TEST 2 驗證 FIFO 順序。
         * ============================================================
         */

        printf("[TEST 1] Write 4 packets\r\n");


        for (uint8_t packet_index = 0U;
             packet_index < HM_RING_BUFFER_COUNT;
             ++packet_index)
        {


        /*
         * ============================================================
         * 模擬 EtherCAT Producer
         * ============================================================
         *
         * 目前尚未接入正式 EtherCAT application。
         *
         * 本區域模擬同事未來每 1 ms 從 EtherCAT
         * 取得最新資料後，將資料交給 HM 模組。
         *
         * 注意：
         *
         *     ethercat_buffer 是「模擬同事自己的資料 buffer」。
         *
         *     正式整合時，這個 buffer 將由同事的
         *     EtherCAT application 提供。
         *
         *     HM 模組完全不知道這個模擬 buffer 的存在。
         * ============================================================
         */

        static uint8_t ethercat_buffer[HM_DATA_SIZE];


        /*
         * ------------------------------------------------------------
         * 模擬 EtherCAT 資料
         * ------------------------------------------------------------
         *
         * 目前建立：
         *
         *     00, 01, 02, ... , 201
         *
         * 共 202 bytes。
         *
         * 這只是為了模擬同事已經取得一筆
         * EtherCAT application data。
         *
         * 正式整合時，本區域會被同事的 EtherCAT
         * data acquisition code 取代。
         */
        for (uint16_t i = 0U; i < HM_DATA_SIZE; ++i)
        {
            //ethercat_buffer[i] = (uint8_t)i;// 這是施韋捷的 EtherCAT 資料 per 1ms 
            ethercat_buffer[i] =
                    (uint8_t)(packet_index + i);
        }


        HAL_StatusTypeDef status =
                HM_WritePacket(ethercat_buffer,
                               sizeof(ethercat_buffer));


            printf("  Write packet %u : %s\r\n",
                   packet_index,
                   (status == HAL_OK) ? "OK" : "FAIL");


            printf("    write=%lu read=%lu pending=%lu\r\n",
                   (unsigned long)HM_DebugGetWritePointer(),
                   (unsigned long)HM_DebugGetReadPointer(),
                   (unsigned long)HM_GetPendingCount());
        }

        /*
         * ====================================================================
         * Test 2
         *
         * Read 4 packets.
         *
         * ====================================================================
         *
         * 這裡真正呼叫 HM_Process()。
         *
         * 目前 HM_Process() 仍然使用 blocking SPI。
         *
         * 因此本測試不適合直接接 CM5。
         *
         * 為了純粹測試 Ring Buffer FIFO，
         * 下一步我們需要提供一個「Debug Read」API，
         * 不經過 SPI 就能取出 Ring Buffer packet。
         *
         * 因此目前先不要執行 HM_Process()。
         */

        printf("\r\n");
        printf("[TEST 2] Read 4 packets\r\n");

        /*
        * ====================================================================
        * 模擬 Ring Buffer Consumer
        * ====================================================================
        *
        * 目前尚未接入正式 SPI transmission。
        *
        * 因此這裡不呼叫 HM_Process()。
        *
        * 改由 HM_DebugReadPacket()：
        *
        *     直接讀取 Ring Buffer
        *     ↓
        *     驗證 FIFO 順序
        *     ↓
        *     驗證 sequence number
        *     ↓
        *     驗證 EtherCAT 模擬資料
        *
        * 注意：
        *
        *     HM_DebugReadPacket() 只供目前單元測試使用。
        *
        * 正式整合時：
        *
        *     HM_Process()
        *     ↓
        *     SPI transmission
        *
        * 才是正式資料消費路徑。
        * ====================================================================
        */

        {
            uint8_t read_packet[HM_PACKET_SIZE];

            for (uint8_t packet_index = 0U;
                packet_index < HM_RING_BUFFER_COUNT;
                ++packet_index)
            {
                HAL_StatusTypeDef status =
                    HM_DebugReadPacket(read_packet,
                                        sizeof(read_packet));


                if (status != HAL_OK)
                {
                    printf("  Read packet %u : FAIL\r\n",
                          packet_index);

                    continue;
                }


                /*
                * ------------------------------------------------------------
                * 讀取 HM 自動加入的 16-bit sequence number。
                *
                * Byte 0 = LSB
                * Byte 1 = MSB
                * ------------------------------------------------------------
                */

                uint16_t sequence =
                    (uint16_t)read_packet[0] |
                    ((uint16_t)read_packet[1] << 8U);


                /*
                * ------------------------------------------------------------
                * 驗證資料內容。
                *
                * 本次模擬資料：
                *
                *     packet 0:
                *         00, 01, 02, ...
                *
                *     packet 1:
                *         01, 02, 03, ...
                *
                *     packet 2:
                *         02, 03, 04, ...
                *
                *     packet 3:
                *         03, 04, 05, ...
                *
                * 因此 Byte 2 的值應該等於 packet_index。
                * ------------------------------------------------------------
                */

                uint8_t expected_first_data =
                    packet_index;


                uint8_t data_ok =
                    (read_packet[HM_SEQUENCE_SIZE] ==
                    expected_first_data);


                printf("  Read packet %u : %s\r\n",
                      packet_index,
                      data_ok ? "OK" : "DATA ERROR");


                printf("    sequence=%u first_data=%u "
                      "read=%lu pending=%lu\r\n",
                      sequence,
                      read_packet[HM_SEQUENCE_SIZE],
                      (unsigned long)HM_DebugGetReadPointer(),
                      (unsigned long)HM_GetPendingCount());
            }
        }


        /*
         * ============================================================
         * TEST 3
         *
         * Overflow protection test。
         *
         * 重新寫入 4 筆資料，使 Ring Buffer 完全滿載。
         *
         * 然後嘗試寫入第 5 筆。
         *
         * 正確結果：
         *
         *     HAL_BUSY
         *     pending = 4
         *     overflow counter +1
         *
         * 注意：
         *
         *     第 5 筆絕對不能覆蓋前面尚未傳送的資料。
         * ============================================================
         */

        printf("\r\n");
        printf("[TEST 3] Overflow test\r\n");


        /*
         * ------------------------------------------------------------
         * 先重新填滿 4 個 Ring Buffer。
         *
         * 這裡同樣使用模擬 EtherCAT data。
         * ------------------------------------------------------------
         */
        for (uint8_t packet_index = 0U;
             packet_index < HM_RING_BUFFER_COUNT;
             ++packet_index)
        {
            uint8_t ethercat_buffer[HM_DATA_SIZE];


            for (uint16_t i = 0U;
                 i < HM_DATA_SIZE;
                 ++i)
            {
                ethercat_buffer[i] =
                    (uint8_t)(0xA0U + packet_index + i);
            }


            HM_WritePacket(ethercat_buffer,
                           sizeof(ethercat_buffer));
        }


        printf("  Buffer filled: pending=%lu\r\n",
               (unsigned long)HM_GetPendingCount());



        /*
         * ------------------------------------------------------------
         * 模擬第五筆 EtherCAT data。
         *
         * 這一筆故意在 Ring Buffer 已滿時寫入。
         *
         * 這只是 overflow 測試資料，
         * 不是真正 EtherCAT data。
         * ------------------------------------------------------------
         */


        {
            uint8_t ethercat_buffer[HM_DATA_SIZE];

            /*
             * 模擬第五筆 EtherCAT data。
             *
             * 此時四個 Ring Buffer 都應該已經 occupied。
             */
            for (uint16_t i = 0U; i < HM_DATA_SIZE; ++i)
            {
                ethercat_buffer[i] = 0xEEU;
            }


            HAL_StatusTypeDef status =
                HM_WritePacket(ethercat_buffer,
                               sizeof(ethercat_buffer));


            printf("  Write packet #5 : %s\r\n",
                   (status == HAL_BUSY) ? "REJECTED (EXPECTED)"
                                        : "UNEXPECTED");


            printf("  pending=%lu\r\n",
                   (unsigned long)HM_GetPendingCount());


            printf("  overflow=%lu\r\n",
                   (unsigned long)HM_GetOverflowCount());
        }

        /*
         * ============================================================
         * TEST CLEANUP
         *
         * 將 TEST 3 填入的 4 筆測試資料讀出。
         *
         * 目的：
         *
         *     讓下一個 1 秒測試週期重新從：
         *
         *         pending = 0
         *
         *     開始。
         *
         * 這是單元測試專用的清理流程。
         * 正式 EtherCAT application 不需要這段程式。
         * ============================================================
         */

        {
            uint8_t cleanup_packet[HM_PACKET_SIZE];


            for (uint8_t i = 0U;
                 i < HM_RING_BUFFER_COUNT;
                 ++i)
            {
                HM_DebugReadPacket(cleanup_packet,
                                    sizeof(cleanup_packet));
            }


            printf("  Cleanup: pending=%lu\r\n",
                   (unsigned long)HM_GetPendingCount());
        }


        printf("========================================\r\n");
    


        /*
         * ------------------------------------------------------------
         * 將同事的 EtherCAT data 交給 HM Ring Buffer。
         * ------------------------------------------------------------
         *
         * 同事只需要知道這一個 API。
         *
         * HM 內部負責：
         *
         *     - Ring Buffer
         *     - sequence number
         *     - write pointer
         *     - overflow
         *     - SPI transmission
         */

        /*
        HAL_StatusTypeDef write_status =
            HM_WritePacket(ethercat_buffer,
                           sizeof(ethercat_buffer));


        if (write_status != HAL_OK)
        {            
            BSP_LED_Toggle(LED_RED);
        }
        */

        /*
         * ============================================================
         * HM transmission processing
         * ============================================================
         *
         * 目前仍然使用既有 blocking SPI path。
         *
         * 下一階段才會改成 non-blocking。
         * ============================================================
         */

        /*
        HAL_StatusTypeDef tx_status = HM_Process();

        if ((tx_status != HAL_OK) &&
            (tx_status != HAL_BUSY))
        {
            BSP_LED_Toggle(LED_RED);
        }
        */
    }



    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 9;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOMEDIUM;
  RCC_OscInitStruct.PLL.PLLFRACN = 3072;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_SLAVE;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x0;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CM5_TRIG_GPIO_Port, CM5_TRIG_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : CM5_TRIG_Pin */
  GPIO_InitStruct.Pin = CM5_TRIG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(CM5_TRIG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_CS_Pin */
  GPIO_InitStruct.Pin = SPI2_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SPI2_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
