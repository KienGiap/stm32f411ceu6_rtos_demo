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
#include "cmsis_os.h"
#include "usart.h"
#include "gpio.h"
#include "string.h"
#include "stdio.h"
void doTaskA(void *parameters);
void doTaskB(void *parameters);
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/**
  * @brief  The application entry point.
  * @retval int
  */

static SemaphoreHandle_t mutex_1;
static SemaphoreHandle_t mutex_2;
static QueueHandle_t uart_queue;

void send_string(const char *message) {
    const char *ptr = message;
    while (*ptr) {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = *ptr++;
    }
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = '\r';
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = '\n';
}

void send_to_queue(const char *message) {
    xQueueSend(uart_queue, &message, portMAX_DELAY);
}

void uart_task(void *parameters) {
    const char *msg;
    while (1) {
        if (xQueueReceive(uart_queue, &msg, portMAX_DELAY) == pdPASS) {
            send_string(msg);
        }
    }
}
int counter = 0;
void doTaskA(void *parameters) {

  // Loop forever
  while (1) {

    // // // Take mutex 1 (introduce wait to force deadlock)
    xSemaphoreTake(mutex_1, portMAX_DELAY);
    //  send_to_queue("Task A took mutex 1");
     vTaskDelay(1 / portTICK_PERIOD_MS);
    // xSemaphoreGive(mutex_1);
     send_to_queue("Task A doing somework  ");
     counter++;
     char buffer[50];
     snprintf(buffer, sizeof(buffer), "Counter value: %d", counter);
     send_to_queue(buffer);
    xSemaphoreGive(mutex_1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

  }
}

// Task B (low priority)
void doTaskB(void *parameters) {

  // Loop forever
  while (1) {
    xSemaphoreTake(mutex_1, portMAX_DELAY);
    //  send_to_queue("Task A took mutex 1");
    vTaskDelay(1 / portTICK_PERIOD_MS);
    // xSemaphoreGive(mutex_1);
    send_to_queue("Task B doing somework  ");
      counter++;
      char buffer[50];
      snprintf(buffer, sizeof(buffer), "Counter value: %d", counter);
      send_to_queue(buffer);
    xSemaphoreGive(mutex_1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

  }
}

// Task C (simple printing task)
void doTaskC(void *parameters) {

  // Loop forever
  while (1) {
    xSemaphoreTake(mutex_2, portMAX_DELAY);
    // send_to_queue("Task C is running");
    vTaskSuspend(NULL);
    send_to_queue("Task C is running");
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Print every 1 second
  }
}

int main(void)
{

  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();
  // =======================USER CODE BEGIN =======================
  mutex_1 = xSemaphoreCreateMutex();
  mutex_2 = xSemaphoreCreateMutex();
  uart_queue = xQueueCreate(10, sizeof(const char*));
  xTaskCreate(doTaskA, "TaskA", 128, NULL, 1, NULL);
  xTaskCreate(doTaskB, "TaskB", 128, NULL, 1, NULL);
  xTaskCreate(doTaskC, "TaskC", 128, NULL, 2, NULL);
  
  xTaskCreate(uart_task, "UART", 128, NULL, 2, NULL);
  send_to_queue("---FreeRTOS Deadlock Demo---");

  // =======================USER CODE END =======================
  /* Start scheduler */
  osKernelStart();
  
  for (;;) {}

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 200;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
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
