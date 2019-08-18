#include <Arduino.h>
#include <ArduinoOTA.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <SparkFun_AS3935.h>

#define PIN_TICK    D4

#define MQTT_HOST   "mosquitto.space.revspace.nl"
#define MQTT_PORT   1883

static char esp_id[16];

static WiFiManager wifiManager;
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

void setup(void)
{
    // initialize serial port
    Serial.begin(115200);
    Serial.print("\nBLIKSEM\n");

    // get ESP id
    sprintf(esp_id, "%08X", ESP.getChipId());
    Serial.print("ESP ID: ");
    Serial.println(esp_id);

    // setup OTA
    ArduinoOTA.setHostname("esp-bliksem");
    ArduinoOTA.setPassword("bliksem");
    ArduinoOTA.begin();
    
    // connect to wifi
    Serial.println("Starting WIFI manager ...");
    wifiManager.setConfigPortalTimeout(120);
    wifiManager.autoConnect("ESP-BLIKSEM");
    
    // MQTT setup
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
}

void loop(void)
{
    // keep MQTT alive
    mqttClient.loop();

    ArduinoOTA.handle();
}


