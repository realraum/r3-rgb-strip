#include <Arduino.h>

// 3rdparty lib includes
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

constexpr const char * const wifi_ssid = "realraum";
constexpr const char * const wifi_pass = "r3alraum";
constexpr const char * const mqtt_url = "mqtt.realraum.at";
constexpr const char * const mqtt_id = "esp8266-rgb-ledstrip";
constexpr const char * const mqtt_base_topic = "action/rgb-ledstrip/#";
constexpr const char * const mqtt_status_topic = "action/rgb-ledstrip/status";
constexpr const char * const mqtt_online_topic = "action/rgb-ledstrip/online";

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

ledstrip_status_t ledstrip_status = {0, 0, 0, 0};

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
    if (strcmp(topic, mqtt_status_topic) != 0)
        return;

    // check if message is valid
    if (!doc.containsKey("r") || !doc.containsKey("g") || !doc.containsKey("b") || !doc.containsKey("br"))
        return;

    // set ledstrip
    setLedstrip(doc["r"], doc["g"], doc["b"], doc["br"]);
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
      mqttClient.subscribe(mqtt_status_topic);
      Serial.printf("Subscribed to %s!\n", mqtt_status_topic);
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
    if (!mqttClient.connected())
    {
        reconnect();
    }
    mqttClient.loop();

    static uint32_t last_millis_render = 0;
    if (millis() - last_millis_render > 1)
    {
        last_millis_render = millis();
        render_ledstrip();
    }

    delay(1);
}
