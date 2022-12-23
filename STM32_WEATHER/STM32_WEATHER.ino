//STM32, STM32LowPower.h, LoRa_STM32
//Adafruit_Sensor
//Adafruit_BME280

#include <Wire.h>
#include <SPI.h>
#include "STM32LowPower.h"
#include <LoRa_STM32.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

#define NSS PA4
#define RST PB8
#define DI0 PB9

#define VBAT_sensor PA0
 
#define TX_P 17
#define BAND 433E6
#define ENCRYPT 0x78

uint32_t sleepTime = 600000;//10 minutes
String LoRaMessage = "";
char waktu[100];

void setup()
{
  //Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);

  LoRa.setTxPower(17);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(12);
  LoRa.setSyncWord(ENCRYPT);
  LoRa.setPins(NSS, RST, DI0);
  LoRa.begin(BAND);

  workingArea(); 
  LowPower.begin();
  LowPower.deepSleep(sleepTime); //deepsleep selama 10 menit
  LowPower.shutdown(1);
}

void loop() {
}

void workingArea() {
  bme.begin(0x76);
  analogReadResolution(12);
  int val = analogRead(VBAT_sensor); //read ADC VBAT
  //  Serial.println(val);
  float vbat = (float(val) / 4096) * 3.323 * 2.482; //formulae to convert the ADC value to voltage

  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;
  bme_sleep();

  //Serial.print(F("Sending packet: "));
  LoRaMessage = "001|" + String(vbat) + "|" + String(temperature) + "|" +  String(humidity) + "|" +  String(pressure);
  //Serial.println(LoRaMessage);

  // send packet
  LoRa.idle();
  LoRa.beginPacket();
  LoRa.print(LoRaMessage);
  LoRa.endPacket();
  LoRa.sleep();
}

void bme_sleep() {
  Wire.beginTransmission(0x76);    // or 0x77
  Wire.write((uint8_t)0xF4);       // Select Control Measurement Register
  Wire.write((uint8_t)0b00000000); // Send '00' for Sleep mode
  Wire.endTransmission();
}
