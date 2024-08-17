#pragma once

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t adc_vbg;
} eeprom_data_t;
#pragma pack(pop)

static const eeprom_data_t * const eeprom_data = (const eeprom_data_t *)0;
