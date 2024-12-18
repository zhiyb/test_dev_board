#pragma once

#include <stdint.h>
#include <avr/eeprom.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t esp_timeout_sec;
    uint16_t pico_timeout_sec;
    uint16_t adc_vbg;
    uint8_t boot_mode;
} eeprom_data_t;
#pragma pack(pop)

static eeprom_data_t * const eeprom_data = (eeprom_data_t *)0;
