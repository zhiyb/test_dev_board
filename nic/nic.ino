#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include "wifi_network.h"

static const unsigned int max_urllen  = 128;
static const unsigned int max_datalen = 8192;

static const unsigned int pin_led = 2;

static const unsigned int pin_req = 5;
static const unsigned int pin_ack = 4;

static const unsigned int pin_spi_cs = 15;

WiFiClientSecure client;

typedef enum {
  ReqUnknown = 0,
  ReqGet = 0x5a,
  ReqPost = 0xa5,
} req_type_t;

static inline void led(int en)
{
  digitalWrite(pin_led, en ? LOW : HIGH);
}

static inline uint8_t spi_transfer(uint8_t v)
{
  digitalWrite(pin_spi_cs, LOW);
  v = SPI.transfer(v);
  digitalWrite(pin_spi_cs, HIGH);
  return v;
}

void setup()
{
  pinMode(pin_req, INPUT);
  pinMode(pin_ack, OUTPUT);
  digitalWrite(pin_ack, HIGH);

  pinMode(pin_led, OUTPUT);
  led(0);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  SPI.begin();
  SPI.beginTransaction(SPISettings(10 * 1000 * 1000, MSBFIRST, SPI_MODE0));
  digitalWrite(pin_spi_cs, HIGH);
  pinMode(pin_spi_cs, OUTPUT);

  Serial.begin(115200);
  Serial.println();

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
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
  // Wait for data request
  if (digitalRead(pin_req) == LOW)
    return;

  // Receive header: type
  uint8_t type = spi_transfer('E');
  uint8_t post = false;
  if (type == ReqGet) {
  } else if (type == ReqPost) {
    post = true;
  } else {
    Serial.print("Unknown type: 0x");
    Serial.print(type, HEX);
    Serial.println();
    type = ReqUnknown;
  }

  // Receive header: urllen, datalen, resplen
  uint16_t urllen = 0;
  uint16_t datalen = 0;
  uint16_t resplen = 0;
  if (type != ReqUnknown) {
    urllen = spi_transfer('S');
    urllen |= spi_transfer('P') << 8;
    if (type == ReqPost) {
      datalen = spi_transfer(0xff);
      datalen |= spi_transfer(0xff) << 8;
    }
    resplen = spi_transfer(0xff);
    resplen |= spi_transfer(0xff) << 8;
    if (urllen >= max_urllen || datalen >= max_datalen) {
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
  if (type != ReqUnknown) {
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
    if (code == HTTP_CODE_OK) {
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
      //Serial.println(resp);
    }
    Serial.println();
  }

  led(0);

  // Data request complete, wait for data request deassert
  digitalWrite(pin_ack, LOW);
  while (digitalRead(pin_req) == HIGH);
  digitalWrite(pin_ack, HIGH);
}
