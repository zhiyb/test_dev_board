#pragma once

#include <stdint.h>

#define MQTT_MAX_PACKET_SIZE    (5 * 1024)
#include <PubSubClient.h>

extern char mqtt_buf_topic[128];
extern char mqtt_buf_data[MQTT_MAX_PACKET_SIZE];

uint32_t mqtt_get(PubSubClient &mqttClient, const char *topic, void *data, uint32_t data_len, bool str_null = false);
