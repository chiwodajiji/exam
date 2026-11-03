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
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef hlpuart1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
extern UART_HandleTypeDef hlpuart1;

uint8_t rx_data;
char uart_rx_buf[20];
uint8_t rx_index = 0;

uint32_t total_time_ms = 60000;      // 預設 1 分鐘 (60000ms)
volatile uint8_t is_running = 0;     // 0 = 停止/暗, 1 = 運行中
volatile uint8_t is_paused = 0;      // 0 = 運行, 1 = 暫停
int32_t current_brightness = 0;      // 初始亮度設為 0 (暗)
uint32_t last_interrupt_time = 0;    // 按鈕防彈跳時間
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM2_Init(void);
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

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

    // 啟動 LPUART 接收中斷（只抓 1 個字元）
    HAL_UART_Receive_IT(&hlpuart1, &rx_data, 1);
    /* USER CODE END 2 */

    /* Infinite loop */
      /* USER CODE BEGIN WHILE */
      uint32_t step_interval = 10;          // 每 10ms 更新一次
      uint8_t current_led_phase = 0;        // 0: 第一顆燈滅, 1: 第二顆燈滅, 2: 第三顆燈滅
      int32_t led_brightness = 1000;        // 單顆燈的當前亮度

      uint8_t last_paused_state = 0;        // 用來追蹤上一次的暫停狀態

      while (1)
      {
          // 1. 隨時偵測暫停狀態是否有改變，若有改變則透過 UART 回傳
          if (is_paused != last_paused_state) {
              last_paused_state = is_paused;
              if (is_paused) {
                  char *pause_msg = "Paused\r\n";
                  HAL_UART_Transmit(&hlpuart1, (uint8_t*)pause_msg, strlen(pause_msg), HAL_MAX_DELAY);
              } else {
                  char *resume_msg = "Resumed\r\n";
                  HAL_UART_Transmit(&hlpuart1, (uint8_t*)resume_msg, strlen(resume_msg), HAL_MAX_DELAY);
              }
          }

          // 2. 收到 start 且未暫停時執行漸暗邏輯
          if (is_running && !is_paused) {
              uint32_t single_led_time = total_time_ms / 3;
              uint32_t total_steps = single_led_time / step_interval;
              if (total_steps == 0) total_steps = 1;

              float decrement = 1000.0f / (float)total_steps;

              // 依照目前的階段（Phase）控制對應的燈漸暗
              if (current_led_phase == 0) {
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1000);
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 1000);

                  if (led_brightness > 0) {
                      led_brightness -= decrement;
                      if (led_brightness < 0) led_brightness = 0;
                      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, (uint32_t)led_brightness);
                  } else {
                      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
                      current_led_phase = 1;
                      led_brightness = 1000;
                  }
              }
              else if (current_led_phase == 1) {
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 1000);

                  if (led_brightness > 0) {
                      led_brightness -= decrement;
                      if (led_brightness < 0) led_brightness = 0;
                      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, (uint32_t)led_brightness);
                  } else {
                      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
                      current_led_phase = 2;
                      led_brightness = 1000;
                  }
              }
              else if (current_led_phase == 2) {
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);

                  if (led_brightness > 0) {
                      led_brightness -= decrement;
                      if (led_brightness < 0) led_brightness = 0;
                      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, (uint32_t)led_brightness);
                  } else {
                      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
                      is_running = 0;
                      current_led_phase = 0;

                      char *done_msg = "All LEDs Fade Complete. All OFF.\r\n";
                      HAL_UART_Transmit(&hlpuart1, (uint8_t*)done_msg, strlen(done_msg), HAL_MAX_DELAY);
                  }
              }

              HAL_Delay(step_interval);
          }
          else {
              // 如果沒有在運行，強制保持三顆燈全暗 (0)
              if (!is_running) {
                  current_led_phase = 0;
                  led_brightness = 1000;
                  last_paused_state = 0; // 重置暫停追蹤
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
                  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, 0);
              }
              HAL_Delay(50);
          }
        /* USER CODE END WHILE */
      }
      /* USER CODE END 3 */
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 169;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == LPUART1) {
        if (rx_data == '\n' || rx_data == '\r') {
            uart_rx_buf[rx_index] = '\0'; // 字串結尾

            // 1. 檢查是否為設定時間指令 "set <分鐘>"
            if (strncmp((char *)uart_rx_buf, "set ", 4) == 0) {
                unsigned int minutes = 0;
                if (sscanf((char *)uart_rx_buf, "set %u", &minutes) == 1) {
                    total_time_ms = (uint32_t)minutes * 60000; // 分鐘轉毫秒
                    char resp_msg[80];
                    sprintf(resp_msg, "Time Set: %u minutes (%lu ms)\r\n", minutes, total_time_ms);
                    HAL_UART_Transmit(&hlpuart1, (uint8_t*)resp_msg, strlen(resp_msg), HAL_MAX_DELAY);
                }
            }
            // 2. 檢查是否為開始指令 "start"
            else if (strcmp((char *)uart_rx_buf, "start") == 0) {
                is_running = 1;              // 啟動運行
                is_paused = 0;               // 解除暫停
                current_brightness = 1000;   // 開始時瞬間切到最亮 (1000)

                char *resp_msg = "Started! LED fading down...\r\n";
                HAL_UART_Transmit(&hlpuart1, (uint8_t*)resp_msg, strlen(resp_msg), HAL_MAX_DELAY);
            }
            else {
                char *err_msg = "Unknown command! Use 'set <min>' or 'start'\r\n";
                HAL_UART_Transmit(&hlpuart1, (uint8_t*)err_msg, strlen(err_msg), HAL_MAX_DELAY);
            }

            // 清空緩存
            rx_index = 0;
            memset((char*)uart_rx_buf, 0, sizeof(uart_rx_buf));
        } else {
            if (rx_index < sizeof(uart_rx_buf) - 1) {
                uart_rx_buf[rx_index++] = rx_data;
            }
        }

        HAL_UART_Receive_IT(&hlpuart1, &rx_data, 1);
    }
}

// PC9 按鈕消彈跳中斷
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_9) {
        uint32_t current_time = HAL_GetTick();
        if ((current_time - last_interrupt_time) > 200) {
            is_paused = !is_paused;
            last_interrupt_time = current_time;
        }
    }
}
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
