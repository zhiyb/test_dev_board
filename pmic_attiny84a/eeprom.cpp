#include <avr/eeprom.h>
#include "eeprom.h"

eeprom_data_t eeprom_cache;

static eeprom_data_t * const eeprom_data = (eeprom_data_t *)0;

void eeprom_init(void)
{
    // Copy eeprom data to cache
    uint8_t len = sizeof(eeprom_data_t);
    uint8_t *src = (uint8_t *)eeprom_data;
    uint8_t *dst = (uint8_t *)&eeprom_cache;
    while (len--)
        *dst++ = eeprom_read_byte(src++);
}

void eeprom_update(void)
{
    // Store cache to eeprom
    uint8_t len = sizeof(eeprom_data_t);
    uint8_t *dst = (uint8_t *)eeprom_data;
    uint8_t *src = (uint8_t *)&eeprom_cache;
    while (len--)
        eeprom_update_byte(dst++, *src++);
}
