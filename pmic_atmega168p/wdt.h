#pragma once

#include <stdint.h>

void wdt_init(void);
uint32_t wdt_tick(void);
uint32_t wdt_sec_to_ticks(uint32_t sec);
