#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "wifi_network.h"
#include "epd.h"
#include "epd_test.h"

static const unsigned int max_urllen = 128;
static const unsigned int max_datalen = 8192;

WiFiClientSecure client;

void setup()
{
    Serial.begin(38400);
    Serial.println();

    init_epd();
    epd_func_2in13().init();
    epd_test_2in13();
    Serial.println("EPD init done");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    client.setInsecure();
}

void loop()
{
    return;

#if 0
    // Receive header: type
    uint8_t type = spi_transfer('E');
    uint8_t post = false;
    if (type == ReqGet)
    {
    }
    else if (type == ReqPost)
    {
        post = true;
    }
    else
    {
        Serial.print("Unknown type: 0x");
        Serial.print(type, HEX);
        Serial.println();
        type = ReqUnknown;
    }

    // Receive header: urllen, datalen, resplen
    uint16_t urllen = 0;
    uint16_t datalen = 0;
    uint16_t resplen = 0;
    if (type != ReqUnknown)
    {
        urllen = spi_transfer('S');
        urllen |= spi_transfer('P') << 8;
        if (type == ReqPost)
        {
            datalen = spi_transfer(0xff);
            datalen |= spi_transfer(0xff) << 8;
        }
        resplen = spi_transfer(0xff);
        resplen |= spi_transfer(0xff) << 8;
        if (urllen >= max_urllen || datalen >= max_datalen)
        {
            Serial.print("Invalid length: ");
            Serial.print(urllen);
            Serial.print(", ");
            Serial.print(datalen);
            Serial.println();
            type = ReqUnknown;
            urllen = 0;
            datalen = 0;
        }
    }

    // Receive data: url, data
    static char url[max_urllen + 1];
    static uint8_t data[max_datalen];
    for (unsigned int i = 0; i < urllen; i++)
        url[i] = spi_transfer(0xff);
    url[urllen] = 0;
    for (unsigned int i = 0; i < datalen; i++)
        data[i] = spi_transfer(0xff);

    led(1);

    // Do data request
    if (type != ReqUnknown)
    {
        system_update_cpu_freq(160);
        HTTPClient http;
        http.begin(client, url);

        int code = 0;
        if (type == ReqGet)
            code = http.GET();
        else
            code = http.POST(data, datalen);
        system_update_cpu_freq(80);

        // Return HTTP status code
        spi_transfer(code >> 0);
        spi_transfer(code >> 8);

        Serial.print("HTTP ");
        Serial.print(code);
        if (code == HTTP_CODE_OK)
        {
            String resp = http.getString();
            Serial.print(" LEN ");
            Serial.print(resp.length());
            if (resp.length() < resplen)
                resplen = resp.length();
            spi_transfer(resplen >> 0);
            spi_transfer(resplen >> 8);

            const char *pstr = resp.c_str();
            for (unsigned int i = 0; i < resplen; i++)
                spi_transfer(pstr[i]);
            // Serial.println(resp);
        }
        Serial.println();
    }

    led(0);

    // Data request complete, wait for data request deassert
    digitalWrite(pin_ack, LOW);
    while (digitalRead(pin_req) == HIGH)
        ;
    digitalWrite(pin_ack, HIGH);
#endif
}
