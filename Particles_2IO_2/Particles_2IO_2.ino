/*
  VINDRIKTNING Ikea Luftgüte Sensor -> AdafruitIO
  Serial Code basierend auf https://github.com/Hypfer/esp8266-vindriktning-particle-sensor
*/

#include "ESP8266WiFi.h"
#include <SoftwareSerial.h>
#include "config.h"

// Je nach Sensor Entsprechend benennen
AdafruitIO_Feed *pm25 = io.feed("TEST_PM25");

// Je nach ESP-Board!
constexpr static const uint8_t PIN_UART_RX = 4; // D2 an Wemos D1 Mini
constexpr static const uint8_t PIN_UART_TX = 13; // UNUSED

SoftwareSerial sensorSerial(PIN_UART_RX, PIN_UART_TX);

// Variablen
uint8_t serialRxBuf[80];
uint8_t rxBufIdx = 0;
int spm25 = 0;
int last = 0;
unsigned long timepast = millis();

// AdafruitIO darf nicht "geflutet" werden
const int sendinterval = 20; // in Sekunden


void setup() {
  // Software Serial für Sensor
  sensorSerial.begin(9600);

  // Debug Serial
  Serial.begin(115200);
  while (! Serial);

  Serial.print("Verbinde mit Adafruit IO");
  // verbinde...
  io.connect();

  // auf Verbindung warten
  while (io.status() < AIO_CONNECTED) {
    Serial.printf("Verbinde: %s \n",io.statusText());
    delay(500);
  }
  Serial.println("Verbunden mit Adafruit IO!");
}

int getSensorData() {
  uint8_t rxBufIdx = 0;
  uint8_t checksum = 0;

  // Sensor Serial aushorchen
  while ((sensorSerial.available() && rxBufIdx < 127) || rxBufIdx < 20) 
  {
    serialRxBuf[rxBufIdx++] = sensorSerial.read();
    // Wenn es Probleme mit dem Empfang gibt:
    //    delay(15);
  }

  // Prüfsumme berechnen
  for (uint8_t i = 0; i < 20; i++) 
  {
    checksum += serialRxBuf[i];
  }

  // Header und Prüfsumme checken
  if (serialRxBuf[0] == 0x16 && serialRxBuf[1] == 0x11 && serialRxBuf[2] == 0x0B && checksum == 0)
  {
    return (serialRxBuf[5] << 8 | serialRxBuf[6]);
  }
  else
  {
    return (-1);
  }
}


void loop() {
  // Adafruit senden
  io.run();
  
  // Sensor abfragen
  spm25 = getSensorData();
  delay(500);

  if (spm25 > 0)
  {
    last = spm25;
  }

  if (millis() > timepast + sendinterval * 1000 && last>0)
  {
    timepast = millis();
    // Ausgabe Debug Monitor
    Serial.printf("Time: %d --- PM2.5=%d µg/m3 (Unkalibriert) \n", millis() / 1000, last);
    // für Adafruit IO speichern
    pm25->save(last);
    delay(100);
  }
}
