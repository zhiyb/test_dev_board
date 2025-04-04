#pragma once

#include <PubSubClient.h>

uint32_t mqtt_get(PubSubClient &mqttClient, const char *topic, void *data, uint32_t data_len);
