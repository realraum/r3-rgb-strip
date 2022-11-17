// non-addressable ledstrip controlled via MQTT and PWM
#include <Arduino.h>

// 3rdparty lib includes
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// local includes
#include "typesafeenum.h"

constexpr const char * const wifi_ssid = "realraum";
constexpr const char * const wifi_pass = "r3alraum";
constexpr const char * const mqtt_url = "mqtt.realraum.at";
constexpr const char * const mqtt_id = "esp8266-ducttape-ledstrip";
constexpr const char * const mqtt_base_topic = "action/ducttape-ledstrip/#";
constexpr const char * const mqtt_status_topic = "action/ducttape-ledstrip/status";
constexpr const char * const mqtt_light_topic = "action/ducttape-ledstrip/light";
constexpr const char * const mqtt_online_topic = "action/ducttape-ledstrip/online";
constexpr const char * const mqtt_ledstrip_modes_topic = "action/ducttape-ledstrip/modes";
constexpr const char * const mqtt_info_topic = "action/ducttape-ledstrip/info";

// r3 specific topics
constexpr const char * const mqtt_realraum_w2frontdoor_topic = "realraum/w2frontdoor/lock";
constexpr const char * const mqtt_realraum_w1frontdoor_topic = "realraum/frontdoor/lock";

constexpr const uint8_t pin_red = D5;
constexpr const uint8_t pin_green = D6;
constexpr const uint8_t pin_blue = D7;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t brightness;

    uint8_t red_pwm;
    uint8_t green_pwm;
    uint8_t blue_pwm;
    uint8_t current_brightness;
} ledstrip_status_t;

#define LEDSTRIP_MODE_VALUES(x) \
    x(MODE_DEFAULT) \
    x(MODE_RAINBOW) \
    x(MODE_STROBO) \
    x(MODE_W1DOOR) \
    x(MODE_W2DOOR) \
    x(_)
DECLARE_TYPESAFE_ENUM(ledstrip_mode_t, : uint8_t, LEDSTRIP_MODE_VALUES);

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t brightness;

    ledstrip_mode_t mode;
} mqtt_status_t;

typedef struct {
    bool w1frontdoor_locked;
    bool w2frontdoor_locked;
} realraum_status_t;

ledstrip_status_t ledstrip_status;
mqtt_status_t mqtt_status;
realraum_status_t realraum_status;

WiFiClient wifiClient;
PubSubClient mqttClient{wifiClient};

void setup_ledstrip()
{
    pinMode(pin_red, OUTPUT);
    pinMode(pin_green, OUTPUT);
    pinMode(pin_blue, OUTPUT);

    digitalWrite(pin_red, HIGH);
    digitalWrite(pin_green, HIGH);
    digitalWrite(pin_blue, HIGH);

    ledstrip_status.red = 0;
    ledstrip_status.green = 0;
    ledstrip_status.blue = 0;
    ledstrip_status.brightness = 255;

    mqtt_status.red = 0;
    mqtt_status.green = 0;
    mqtt_status.blue = 0;
    mqtt_status.mode = ledstrip_mode_t::MODE_DEFAULT;
}

void render_ledstrip(bool instant = false)
{
    if (instant)
    {
        ledstrip_status.red_pwm = ledstrip_status.red;
        ledstrip_status.green_pwm = ledstrip_status.green;
        ledstrip_status.blue_pwm = ledstrip_status.blue;
        ledstrip_status.current_brightness = ledstrip_status.brightness;
    }
    else
    {
        if (ledstrip_status.red_pwm < ledstrip_status.red)
            ledstrip_status.red_pwm++;
        else if (ledstrip_status.red_pwm > ledstrip_status.red)
            ledstrip_status.red_pwm--;

        if (ledstrip_status.green_pwm < ledstrip_status.green)
            ledstrip_status.green_pwm++;
        else if (ledstrip_status.green_pwm > ledstrip_status.green)
            ledstrip_status.green_pwm--;

        if (ledstrip_status.blue_pwm < ledstrip_status.blue)
            ledstrip_status.blue_pwm++;
        else if (ledstrip_status.blue_pwm > ledstrip_status.blue)
            ledstrip_status.blue_pwm--;

        if (ledstrip_status.current_brightness < ledstrip_status.brightness)
            ledstrip_status.current_brightness++;
        else if (ledstrip_status.current_brightness > ledstrip_status.brightness)
            ledstrip_status.current_brightness--;
    }

    analogWrite(pin_red, ledstrip_status.red_pwm * ledstrip_status.current_brightness / 255);
    analogWrite(pin_green, ledstrip_status.green_pwm * ledstrip_status.current_brightness / 255);
    analogWrite(pin_blue, ledstrip_status.blue_pwm * ledstrip_status.current_brightness / 255);
}

void setLedstrip(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness)
{
    ledstrip_status.red = red;
    ledstrip_status.green = green;
    ledstrip_status.blue = blue;
    ledstrip_status.brightness = brightness;
}

void setMQTTLedstrip(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness)
{
    mqtt_status.red = red;
    mqtt_status.green = green;
    mqtt_status.blue = blue;
    mqtt_status.brightness = brightness;
}

void setMQTTLedstripRed(uint8_t red)
{
    mqtt_status.red = red;
}

void setMQTTLedstripGreen(uint8_t green)
{
    mqtt_status.green = green;
}

void setMQTTLedstripBlue(uint8_t blue)
{
    mqtt_status.blue = blue;
}

void setMQTTLedstripBrightness(uint8_t brightness)
{
    mqtt_status.brightness = brightness;
}

void setLedstripBrightness(uint8_t brightness)
{
    mqtt_status.brightness = brightness;
    ledstrip_status.brightness = brightness;
}

void handle_rainbow()
{
    static uint32_t last_update = 0;
    const uint32_t now = millis();

    if (now - last_update > 50)
    {
        last_update = now;

        static uint16_t hue = 0; // 0 - 360
        hue += 1;

        if (hue > 360)
            hue = 0;

        uint8_t red, green, blue;

        if (hue < 60)
        {
            red = 255;
            green = hue * 255 / 60;
            blue = 0;
        }
        else if (hue < 120)
        {
            red = (120 - hue) * 255 / 60;
            green = 255;
            blue = 0;
        }
        else if (hue < 180)
        {
            red = 0;
            green = 255;
            blue = (hue - 120) * 255 / 60;
        }
        else if (hue < 240)
        {
            red = 0;
            green = (240 - hue) * 255 / 60;
            blue = 255;
        }
        else if (hue < 300)
        {
            red = (hue - 240) * 255 / 60;
            green = 0;
            blue = 255;
        }
        else
        {
            red = 255;
            green = 0;
            blue = (360 - hue) * 255 / 60;
        }

        setLedstrip(red, green, blue, ledstrip_status.brightness);
    }
}

void handle_strobo()
{
    static uint32_t last_update = 0;
    const uint32_t now = millis();

    if (now - last_update > 100)
    {
        last_update = now;

        static bool on = false;
        on = !on;

        if (on)
            setLedstrip(255, 255, 255, ledstrip_status.brightness);
        else
            setLedstrip(0, 0, 0, ledstrip_status.brightness);
    }
}

void handle_w1status()
{
    static uint32_t last_update = 0;
    const uint32_t now = millis();

    if (now - last_update > 100)
    {
        last_update = now;

        if (realraum_status.w1frontdoor_locked)
        {
            setLedstrip(255, 0, 0, ledstrip_status.brightness); // red
        }
        else
        {
            setLedstrip(0, 255, 0, ledstrip_status.brightness); // green
        }
    }
}

void handle_w2status()
{
    static uint32_t last_update = 0;
    const uint32_t now = millis();

    if (now - last_update > 100)
    {
        last_update = now;

        if (realraum_status.w2frontdoor_locked)
        {
            setLedstrip(255, 0, 0, ledstrip_status.brightness); // red
        }
        else
        {
            setLedstrip(0, 255, 0, ledstrip_status.brightness); // green
        }
    }
}

void handle_ledstrip()
{
    switch (mqtt_status.mode)
    {
        case ledstrip_mode_t::MODE_DEFAULT:
        {
            ledstrip_status.red = mqtt_status.red;
            ledstrip_status.green = mqtt_status.green;
            ledstrip_status.blue = mqtt_status.blue;
            ledstrip_status.brightness = mqtt_status.brightness;
            break;
        }
        case ledstrip_mode_t::MODE_RAINBOW:
            handle_rainbow();
            break;
        case ledstrip_mode_t::MODE_STROBO:
            handle_strobo();
            break;
        case ledstrip_mode_t::MODE_W1DOOR:
            handle_w1status();
            break;
        case ledstrip_mode_t::MODE_W2DOOR:
            handle_w2status();
            break;
        default:;
    }
}

void publish_modes()
{
    StaticJsonDocument<256> doc;

    iterateledstrip_mode_t([&](ledstrip_mode_t mode, const auto &string_value) {
        if (mode != ledstrip_mode_t::_)
            doc[string_value] = static_cast<uint8_t>(mode);
    });

    char buffer[256];
    serializeJson(doc, buffer);

    mqttClient.publish(mqtt_ledstrip_modes_topic, buffer);
}

void publish_info()
{
    StaticJsonDocument<256> doc;

    doc["sha"] = GIT_HASH;
    doc["repo"] = "https://github.com/realraum/r3-rgb-strip/";

    char buffer[256];

    serializeJson(doc, buffer);

    mqttClient.publish(mqtt_info_topic, buffer);
}

void setup_wifi()
{
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(wifi_ssid);

    WiFi.begin(wifi_ssid, wifi_pass);

    setLedstrip(0, 0, 0, 255);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
        ledstrip_status.red = ledstrip_status.red == 0 ? 255 : 0;
        render_ledstrip(true);
    }

    ledstrip_status.red = 0;

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
    // debug log message
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    for (uint8_t i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }

    Serial.println();

    // json parse message
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }

    // check if message is for us
    if (strcmp(topic, mqtt_status_topic) == 0)
    {
        return; // disable for now, later add only brightness and mode and ignore brightness on mode 0
        if (doc.containsKey("r") && doc["r"].is<uint8_t>())
        {
            Serial.printf("mqtt: red: %d\n", doc["r"].as<uint8_t>());
            setMQTTLedstripRed(doc["r"].as<uint8_t>());
        }
        if (doc.containsKey("g") && doc["g"].is<uint8_t>())
        {
            Serial.printf("mqtt: g: %d\n", doc["g"].as<uint8_t>());
            setMQTTLedstripGreen(doc["g"].as<uint8_t>());
        }
        if (doc.containsKey("b") && doc["b"].is<uint8_t>())
        {
            Serial.printf("mqtt: b: %d\n", doc["b"].as<uint8_t>());
            setMQTTLedstripBlue(doc["b"].as<uint8_t>());
        }
        if (doc.containsKey("br") && doc["br"].is<uint8_t>())
        {
            Serial.printf("mqtt: br: %d\n", doc["br"].as<uint8_t>());
            setLedstripBrightness(doc["br"].as<uint8_t>());
        }
        if (doc.containsKey("mode") && doc["mode"].is<uint8_t>())
        {
            const uint8_t mode = doc["mode"].as<uint8_t>();
            constexpr const uint8_t last_mode = static_cast<uint8_t>(ledstrip_mode_t::_) - 1;

            if (mode <= last_mode)
            {
                Serial.printf("mqtt: mode: %d\n", mode);
                mqtt_status.mode = static_cast<ledstrip_mode_t>(mode);
            }
            else
            {
                Serial.printf("mqtt: mode: %d is out of range\n", mode);
            }
        }
    }
    else if (strcmp(topic, mqtt_light_topic) == 0) // {r:1000,g:1000,b:0} => convert 0-1000 to 0-255, also set brightness to 255 and mode to 0
    {
        if (doc.containsKey("r") && doc["r"].is<uint16_t>())
        {
            const auto value = doc["r"].as<uint16_t>() * 255 / 1000;
            Serial.printf("mqtt: red: %d (%d * 255 / 1000)\n", value, doc["r"].as<uint16_t>());
            setMQTTLedstripRed(value);
        }
        if (doc.containsKey("g") && doc["g"].is<uint16_t>())
        {
            const auto value = doc["g"].as<uint16_t>() * 255 / 1000;
            Serial.printf("mqtt: g: %d (%d * 255 / 1000)\n", value, doc["g"].as<uint16_t>());
            setMQTTLedstripGreen(value);
        }
        if (doc.containsKey("b") && doc["b"].is<uint16_t>())
        {
            const auto value = doc["b"].as<uint16_t>() * 255 / 1000;
            Serial.printf("mqtt: b: %d (%d * 255 / 1000)\n", value, doc["b"].as<uint16_t>());
            setMQTTLedstripBlue(value);
        }
        setLedstripBrightness(255);
        mqtt_status.mode = ledstrip_mode_t::MODE_DEFAULT;
    }
    else if (strcmp(topic, mqtt_realraum_w2frontdoor_topic) == 0)
    {
        if (doc.containsKey("Locked") && doc["Locked"].is<bool>())
        {
            Serial.printf("mqtt: w2frontdoor: %s\n", doc["Locked"].as<bool>() ? "locked" : "unlocked");
            realraum_status.w2frontdoor_locked = doc["Locked"].as<bool>();
        }
    }
    else if (strcmp(topic, mqtt_realraum_w1frontdoor_topic) == 0)
    {
        if (doc.containsKey("Locked") && doc["Locked"].is<bool>())
        {
            Serial.printf("mqtt: w1frontdoor: %s\n", doc["Locked"].as<bool>() ? "locked" : "unlocked");
            realraum_status.w1frontdoor_locked = doc["Locked"].as<bool>();
        }
    }
    else
    {
        Serial.print("Unknown topic: ");
        Serial.println(topic);
    }
}

void send_status()
{
    StaticJsonDocument<128> doc;
    doc["ip"] = WiFi.localIP().toString();
    doc["online"] = true;

    char json[128];
    serializeJson(doc, json);
    mqttClient.publish(mqtt_online_topic, json);
}

void reconnect()
{
    // Loop until we're reconnected
    while (!mqttClient.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect with will message

        StaticJsonDocument<128> doc;
        doc["ip"] = WiFi.localIP().toString();
        doc["online"] = false;

        char json[128];
        serializeJson(doc, json);

        if (mqttClient.connect(mqtt_id, mqtt_online_topic, 0, true, json))
        {
            Serial.println("connected");
            mqttClient.publish("outTopic", "hello world");

            publish_modes();
            publish_info();

            // subscribe
            mqttClient.subscribe(mqtt_status_topic);
            Serial.printf("Subscribed to %s!\n", mqtt_status_topic);

            mqttClient.subscribe(mqtt_light_topic);
            Serial.printf("Subscribed to %s!\n", mqtt_light_topic);

            mqttClient.subscribe(mqtt_realraum_w1frontdoor_topic);
            Serial.printf("Subscribed to %s!\n", mqtt_realraum_w1frontdoor_topic);

            mqttClient.subscribe(mqtt_realraum_w2frontdoor_topic);
            Serial.printf("Subscribed to %s!\n", mqtt_realraum_w2frontdoor_topic);

            send_status();      
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void setup()
{
    Serial.begin(115200);

    // setup leds
    setup_ledstrip();

    // setup wifi
    setup_wifi();

    // setup mqtt
    mqttClient.setServer(mqtt_url, 1883);
    mqttClient.setCallback(mqtt_callback);

    // turn off ledstrip
    setLedstrip(0, 0, 0, 0);
}

void loop()
{
    bool instant = false;

    if (!mqttClient.connected())
    {
        reconnect();
    }
    mqttClient.loop();

    handle_ledstrip();

    if (mqtt_status.mode == ledstrip_mode_t::MODE_STROBO)
        instant = true;

    static uint32_t last_millis_render = 0;
    if (millis() - last_millis_render > 1)
    {
        last_millis_render = millis();
        render_ledstrip(instant);
    }

    delay(1);
}
