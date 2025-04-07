#pragma once

#include <NTPClient.h>
#include "mqtt.h"

void pmic_init(void);
// void pmic_update(void);
void pmic_update(NTPClient &ntpClient, PubSubClient &mqttClient, const char *id);
void pmic_shutdown(void);
uint8_t pmic_get_key(void);

void pmic_debug(const char *str);
