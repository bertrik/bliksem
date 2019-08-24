#include <Arduino.h>
#include <ArduinoOTA.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <SparkFun_AS3935.h>
#include <NTPClient.h>

#define PIN_SI  D1
#define PIN_IRQ D2
#define PIN_CS  D8

#define MQTT_HOST   "mosquitto.space.revspace.nl"
//#define MQTT_HOST   "stofradar.nl"
#define MQTT_PORT   1883
#define MQTT_TOPIC  "revspace/sensors/bliksem/%s/%s"

static char esp_id[16];

static WiFiManager wifiManager;
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);
static SparkFun_AS3935 lightning;
static WiFiUDP ntpUDP;
static NTPClient ntp(ntpUDP);

static char statustopic[128];
static char valuetopic[128];

static bool mqtt_alive(void)
{
    mqttClient.loop();

    // stay connected
    bool result = mqttClient.connected();
    if (!result) {
        Serial.printf("Connecting with status topic %s...", statustopic);
        result = mqttClient.connect(esp_id, statustopic, 0, true, "offline");
        if (result) {
            result = mqttClient.publish(statustopic, "online", true);
        }
        Serial.printf(result ? "OK\n" : "FAILED\n");
    }

    return result;
}

static bool mqtt_send(const char *topic, const char *value, bool retained)
{
    bool result = mqttClient.connected();
    if (result) {
        Serial.printf("Publishing %s to %s ...", value, topic);
        result = mqttClient.publish(topic, value, retained);
        Serial.println(result ? "OK" : "FAIL");
    }
    return result;
}

void setup(void)
{
    pinMode(PIN_IRQ, INPUT);
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
        while (1);
    }
    // ignore disturbances
    lightning.maskDisturber(true);
    // print summary of settings
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
    sprintf(esp_id, "%06x", ESP.getChipId());
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
    snprintf(statustopic, sizeof(statustopic), MQTT_TOPIC, esp_id, "status");
    snprintf(valuetopic, sizeof(valuetopic), MQTT_TOPIC, esp_id, "bliksem");
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    // NTP
    ntp.begin();
}

void loop(void)
{
    if (digitalRead(PIN_IRQ) != 0) {
        // wait 2 ms and read interrupt cause
        delay(2);

        char value[256];
        int intReg = lightning.readInterruptReg();
        unsigned long int time = ntp.getEpochTime();
        int distance;
        int energy;
        switch (intReg) {
        case NOISE_TO_HIGH:
        case DISTURBER_DETECT:
            // ignore
            break;
        case LIGHTNING:
            distance = lightning.distanceToStorm();
            energy = lightning.lightningEnergy();
            if (distance > 1) {
                snprintf(value, sizeof(value), "{\"time\":%lu,\"distance\":%d,\"energy\":%d}", time,
                         distance, energy);
                mqtt_send(valuetopic, value, true);
            }
            break;
        default:
            // spurious, ignore
            break;
        }
    }
    // keep MQTT alive
    mqtt_alive();

    ArduinoOTA.handle();

    ntp.update();
}
