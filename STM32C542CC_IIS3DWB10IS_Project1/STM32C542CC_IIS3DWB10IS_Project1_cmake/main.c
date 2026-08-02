/**
  ******************************************************************************
  * file           : main.c
  * brief          : Main program body
  *                  Calls target system initialization then loop in main.
  ******************************************************************************
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype -----------------------------------------------*/

#include "mx_usart1.h"
#include <stdio.h>
#include <string.h>

#include "iis3dwb10is_reg.h"

int _write(int file, char *ptr, int len)
{
    hal_uart_handle_t *huart1 = mx_usart1_uart_gethandle();

    if (huart1 != NULL)
    {
        HAL_UART_Transmit(huart1, ptr, len, 1000);
    }

    return len;
}

/* Private macro -------------------------------------------------------------*/
#define    BOOT_TIME        20 //ms

/* Private variables ---------------------------------------------------------*/
static int32_t data_raw_acceleration[3];
static int16_t data_raw_temperature;
static float_t acceleration_mg[3];
static float_t temperature_degC;
static uint8_t whoamI;
static uint8_t tx_buffer[1000];
static iis3dwb10is_pin_int_route_t route = { 0 };

/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t platform_write(void* handle, uint8_t reg, const uint8_t* bufp,
    uint16_t len);
static int32_t platform_read(void* handle, uint8_t reg, uint8_t* bufp,
    uint16_t len);
static void tx_com(uint8_t* tx_buffer, uint16_t len);
static void platform_delay(uint32_t ms);
static void platform_init(void);

static uint64_t xl_event_num = 0, temp_event_num = 0;
static uint8_t xl_event = 0, temp_event = 0;
static stmdev_ctx_t dev_ctx;


/**
  * brief:  The application entry point.
  * retval: none but we specify int to comply with C99 standard
  */
int main(void)
{
  /** System Init: this code placed in targets folder initializes your system.
    * It calls the initialization (and sets the initial configuration) of the peripherals.
    * You can use STM32CubeMX to generate and call this code or not in this project.
    * It also contains the HAL initialization and the initial clock configuration.
    */
  if (mx_system_init() != SYSTEM_OK)
  {
    return (-1);
  }
  else
  {
    /*
      * You can start your application code here
      */

	  printf("HELLO\n");
	  HAL_GPIO_WritePin(CS_PORT, CS_PIN, HAL_GPIO_PIN_SET);

      iis3dwb10is_data_rate_t rate;
      iis3dwb10is_xl_data_cfg_t xl_cfg;

      /* Initialize mems driver interface */
      dev_ctx.write_reg = platform_write;
      dev_ctx.read_reg = platform_read;
      dev_ctx.mdelay = platform_delay;
      dev_ctx.handle = mx_spi1_gethandle();
      /* Init test platform */
//	  platform_init();
      /* Wait sensor boot time */
      platform_delay(BOOT_TIME);
      /* Check device ID */
      iis3dwb10is_device_id_get(&dev_ctx, &whoamI);
      printf("IIS3DWB10IS_ID=0x%x,id=0x%x\n", IIS3DWB10IS_ID, whoamI);
      if (whoamI != IIS3DWB10IS_ID)
          while (1);

      /* Restore default configuration */
      iis3dwb10is_sw_por(&dev_ctx);

      /* Enable Block Data Update */
      iis3dwb10is_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);

      iis3dwb10is_xl_data_config_get(&dev_ctx, &xl_cfg);
      xl_cfg.rounding = IIS3DWB10IS_WRAPAROUND_DISABLED;
      iis3dwb10is_xl_data_config_set(&dev_ctx, xl_cfg);

      /* Set full scale */
      iis3dwb10is_xl_full_scale_set(&dev_ctx, IIS3DWB10IS_50g);

      iis3dwb10is_pin_int1_route_get(&dev_ctx, &route);
      route.drdy_xl = 1;
      route.drdy_temp = 1;
      iis3dwb10is_pin_int1_route_set(&dev_ctx, route);

      /* Set Output Data Rate */
      rate.burst = IIS3DWB10IS_CONTINUOS_MODE;
      rate.odr = IIS3DWB10IS_ODR_10KHz;
      iis3dwb10is_xl_data_rate_set(&dev_ctx, rate);

      /* Wait until the first valid sample is available */
      platform_delay(5);
      printf("IIS3DWB10IS initialization successful.\r\n");

      while (1) {

          /*
           * Poll the status register on every loop.
           * The previous code only checked xl_event/temp_event, but never set
           * these flags or read the sensor data, so the print conditions could
           * never become true.
           */
          iis3dwb10is_data_ready_t drdy;
          memset(&drdy, 0x00, sizeof(drdy));
          iis3dwb10is_data_ready_get(&dev_ctx, &drdy);

          /* Read acceleration when a new sample is available */
          if (drdy.drdy_xl) {
              xl_event = 1;
              xl_event_num++;

              memset(data_raw_acceleration, 0x00,
                  3U * sizeof(data_raw_acceleration[0]));
              iis3dwb10is_acceleration_raw_get(&dev_ctx,
                  data_raw_acceleration);
          }

          /* Read temperature when a new sample is available */
          if (drdy.drdy_temp) {
              temp_event = 1;
              temp_event_num++;

              memset(&data_raw_temperature, 0x00,
                  sizeof(data_raw_temperature));
              iis3dwb10is_temperature_raw_get(&dev_ctx,
                  &data_raw_temperature);
          }

          /* 10 kHz ODR: print one acceleration sample every 100 reads */
          if (xl_event && ((xl_event_num % 100U) == 0U)) {
              xl_event = 0;

              acceleration_mg[0] =
                  iis3dwb10is_from_fs50g_to_mg(data_raw_acceleration[0]);
              acceleration_mg[1] =
                  iis3dwb10is_from_fs50g_to_mg(data_raw_acceleration[1]);
              acceleration_mg[2] =
                  iis3dwb10is_from_fs50g_to_mg(data_raw_acceleration[2]);

              printf("Acceleration [mg]:%4.2f\t%4.2f\t%4.2f\r\n",
                  acceleration_mg[0],
                  acceleration_mg[1],
                  acceleration_mg[2]);
          }

          /* Temperature refresh rate is 52 Hz: print about once per second */
          if (temp_event && ((temp_event_num % 52U) == 0U)) {
              temp_event = 0;

              temperature_degC =
                  iis3dwb10is_from_lsb_to_celsius(data_raw_temperature);

              printf("Temperature [degC]:%6.2f\r\n",
                  temperature_degC);
          }

      }
  }
} /* end main */



/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument used to select the SPI handler
 * @param  reg       register to write
 * @param  bufp      pointer to data to write
 * @param  len       number of consecutive registers to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg,
                              const uint8_t *bufp, uint16_t len)
{
  hal_status_t status;
  uint8_t tx[33] = {0};
  uint8_t rx[33] = {0};

  if ((handle == NULL) || (bufp == NULL) ||
      (len == 0U) || (len > 32U))
  {
    return -1;
  }

  /* SPI write command: RW bit = 0 */
  tx[0] = reg & 0x7FU;
  memcpy(&tx[1], bufp, len);

  HAL_GPIO_WritePin(CS_PORT, CS_PIN, HAL_GPIO_PIN_RESET);
  status = HAL_SPI_TransmitReceive((hal_spi_handle_t *)handle,
                                   tx,
                                   rx,
                                   (uint32_t)len + 1U,
                                   1000U);
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, HAL_GPIO_PIN_SET);

  return (status == HAL_OK) ? 0 : -1;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument used to select the SPI handler
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that stores the data read
 * @param  len       number of consecutive registers to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg,
                             uint8_t *bufp, uint16_t len)
{
  hal_status_t status;
  uint8_t tx[33] = {0};
  uint8_t rx[33] = {0};

  if ((handle == NULL) || (bufp == NULL) ||
      (len == 0U) || (len > 32U))
  {
    return -1;
  }

  /* SPI read command: RW bit = 1 */
  tx[0] = reg | 0x80U;

  HAL_GPIO_WritePin(CS_PORT, CS_PIN, HAL_GPIO_PIN_RESET);
  status = HAL_SPI_TransmitReceive((hal_spi_handle_t *)handle,
                                   tx,
                                   rx,
                                   (uint32_t)len + 1U,
                                   1000U);
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, HAL_GPIO_PIN_SET);

  if (status != HAL_OK)
  {
    return -1;
  }

  memcpy(bufp, &rx[1], len);
  return 0;
}

/*
 * @brief  Send data through USART1
 *
 * @param  tx_buffer pointer to transmit buffer
 * @param  len       number of bytes to transmit
 *
 */
static void tx_com(uint8_t *tx_buffer, uint16_t len)
{
  hal_uart_handle_t *huart1 = mx_usart1_uart_gethandle();

  if ((huart1 != NULL) && (tx_buffer != NULL) && (len > 0U))
  {
    HAL_UART_Transmit(huart1, tx_buffer, len, 1000U);
  }
}

/*
 * @brief  Platform specific delay
 *
 * @param  ms delay in milliseconds
 *
 */
static void platform_delay(uint32_t ms)
{
  HAL_Delay(ms);
}
