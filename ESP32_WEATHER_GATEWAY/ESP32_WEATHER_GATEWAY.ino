#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include "time.h"
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         &Wire, -1);
//definisikan pin untuk LoRa
#define ss 5
#define rst 4
#define dio0 2
#define BAND 433E6
#define ENCRYPT 0x78
WiFiClient espClient;
PubSubClient client(espClient);

const char* ssid = "SSID";
const char* password = "PASSWORD";
const char* mqttServer = "IPADDRESS";
const int mqttPort = 1883;
const char* mqttUser = "MQTTUSERNAME";
const char* mqttPassword = "MQTTPASSPORD";

String nodeId, nodeVbat, nodeTemp, nodeHum, nodePress;
unsigned long lastTimer = 0;
unsigned long lastTimerDisplay = 0;
unsigned long timerDelayDisplay = 60000;
boolean data_ready = false;

void displayStatus(String vbat, String temp, String hum,
                   String pressure) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 9);
  display.print("IP :");
  display.print(WiFi.localIP());
  display.setCursor(0, 18);
  display.print("Temp. :");
  display.print(temp);
  display.setCursor(0, 27);
  display.print("Hum. :");
  display.print(hum);
  display.setCursor(0, 36);
  display.print("Press.:");
  display.print(pressure);
  display.setCursor(0, 45);
  display.print("Batt. :");
  display.print(vbat);
  display.print("V");
  display.display();
  delay(100);
}

void displaying(int x, int y, String dataDisplay) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(x, y);
  display.print(dataDisplay);
  display.display();
  delay(100);
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;
  while (LoRa.available()) {
    String LoRaData = LoRa.readString();
    Serial.print(LoRaData);
    nodeId = getValue(LoRaData, '|', 0);
    nodeVbat = getValue(LoRaData, '|', 1);
    nodeTemp = getValue(LoRaData, '|', 2);
    nodeHum = getValue(LoRaData, '|', 3);
    nodePress = getValue(LoRaData, '|', 4);
  }
  Serial.print("RSSI: ");
  Serial.print(LoRa.packetRssi());
  Serial.print("Snr: ");
  Serial.println(LoRa.packetSnr());
  data_ready = true;
  //mqtt_send("001", nodeTemp, nodeHum, nodePress,  nodeVbat);

  //displayStatus(nodeTime, nodeVbat, nodeTemp, nodeHum,  nodePress, String(LoRa.packetRssi()));
}

void wifi_reconnect() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("esp32weather", mqttUser,
                       mqttPassword )) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }

}

void mqtt_send(String id, String d0, String d1, String
               d2, String d3) {
  if (WiFi.status() != WL_CONNECTED) {
    wifi_reconnect();
  }
  if (!client.connected()) {
    mqtt_reconnect();
  }
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JSONencoder["id"] = id;
  JSONencoder["temp"] = d0;
  JSONencoder["hum"] = d1;
  JSONencoder["press"] = d2;
  JSONencoder["volt"] = d3;
  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer,
                      sizeof(JSONmessageBuffer));
  Serial.println("Sending message to MQTT topic..");
  Serial.println(JSONmessageBuffer);
  if (client.publish("weather", JSONmessageBuffer) ==
      true) {
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
  }

  Serial.println(client.loop());
  Serial.println(client.state());
  Serial.println("-------------");
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;
  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0],
                                        strIndex[1]) : "";
}

void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED NOT WORKING");
  }
  wifi_reconnect();
  client.setServer(mqttServer, mqttPort);
  mqtt_reconnect();

  LoRa.setPins(ss, rst, dio0);
  LoRa.setSyncWord(ENCRYPT);
  LoRa.setSignalBandwidth(125E3);
  while (!LoRa.begin(BAND)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.onReceive(onReceive);
  LoRa.receive();
  displaying(0, 0, "OK");
}

void loop() {
  client.loop();
  if (data_ready == true) {
    mqtt_send(nodeId, nodeTemp, nodeHum, nodePress,
              nodeVbat);
    displayStatus(nodeVbat, nodeTemp, nodeHum,
                  nodePress);
    data_ready = false;
  }
}
