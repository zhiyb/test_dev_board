#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "http.h"

#define DEBUG_PRINT         1
#define DEBUG_PRINT_DATA    0

WiFiClientSecure client;
HTTPClient http;

void http_init(void)
{
    client.setInsecure();
}

const String &http_get(const char *url, int *code)
{
    int icode;
    code = code ?: &icode;

#if DEBUG_PRINT
    Serial.print("GET ");
    Serial.println(url);
#endif

    http.begin(client, url);
    *code = http.GET();
#if DEBUG_PRINT
    Serial.print("HTTP ");
    Serial.print(*code);
    Serial.print(" SIZE ");
    Serial.print(http.getSize());
#endif

    if (*code != HTTP_CODE_OK)
        return String();
    const String &resp = http.getString();
#if DEBUG_PRINT
    Serial.print(" READ ");
    Serial.println(resp.length());
#endif
#if DEBUG_PRINT_DATA
    Serial.print("DATA ");
    Serial.println(resp);
#endif
    return resp;
}

const String &http_post(const char *url, int *code, const void *data, uint32_t len)
{
    int icode;
    code = code ?: &icode;

#if DEBUG_PRINT
    Serial.print("POST ");
    Serial.println(url);
#endif

    http.begin(client, url);
    *code = http.POST((uint8_t *)data, len);
#if DEBUG_PRINT
    Serial.print("HTTP ");
    Serial.print(*code);
    Serial.print(" SIZE ");
    Serial.print(http.getSize());
#endif

    if (*code != HTTP_CODE_OK)
        return String();
    const String &resp = http.getString();
#if DEBUG_PRINT
    Serial.print(" READ ");
    Serial.println(resp.length());
#endif
#if DEBUG_PRINT_DATA
    Serial.print("DATA ");
    Serial.println(resp);
#endif
    return resp;
}
