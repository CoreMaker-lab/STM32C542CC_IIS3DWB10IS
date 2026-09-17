/**
  ******************************************************************************
  * @file           : mx_i2c1.h
  * @brief          : Header for mx_i2c1.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the mx_stm32c5xx_hal_drivers_license.md file
  * in the same directory as the generated code.
  * If no mx_stm32c5xx_hal_drivers_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MX_I2C1_H
#define MX_I2C1_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for I2C in HAL layer */
/******************************************************************************/
/**
  * @brief mx_i2c1_i2c init function
  * This function configures the hardware resources used in this example
  * @retval pointer to handle or NULL in case of failure
  */
hal_i2c_handle_t *mx_i2c1_i2c_init(void);

/**
  * @brief  De-initialize i2c1 instance and return it.
  */
void mx_i2c1_i2c_deinit(void);

/**
  * @brief  Get the I2C1 object.
  * @retval Pointer on the I2C1 Handle
  */
hal_i2c_handle_t *mx_i2c1_i2c_gethandle(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_I2C1_H */
