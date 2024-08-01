#include <ESP8266HTTPClient.h>
#include "http.h"

#define DEBUG_PRINT         0
#define DEBUG_PRINT_DATA    0

HTTPClient http;

const String &http_get(WiFiClient &client, const char *url, int *code)
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
    const String &data = http.getString();
#if DEBUG_PRINT
    Serial.print(" READ ");
    Serial.println(data.length());
#endif
#if DEBUG_PRINT_DATA
    Serial.print("DATA ");
    Serial.println(data);
#endif
    return data;
}
