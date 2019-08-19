#include <Arduino.h>
#include <ArduinoOTA.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <SparkFun_AS3935.h>

#define PIN_SI  D1
#define PIN_CS  D8

#define MQTT_HOST   "mosquitto.space.revspace.nl"
#define MQTT_PORT   1883

static char esp_id[16];

static WiFiManager wifiManager;
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static SparkFun_AS3935 lightning;

void setup(void)
{
    pinMode(PIN_SI, OUTPUT);
    digitalWrite(PIN_SI, 0);    // 0 = SPI, 1 = I2C

    // initialize serial port
    Serial.begin(115200);
    Serial.print("\nBLIKSEM\n");

    // init AS3935
    SPI.begin();
    if (lightning.beginSPI(PIN_CS, 1000000)) {
        Serial.println("Bliksem init OK!");
    } else {
        Serial.println("Bliksem init FAIL!");
        while(1);
    }
    Serial.printf("* indoor/outdoor:      0x%X\n", lightning.readIndoorOutdoor());
    Serial.printf("* watchdog threshold:  %d\n", lightning.readWatchdogThreshold());
    Serial.printf("* noise level:         %d\n", lightning.readNoiseLevel());
    Serial.printf("* spike rejection:     %d\n", lightning.readSpikeRejection());
    Serial.printf("* lightning threshold: %d\n", lightning.readLightningThreshold());
    Serial.printf("* interrupt reg:       0x%02X\n", lightning.readInterruptReg());
    Serial.printf("* mask disturber:      %d\n", lightning.readMaskDisturber());
    Serial.printf("* div ratio:           %d\n", lightning.readDivRatio());
    Serial.printf("* distance to storm:   %d\n", lightning.distanceToStorm());
    Serial.printf("* lightning energy:    %d\n", lightning.lightningEnergy());

    // get ESP id
    sprintf(esp_id, "%06X", ESP.getChipId());
    Serial.printf("ESP ID: %s\n", esp_id);

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
