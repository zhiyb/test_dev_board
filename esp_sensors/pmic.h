#pragma once

void pmic_init(void);
void pmic_update(void);
void pmic_update(uint32_t schedule_secs);
void pmic_shutdown(void);
uint8_t pmic_get_key(void);
void pmic_power_enable(bool sensors);
