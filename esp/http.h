#pragma once

#include <Stream.h>
#include <cstdint>

const String &http_get(WiFiClient &client, const char *url, int *code);
