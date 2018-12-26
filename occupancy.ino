#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "publicConfiguration.h"
#include "privateConfiguration.h"

bool isHumanPresentStatistics[PIR_STATUS_PROBE_FREQUENCY];
HTTPClient http;

void setup() 
{
  Serial.begin(115200);
  Serial.println();
  
  pinMode(ESP8266_LED, OUTPUT);
  pinMode(PIR_MODE, OUTPUT);
  pinMode(PIR_STATUS, INPUT);

  setupWiFi();
  setupPir();
}

void setupWiFi()
{
  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  
  if (WIFI_MODE == WIFI_MODE_NO_AUTH || WIFI_MODE == WIFI_MODE_WEB_AUTH)
  {
    WiFi.begin(WIFI_SSID);
  } else if (WIFI_MODE == WIFI_MODE_PASS_AUTH)
  {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void setupPir() {
  digitalWrite(PIR_MODE, PIR_MODE_REPEATABLE);
  
  memset(isHumanPresentStatistics, 0, sizeof(isHumanPresentStatistics));
}

unsigned short currentItteration = 0;
void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    setupWiFi();
  }
  
  bool isPresentNow = digitalRead(PIR_STATUS);
  isHumanPresentStatistics[currentItteration] = isPresentNow;
  Serial.print(isPresentNow ? "!" : ".");
  delay(1000 / PIR_STATUS_PROBE_FREQUENCY);
  currentItteration = ++currentItteration % PIR_STATUS_PROBE_FREQUENCY;
  if (currentItteration == 0) {
    reportToServer();
    memset(isHumanPresentStatistics, 0, sizeof(isHumanPresentStatistics));
  }

}

void reportToServer()
{
  bool _isHumanPresent = isHumanPresent();
  
  Serial.print("Is human present: ");
  Serial.print(_isHumanPresent ? "Yes" : "No");
  Serial.println();
  digitalWrite(ESP8266_LED, _isHumanPresent);

  Serial.println("Reporting to server");

  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"id\": " + String(API_ROOM_ID) + ", \"name\": \"" + String(API_ROOM_NAME) + "\", \"isOccupied\": " + (_isHumanPresent ? "true" : "false") + "}";
  Serial.println(payload);
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    String responseBody = http.getString();
    Serial.println(httpCode, DEC);
    Serial.println(responseBody);

  }
  http.end();
}

bool isHumanPresent()
{
  int presentCount = 0;
  for (int i = 0; i < PIR_STATUS_PROBE_FREQUENCY; i++) {
    if (isHumanPresentStatistics[i] == true) {
      ++presentCount;
    }
  }

  Serial.print("presentCount: ");
  Serial.print(presentCount, DEC);
  Serial.print(" out of : ");
  Serial.print(PIR_STATUS_PROBE_FREQUENCY, DEC);
  Serial.println();
  
  int percentage = (presentCount * 100) / PIR_STATUS_PROBE_FREQUENCY;
  Serial.print("Presence percentage: ");
  Serial.print(percentage, DEC);
  Serial.println();
  return percentage >= PRESENCE_STATISTICS_PERCENTAGE_FOR_TRUE;
}
