/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32C542CCT6 + IIS3DWB10IS, I2C1 polling example (HAL2).
  ******************************************************************************
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */

#include "main.h"
#include "mx_gpio_default.h"
#include "mx_i2c1.h"
#include "mx_usart1.h"
#include "iis3dwb10is_reg.h"
#include <stdio.h>
#include <string.h>

/* CubeMX: I2C1_SCL = PB6, I2C1_SDA = PB7, CS = PB0, SA0 = PA1.
 * PB0/CS: 推挽输出，初始化电平 High；PA1/SA0: 推挽输出，初始化电平 Low。
 * I2C1 使用 7 位寻址；SCL/SDA 配置开漏复用，并接外部上拉电阻。
 * 使用当前工程 mx_system_init() 生成的时钟、I2C1 和 USART1 配置。
 */
#ifndef IIS3DWB10IS_I2C_ADDR_LOW
#define IIS3DWB10IS_I2C_ADDR_LOW     0x1AU /* SDO/TA0 = 0 */
#endif
#ifndef IIS3DWB10IS_I2C_ADDR_HIGH
#define IIS3DWB10IS_I2C_ADDR_HIGH    0x1BU /* SDO/TA0 = 1 */
#endif
#define SENSOR_SA0_HIGH             0U  /* 0: PA1 Low/0x1A; 1: PA1 High/0x1B */
#if SENSOR_SA0_HIGH == 0U
#define SENSOR_I2C_ADDR_7BIT         IIS3DWB10IS_I2C_ADDR_LOW
#else
#define SENSOR_I2C_ADDR_7BIT         IIS3DWB10IS_I2C_ADDR_HIGH
#endif
/* HAL2 的 device_addr 参数仍要求将 7 位地址左移一位。 */
#define SENSOR_I2C_ADDR_HAL          ((uint32_t)SENSOR_I2C_ADDR_7BIT << 1U)
#define BOOT_TIME_MS                20U
#define I2C_TIMEOUT_MS              100U
#define RESET_TIMEOUT_MS            100U
#define POLL_INTERVAL_MS            10U
#define PRINT_INTERVAL_MS           100U
#define DATA_TIMEOUT_MS             1000U

static stmdev_ctx_t dev_ctx;
static hal_i2c_handle_t *sensor_i2c;
static hal_uart_handle_t *com_uart;
static volatile hal_status_t last_uart_status = HAL_OK;
static hal_status_t last_i2c_status = HAL_OK;
static int32_t data_raw_acceleration[3];
static int16_t data_raw_temperature;
static float_t acceleration_mg[3];
static float_t temperature_degC;
static uint8_t acceleration_valid;
static uint8_t temperature_valid;
static uint8_t acceleration_updated;
static uint32_t last_xl_tick;
static uint32_t last_temp_tick;

/* 除 main 外，仅定义以下五个辅助函数。 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void tx_com(uint8_t *tx_buffer, uint16_t len);
static void platform_delay(uint32_t ms);
static void platform_init(void);

int main(void)
{
  uint8_t tx_buffer[256];
  uint32_t last_print_tick;
  uint32_t last_warning_tick;
  int32_t ret;
  const char *stage;

  if (mx_system_init() != SYSTEM_OK)
  {
    return -1;
  }
  platform_init();
  if (com_uart == NULL)
  {
    return -1;
  }

  (void)snprintf((char *)tx_buffer, sizeof(tx_buffer),
                 "\r\nSTM32C542CCT6 + IIS3DWB10IS IIC polling\r\n"
                 "I2C address: 7-bit=0x%02X, HAL=0x%02lX\r\n",
                 (unsigned int)SENSOR_I2C_ADDR_7BIT,
                 (unsigned long)SENSOR_I2C_ADDR_HAL);
  tx_com(tx_buffer, (uint16_t)strlen((char *)tx_buffer));
  if (sensor_i2c == NULL)
  {
    (void)snprintf((char *)tx_buffer, sizeof(tx_buffer),
                   "ERROR: I2C1 handle is NULL. Check mx_system_init().\r\n");
    tx_com(tx_buffer, (uint16_t)strlen((char *)tx_buffer));
    return -1;
  }

  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.mdelay = platform_delay;
  dev_ctx.handle = sensor_i2c;

  /* 传感器初始化直接在 main 中完成；失败后间隔 1 秒重试。 */
  while (1)
  {
    uint8_t id = 0U;
    uint8_t ctrl3 = 0U;
    uint8_t config[4] = {0};
    uint32_t reset_start;
    iis3dwb10is_data_rate_t rate = {0};
    iis3dwb10is_xl_data_cfg_t xl_cfg = {0};

    do
    {
      stage = "WHO_AM_I";
      ret = iis3dwb10is_device_id_get(&dev_ctx, &id);
      if (ret != 0) { break; }
      (void)snprintf((char *)tx_buffer, sizeof(tx_buffer),
                     "WHO_AM_I: expected=0x%02X, read=0x%02X\r\n",
                     (unsigned int)IIS3DWB10IS_ID, (unsigned int)id);
      tx_com(tx_buffer, (uint16_t)strlen((char *)tx_buffer));
      if (id != IIS3DWB10IS_ID)
      {
        ret = -2;
        break;
      }

      stage = "software reset";
      ret = iis3dwb10is_sw_reset(&dev_ctx);
      if (ret != 0) { break; }
      reset_start = HAL_GetTick();
      do
      {
        ret = iis3dwb10is_read_reg(&dev_ctx, IIS3DWB10IS_CTRL3, &ctrl3, 1U);
        if ((ret != 0) || ((ctrl3 & 0x01U) == 0U)) { break; }
        if ((uint32_t)(HAL_GetTick() - reset_start) >= RESET_TIMEOUT_MS)
        {
          stage = "software reset timeout";
          ret = -3;
          break;
        }
        platform_delay(1U);
      } while (1);
      if (ret != 0) { break; }
      platform_delay(BOOT_TIME_MS);

      /* 自动地址递增，支持连续读取 XYZ 的 12 字节和温度的 2 字节。 */
      stage = "IF_INC";
      ret = iis3dwb10is_read_reg(&dev_ctx, IIS3DWB10IS_CTRL3, &ctrl3, 1U);
      if (ret != 0) { break; }
      ctrl3 |= 0x04U;
      ret = iis3dwb10is_write_reg(&dev_ctx, IIS3DWB10IS_CTRL3, &ctrl3, 1U);
      if (ret != 0) { break; }

      stage = "BDU";
      ret = iis3dwb10is_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
      if (ret != 0) { break; }
      stage = "full scale";
      ret = iis3dwb10is_xl_full_scale_set(&dev_ctx, IIS3DWB10IS_50g);
      if (ret != 0) { break; }

      /* 20 位有符号数据，每轴 4 字节；启用 XYZ，关闭 16 位 rounding。 */
      stage = "XYZ data format";
      xl_cfg.rounding = IIS3DWB10IS_WRAPAROUND_DISABLED;
      xl_cfg.x_axis_en = 1U;
      xl_cfg.y_axis_en = 1U;
      xl_cfg.z_axis_en = 1U;
      ret = iis3dwb10is_xl_data_config_set(&dev_ctx, xl_cfg);
      if (ret != 0) { break; }

      stage = "ODR";
      rate.burst = IIS3DWB10IS_CONTINUOS_MODE;
      rate.odr = IIS3DWB10IS_ODR_2KHz5;
      ret = iis3dwb10is_xl_data_rate_set(&dev_ctx, rate);
      if (ret != 0) { break; }
      platform_delay(BOOT_TIME_MS);

      stage = "configuration readback";
      ret = iis3dwb10is_read_reg(&dev_ctx, IIS3DWB10IS_CTRL1, config, 4U);
      if (ret != 0) { break; }
      (void)snprintf((char *)tx_buffer, sizeof(tx_buffer),
                     "CTRL1/2/3/4: %02X %02X %02X %02X\r\n",
                     (unsigned int)config[0], (unsigned int)config[1],
                     (unsigned int)config[2], (unsigned int)config[3]);
      tx_com(tx_buffer, (uint16_t)strlen((char *)tx_buffer));
      if ((config[0] != 0x02U) || ((config[1] & 0x60U) != 0U) ||
          ((config[2] & 0x44U) != 0x44U) || ((config[3] & 0xDCU) != 0x1CU))
      {
        ret = -4;
      }
    } while (0);

    if (ret == 0) { break; }
    (void)snprintf((char *)tx_buffer, sizeof(tx_buffer),
                   "ERROR: %s; ret=%ld, HAL status=%ld\r\n"
                   "Check CS=High, SA0, I2C pull-ups and resistor links; retry in 1 s.\r\n",
                   stage, (long)ret, (long)last_i2c_status);
    tx_com(tx_buffer, (uint16_t)strlen((char *)tx_buffer));
#if defined(USE_HAL_I2C_GET_LAST_ERRORS) && (USE_HAL_I2C_GET_LAST_ERRORS == 1)
    (void)snprintf((char *)tx_buffer, sizeof(tx_buffer),
                   "I2C error flags=0x%08lX\r\n",
                   (unsigned long)HAL_I2C_GetLastErrorCodes(sensor_i2c));
    tx_com(tx_buffer, (uint16_t)strlen((char *)tx_buffer));
#endif
    platform_delay(1000U);
  }

  (void)snprintf((char *)tx_buffer, sizeof(tx_buffer),
                 "Initialization OK: FS=+/-50 g, ODR=2.5 kHz, continuous mode.\r\n"
                 "Polling output registers every 10 ms; display every 100 ms.\r\n");
  tx_com(tx_buffer, (uint16_t)strlen((char *)tx_buffer));
  last_print_tick = HAL_GetTick();
  last_warning_tick = last_print_tick;
  last_xl_tick = last_print_tick;
  last_temp_tick = last_print_tick;

  while (1)
  {
    iis3dwb10is_data_ready_t ready = {0};
    ret = iis3dwb10is_data_ready_get(&dev_ctx, &ready);
    if ((ret == 0) && (ready.drdy_xl != 0U))
    {
      ret = iis3dwb10is_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
      if (ret == 0)
      {
        for (uint32_t axis = 0U; axis < 3U; axis++)
        {
          /* 确认 20 位数据已扩展符号到 32 位，再执行单位换算。 */
          if ((data_raw_acceleration[axis] < -524288) ||
              (data_raw_acceleration[axis] > 524287))
          {
            ret = -5;
            break;
          }
        }
      }
      if (ret == 0)
      {
        for (uint32_t axis = 0U; axis < 3U; axis++)
        {
          acceleration_mg[axis] =
            iis3dwb10is_from_fs50g_to_mg(data_raw_acceleration[axis]);
        }
        acceleration_valid = 1U;
        acceleration_updated = 1U;
        last_xl_tick = HAL_GetTick();
      }
    }

    if ((ret == 0) && (ready.drdy_temp != 0U))
    {
      ret = iis3dwb10is_temperature_raw_get(&dev_ctx, &data_raw_temperature);
      if (ret == 0)
      {
        /* IIS3DWB10IS 的温度换算为 raw / 200。 */
        temperature_degC = iis3dwb10is_from_lsb_to_celsius(data_raw_temperature);
        temperature_valid = 1U;
        last_temp_tick = HAL_GetTick();
      }
    }

    if ((ret != 0) &&
        ((uint32_t)(HAL_GetTick() - last_warning_tick) >= DATA_TIMEOUT_MS))
    {
      (void)snprintf((char *)tx_buffer, sizeof(tx_buffer),
                     "ERROR: read data; ret=%ld, HAL status=%ld\r\n",
                     (long)ret, (long)last_i2c_status);
      tx_com(tx_buffer, (uint16_t)strlen((char *)tx_buffer));
#if defined(USE_HAL_I2C_GET_LAST_ERRORS) && (USE_HAL_I2C_GET_LAST_ERRORS == 1)
      (void)snprintf((char *)tx_buffer, sizeof(tx_buffer),
                     "I2C error flags=0x%08lX\r\n",
                     (unsigned long)HAL_I2C_GetLastErrorCodes(sensor_i2c));
      tx_com(tx_buffer, (uint16_t)strlen((char *)tx_buffer));
#endif
      last_warning_tick = HAL_GetTick();
    }
    if ((uint32_t)(HAL_GetTick() - last_xl_tick) >= DATA_TIMEOUT_MS)
    {
      acceleration_valid = 0U;
      acceleration_updated = 0U;
      if ((uint32_t)(HAL_GetTick() - last_warning_tick) >= DATA_TIMEOUT_MS)
      {
        (void)snprintf((char *)tx_buffer, sizeof(tx_buffer),
                       "WARNING: no fresh acceleration data for 1 s.\r\n");
        tx_com(tx_buffer, (uint16_t)strlen((char *)tx_buffer));
        last_warning_tick = HAL_GetTick();
      }
    }

    if ((uint32_t)(HAL_GetTick() - last_print_tick) >= PRINT_INTERVAL_MS)
    {
      last_print_tick = HAL_GetTick();
      if ((ret == 0) && (acceleration_valid != 0U) &&
          (acceleration_updated != 0U))
      {
        char value_text[4][20] = {"", "", "", "--"};
        float_t values[4] = {acceleration_mg[0], acceleration_mg[1],
                             acceleration_mg[2], temperature_degC};
        uint32_t count = 3U;
        if ((temperature_valid != 0U) &&
            ((uint32_t)(HAL_GetTick() - last_temp_tick) < DATA_TIMEOUT_MS))
        {
          count = 4U;
        }
        /* 在 main 中格式化两位小数，不需要 printf 浮点链接选项。 */
        for (uint32_t i = 0U; i < count; i++)
        {
          float_t scaled = values[i] * 100.0f;
          int32_t value_x100 =
            (int32_t)(scaled + ((scaled < 0.0f) ? -0.5f : 0.5f));
          uint32_t magnitude = (value_x100 < 0) ?
                               (0U - (uint32_t)value_x100) : (uint32_t)value_x100;
          (void)snprintf(value_text[i], sizeof(value_text[i]), "%s%lu.%02lu",
                         (value_x100 < 0) ? "-" : "",
                         (unsigned long)(magnitude / 100U),
                         (unsigned long)(magnitude % 100U));
        }
        (void)snprintf((char *)tx_buffer, sizeof(tx_buffer),
                       "Acceleration [mg]: X=%s, Y=%s, Z=%s; Temperature [C]: %s\r\n",
                       value_text[0], value_text[1], value_text[2], value_text[3]);
        tx_com(tx_buffer, (uint16_t)strlen((char *)tx_buffer));
        acceleration_updated = 0U;
      }
    }
    /* 基础轮询演示：读取近期样本，不能用作 2.5 kHz 全量无丢样采集。 */
    platform_delay(POLL_INTERVAL_MS);
  }
}

static int32_t platform_write(void *handle, uint8_t reg,
                              const uint8_t *bufp, uint16_t len)
{
  if ((handle == NULL) || (bufp == NULL) || (len == 0U))
  {
    last_i2c_status = HAL_ERROR;
    return -1;
  }
  /* IIC 不设置 SPI 读写位，也不切换 CS。 */
  last_i2c_status = HAL_I2C_MASTER_MemWrite((hal_i2c_handle_t *)handle,
                     SENSOR_I2C_ADDR_HAL, reg, HAL_I2C_MEM_ADDR_8BIT,
                     bufp, len, I2C_TIMEOUT_MS);
  return (last_i2c_status == HAL_OK) ? 0 : -1;
}

static int32_t platform_read(void *handle, uint8_t reg,
                             uint8_t *bufp, uint16_t len)
{
  if ((handle == NULL) || (bufp == NULL) || (len == 0U))
  {
    last_i2c_status = HAL_ERROR;
    return -1;
  }
  /* MemRead 内部完成寄存器地址写入及 repeated START 读取。 */
  last_i2c_status = HAL_I2C_MASTER_MemRead((hal_i2c_handle_t *)handle,
                     SENSOR_I2C_ADDR_HAL, reg, HAL_I2C_MEM_ADDR_8BIT,
                     bufp, len, I2C_TIMEOUT_MS);
  return (last_i2c_status == HAL_OK) ? 0 : -1;
}

static void tx_com(uint8_t *tx_buffer, uint16_t len)
{
  if (len == 0U) { return; }
  if ((com_uart == NULL) || (tx_buffer == NULL))
  {
    last_uart_status = HAL_ERROR;
    return;
  }
  /* 串口输出统一从此处发送；last_uart_status 可在调试器中查看。 */
  last_uart_status = HAL_UART_Transmit(com_uart, tx_buffer, len, 1000U);
}

static void platform_delay(uint32_t ms)
{
  HAL_Delay(ms);
}

static void platform_init(void)
{
  /* mx_system_init() 已初始化 GPIO/I2C1/USART1，再取得 HAL2 句柄。 */
  sensor_i2c = mx_i2c1_i2c_gethandle();
  com_uart = mx_usart1_uart_gethandle();
  /* 必须在首次访问传感器前设置 CS 和地址选择电平。 */
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, HAL_GPIO_PIN_SET);
#if SENSOR_SA0_HIGH == 0U
  HAL_GPIO_WritePin(SA0_PORT, SA0_PIN, HAL_GPIO_PIN_RESET);
#else
  HAL_GPIO_WritePin(SA0_PORT, SA0_PIN, HAL_GPIO_PIN_SET);
#endif
  platform_delay(BOOT_TIME_MS);
}
