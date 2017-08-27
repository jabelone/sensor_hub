#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string.h>

// Pin Settings
const int MOTION_PIN = D7;
const int WINDOW_PIN = D6;

// WiFi Settings
const char* ssid = "SkyNet";
const char* password = "225261007622";

// MQTT Settings
const char* mqttServer = "192.168.0.42";
const char*  mqttUser = "sensor";
const char* mqttPass = "F20TKPWI9wrqYTyh";

//SensorHub Settings
const String hubName = "livingroom";

// Shouldn't need touch anything below here
//Generate all our topic names
const String sendDataTopic = "sensorhub/" + hubName + "/data";
const String sendInfoTopic = "sensorhub/" + hubName + "/info";


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttUser, mqttPass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      StaticJsonBuffer<500> jsonBuffer;
      JsonObject& readyJson = jsonBuffer.createObject();
      readyJson["status"] = "READY";
      readyJson["ip"] = WiFi.localIP().toString();
      char json[256];
      readyJson.printTo(json, sizeof(json));
      
      client.publish(sendInfoTopic.c_str(), json);
      // ... and resubscribe
      client.subscribe("sensorhub/control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(MOTION_PIN, INPUT); // Motion Sensor
  pinMode(WINDOW_PIN, INPUT); // Window Sensor
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now;
    StaticJsonBuffer<500> jsonBuffer;
    JsonObject& dataJson = jsonBuffer.createObject();
    dataJson["motionSensor"] = (bool)digitalRead(MOTION_PIN);
    dataJson["lightLevel"] = analogRead(A0);
    dataJson["windowSensor"] = !digitalRead(WINDOW_PIN);
    char json[256];
    dataJson.printTo(json, sizeof(json));
    
    Serial.print("Published data: ");
    Serial.println(json);
    client.publish(sendDataTopic.c_str(), json);
  }
}
