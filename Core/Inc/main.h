/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BTN_Pin GPIO_PIN_13
#define BTN_GPIO_Port GPIOC
#define BCD8_IN_Pin GPIO_PIN_0
#define BCD8_IN_GPIO_Port GPIOC
#define BCD4_IN_Pin GPIO_PIN_1
#define BCD4_IN_GPIO_Port GPIOC
#define BCD2_IN_Pin GPIO_PIN_2
#define BCD2_IN_GPIO_Port GPIOC
#define BCD1_IN_Pin GPIO_PIN_3
#define BCD1_IN_GPIO_Port GPIOC
#define FWD_Pin GPIO_PIN_0
#define FWD_GPIO_Port GPIOA
#define REF_Pin GPIO_PIN_1
#define REF_GPIO_Port GPIOA
#define IFWD_Pin GPIO_PIN_2
#define IFWD_GPIO_Port GPIOA
#define VOLTAGE_Pin GPIO_PIN_3
#define VOLTAGE_GPIO_Port GPIOA
#define CURRENT_Pin GPIO_PIN_4
#define CURRENT_GPIO_Port GPIOA
#define RESERVED_A_I_Pin GPIO_PIN_5
#define RESERVED_A_I_GPIO_Port GPIOA
#define PTT_IN_Pin GPIO_PIN_4
#define PTT_IN_GPIO_Port GPIOC
#define RSRV_I_D_Pin GPIO_PIN_5
#define RSRV_I_D_GPIO_Port GPIOC
#define DS2_Pin GPIO_PIN_12
#define DS2_GPIO_Port GPIOB
#define DS1_Pin GPIO_PIN_13
#define DS1_GPIO_Port GPIOB
#define HF_IN_Pin GPIO_PIN_6
#define HF_IN_GPIO_Port GPIOC
#define RSRV_O_D_2_Pin GPIO_PIN_7
#define RSRV_O_D_2_GPIO_Port GPIOC
#define RSRV_O_D_1_Pin GPIO_PIN_8
#define RSRV_O_D_1_GPIO_Port GPIOC
#define BCD1_Pin GPIO_PIN_9
#define BCD1_GPIO_Port GPIOA
#define BCD2_Pin GPIO_PIN_10
#define BCD2_GPIO_Port GPIOA
#define BCD4_Pin GPIO_PIN_11
#define BCD4_GPIO_Port GPIOA
#define BCD8_Pin GPIO_PIN_12
#define BCD8_GPIO_Port GPIOA
#define PWM_FAN_Pin GPIO_PIN_15
#define PWM_FAN_GPIO_Port GPIOA
#define PWM_PUMP_Pin GPIO_PIN_3
#define PWM_PUMP_GPIO_Port GPIOB
#define PTT_OUT_Pin GPIO_PIN_4
#define PTT_OUT_GPIO_Port GPIOB
#define PWR_CTL_Pin GPIO_PIN_5
#define PWR_CTL_GPIO_Port GPIOB
#define PTT_RELAY_Pin GPIO_PIN_6
#define PTT_RELAY_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
