
#include <ESP8266WiFi.h>
#include <OneWire.h>            // Libraries for DS18B20
#include <DallasTemperature.h>  // Libraries for DS18B20
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// WiFi Credentials
#include "arduino_secrets.h"

// Data wire is plugged into pin D2 on the Arduino.
#define ONE_WIRE_BUS D2

// Setup a oneWire instance to communicate with any OneWire devices.
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// SHA1 fingerprint of the server certificate
const uint8_t sha1fp[20] = CERT_FINGERPRINT;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);

  delay(150);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
    {
      delay(800);
      Serial.print(".");
    }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  delay(100);
}

String form_payload(float temp)
{
  String buf = METER_ID;
  unsigned long epochTime;

  buf = buf + " temperature=";;
  buf = buf + String(temp);

  return buf;
}

int post_result(float temp)
{
  // Get https from the cloud
  int httpCode;
  String payload;
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  String url = "https://database.example.com:8086/write?";
  client->setFingerprint(sha1fp);
  HTTPClient https;

  url = url + DB_NAME + DB_USER + DB_PASSWD;
  Serial.println(url);

  payload = form_payload(temp);
  Serial.println(payload);
  if (!https.begin(*client, url))
    {
      // Reguest failed
      Serial.printf("[HTTPS] Unable to connect\n");
      return -1;
    }
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  httpCode = https.POST(payload);
  if (httpCode < 0)
    {
      // HTTP error
      Serial.printf("[HTTPS] code: %d\n", httpCode);
      Serial.printf("[HTTPS] POST... failed, error: %s\n",
                    https.errorToString(httpCode).c_str());

      https.end();
      return httpCode;
    }
  Serial.printf("HTTP code: %d\n", httpCode);
  Serial.println(https.getString());
  https.end();
  return httpCode;
}

void loop()
{
  // put your main code here, to run repeatedly:
  float current_temp;
  sensors.requestTemperatures();

  current_temp = sensors.getTempCByIndex(0);
  if (current_temp < -110)
    {
      Serial.println("ERROR: Could not get reading from sensor");
    }
  else
    {
      Serial.printf("Temperature is: %f\n", current_temp);
      Serial.println();
      post_result(current_temp);
    }
  delay(INTERVAL * MINUTES);
}
