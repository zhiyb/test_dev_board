#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "epd.h"

#ifdef __cplusplus
extern "C" {
#endif

// HAL compatibility
void systick_delay(uint32_t ms);
void sleep_ms(uint32_t ms);
void spi_master_enable(bool val);
void gpio_set_rst(bool val);
void gpio_set_dc(bool val);
bool gpio_get_busy(void);
void spi_receive_en(bool en);
void spi_master_transmit(uint8_t val);
uint8_t spi_master_receive(void);

#ifdef __cplusplus
}
#endif
