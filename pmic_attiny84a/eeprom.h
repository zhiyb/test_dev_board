#pragma once

#include <stdint.h>
#include <avr/eeprom.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t boot_mode;
    uint16_t aux_periodic_ticks;
    uint16_t aux_timeout_ticks;
    uint16_t esp_periodic_ticks;
    uint16_t esp_timeout_ticks;
    uint16_t sht_periodic_ticks;
    uint16_t sht_timeout_ticks;
} eeprom_data_t;
#pragma pack(pop)

extern eeprom_data_t eeprom_cache;

void eeprom_init(void);
void eeprom_update(void);
