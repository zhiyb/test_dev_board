#pragma once

#include <Stream.h>
#include <cstdint>

static const char url_base[] = "https://zhiyb.me/api/schd.php";
static const char sensor_url_base[] = "https://zhiyb.me/api/sensor.php";
extern char id[32];

extern WiFiClientSecure client;

void http_init(void);
const String &http_get(const char *url, int *code);
const String &http_post(const char *url, int *code, const void *data, uint32_t len);
