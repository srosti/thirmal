#include <Arduino.h>
#include "WiFi.h"
#include <PubSubClient.h>

// setup dallas dS18b20 tempeature probe
#include <OneWire.h>
#include <DallasTemperature.h>
const int oneWireBus = 4; // gpio port
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

// Needed for to get NTP time
#include <NTPClient.h>
#include <WiFiUdp.h>

// Watchdog timer
const int wdtTimeout = 3000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;
void IRAM_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

// RTC SRAM is preserved in deep-sleep
RTC_DATA_ATTR int prevTemp = 0;

// Duration of ESP32 to stay in deep-sleep
#define MICROSECOND 1000000
#define MINUTE 60
#define SLEEP_DURATION 10 * MINUTE *MICROSECOND

// WiFi credentials.
const char *WIFI_SSID = "Toby's Spy Camera";
 const char *WIFI_PASS = "******";

// mqtt.eclipse.org
const char *mqtt_server = "137.135.83.217";

/* create an instance of PubSubClient client */
WiFiClient espClient;
PubSubClient client(espClient);
#define EXTERNAL_TEMP_TOPIC "cabin/hottub/external-temp"
#define INTERNAL_TEMP_TOPIC "cabin/hottub/internal-temp"
#define TIME_TOPIC "cabin/hottub/time"
#define DEBUG_TOPIC "cabin/hottub/debug"

#ifdef __cplusplus
extern "C"
{
#endif
  uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif

void disconnect_wifi()
{
  Serial.print("WIFI status = ");
  Serial.println(WiFi.getMode());
  WiFi.disconnect();
  delay(1000);
  WiFi.mode(WIFI_OFF);
  delay(1000);
  Serial.print("WIFI status = ");
  Serial.println(WiFi.getMode());
}

bool connect_wifi()
{

  int retries = 5;

  while (WiFi.status() != WL_CONNECTED and retries > 0)
  {
    // Connect to WPA/WPA2 network:
    Serial.println("Connecting to wifi");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("Waiting for wifi connection: status =");
    Serial.println(WiFi.status());
    // Check to see if connecting failed.
    // This is due to incorrect credentials
    if (WiFi.status() == WL_CONNECT_FAILED)
    {
      Serial.println("Failed to connect to WIFI. Please verify credentials: ");
      Serial.println();
      Serial.print("SSID: ");
      Serial.println(WIFI_SSID);
      Serial.print("Password: ");
      Serial.println(WIFI_PASS);
      Serial.println();
      // reset wlan on esp32
      //      disconnect_wifi();
    }
    delay(1000);
    retries--;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Connected to wifi: status =");
    Serial.println(WiFi.status());
    return true;
  }
  else
    Serial.print("Could not connect to wifi");
  return false;
}

void mqttconnect(const char topic[])
{
  int retries = 5;

  /* Loop until reconnected */
  while (!client.connected() and retries > 0)
  {
    Serial.print("MQTT connecting ...");
    String clientId = "ESP32Client";
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("mqttconnnect() failed: status code=");
      Serial.print(client.state());
      client.disconnect();
    }
    retries--;
  }
}

void publish(const char topic[], const char msg[])
{

  /* if client was disconnected then try to reconnect again */
  if (!client.connected())
  {
    Serial.println("mqtt client was not connected - try again");
    mqttconnect(topic);
  }
  else
  {
    Serial.println("mqtt client client connected");
  }

  Serial.println("publishing msg to topic");
  if (client.publish(topic, msg, true))
  {
    Serial.println("publish temp message success");
  }

}

void get_internal_temp(char internal_temp[])
{
  snprintf(internal_temp, 20, "%d", temprature_sens_read());
}

void get_external_temp(char external_temp[])
{

  // get the current temperature reading
  sensors.requestTemperaturesByIndex(0);
  sensors.requestTemperatures();
  snprintf(external_temp, 20, "%.0f", sensors.getTempFByIndex(0));
}

void get_time(char time[])
{
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP);
  int hours, minutes;
  int retries = 5;
  char meridieum[3] = "AM";

  timeClient.begin();
  while (!timeClient.update() and retries > 0)
  {
    timeClient.forceUpdate();
    retries--;
  }
  // Daylight savings GMT offset for Mountain Time Zone (-7*3600)
  timeClient.setTimeOffset(-25200);

  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  if (hours > 12)
  {
    snprintf(meridieum, 2, "%s", "PM");
    hours = hours % 12;
    if (hours == 0) {
      hours = 1;
    }
  }
  snprintf(time, 8, "%d:%02d %s", hours, minutes, meridieum);
}

void setup()
{
  char msg[20];

  Serial.begin(115200);
// watchdog timer
//  timer = timerBegin(0, 80, true);                  //timer 0, div 80
//  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
//  timerAlarmWrite(timer, wdtTimeout * 10000, false); //set time in us
//  timerAlarmEnable(timer);                          //enable interrupt

  if (connect_wifi())
  {

    client.setServer(mqtt_server, 1883);

    get_internal_temp(msg);
    Serial.print("publishing internal temp: ");
    Serial.println(msg);
    publish(INTERNAL_TEMP_TOPIC, msg);

    get_external_temp(msg);
    Serial.print("publishing external temp: ");
    Serial.println(msg);
    publish(EXTERNAL_TEMP_TOPIC, msg);

    get_time(msg);
    Serial.print("publishing time:");
    Serial.println(msg);
    publish(TIME_TOPIC, msg);
  }

  delay(1000);

  Serial.println("Disconnecting from wifi");
  disconnect_wifi();
  delay(1000);

  Serial.println("Enabling ESP32 deep-sleep");
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION);

  Serial.println("Putting the ESP32 into deep-sleep");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {}