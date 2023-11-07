// Incluimos la librería para Servo
#include <ESP32Servo.h>
#include <ArduinoJson.h>    // Lib to format JSON Document
#include "EspMQTTClient.h"  // Lib to comunicate MQTT from ESP

#define servoPin 9
#define pirOutside 4
#define pirInside 2

char myJson[300];

String mqttTopic = "doorWatch";

//MQTT and WiFi configuration
EspMQTTClient client(
  "WiFI",          //nome da sua rede Wi-Fi
  "2153818aa",     //senha da sua rede Wi-Fi
  "mqtt.tago.io",  //Endereço do servidor MQTT
  "Default",       //User é sempre default pois vamos usar token
  "TOKEN",         // Código do Token
  "esp32",         //Nome do device
  1883             //Porta de comunicação padrao
);

void connectToWifi() {
  Serial.println("Conectando WiFi");
  while (!client.isWifiConnected()) {
    Serial.print('.');
    client.loop();
    delay(1000);
  }
  Serial.println("Conectado!");
}

void connectToTago() {
  Serial.println("Conectando com o broker MQTT");
  while (!client.isMqttConnected()) {
    Serial.print('.');
    client.loop();
    delay(1000);
  }
  Serial.println("Conectado!");
}

bool isDoorOpen = false;
long closeDoorIn;
int doorStandOpenFor = 3000;

int PIR_OUTSIDE = 1;
int PIR_INSIDE = 2;

int SERVO_OPEN_ANGLE = 180;
int SERVO_CLOSE_ANGLE = 0;

Servo servo;  // Instanciando a classe Servo

void setup() {
  Serial.begin(9600);

  pinMode(pirOutside, INPUT);
  pinMode(pirInside, INPUT);

  servo.attach(servoPin);
  servo.write(SERVO_CLOSE_ANGLE);

  connectToWifi();
  connectToTago();
}
// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished() {}

void loop() {
  client.loop();
  const long now = millis();
  if (isDoorOpen && now >= closeDoorIn) {
    closeDoor();
  }

  int pirOutsideValue = digitalRead(pirOutside);
  int pirInsideValue = digitalRead(pirInside);

  if (pirOutsideValue == HIGH) {
    openDoor(PIR_OUTSIDE);
  }
  if (pirInsideValue == HIGH) {
    openDoor(PIR_INSIDE);
  }
}

void incrementDoorTime() {
  closeDoorIn = millis() + doorStandOpenFor;
}

void openDoor(int openedBy) {
  if (isDoorOpen) {
    incrementDoorTime();
  } else {
    isDoorOpen = true;
    servo.write(SERVO_OPEN_ANGLE);
    StaticJsonDocument<300> documentoJson;

    documentoJson["variable"] = 'DoorIsOpen';
    documentoJson["value"] = true,
    serializeJson(documentoJson, myJson);
    client.publish(mqttTopic, myJson);

    if (openedBy == PIR_OUTSIDE) {
      Serial.println("The door was opened by the outside sensor");
      StaticJsonDocument<300> documentoJsonOut;

      documentoJsonOut["variable"] = 'OpenedBy';
      documentoJsonOut["value"] = "OUTSIDE",
      serializeJson(documentoJsonOut, myJson);
      client.publish(mqttTopic, myJson);

    } else if (openedBy == PIR_INSIDE) {
      Serial.println("The door was opened by the inside sensor");
      StaticJsonDocument<300> documentoJsonIn;

      documentoJsonIn["variable"] = 'OpenedBy';
      documentoJsonIn["value"] = "INSIDE",
      serializeJson(documentoJsonIn, myJson);
      client.publish(mqttTopic, myJson);
    }
    incrementDoorTime();
    // Send that door is open
  }
}

void closeDoor() {
  isDoorOpen = false;
  servo.write(SERVO_CLOSE_ANGLE);
  Serial.println("The door was closed");
  StaticJsonDocument<300> documentoJson;

  documentoJson["variable"] = 'DoorIsOpen';
  documentoJson["value"] = false,
  serializeJson(documentoJson, myJson);
  client.publish(mqttTopic, myJson);

  // Send that door is closed
}
