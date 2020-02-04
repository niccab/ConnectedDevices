#include <SPI.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"

WiFiClient client;
char server[] = "10.23.11.48"; //IP address

const int buttonPin[] = {6, 5, 4, 3, 2};
const int ledPin[] =    {11, 10, 9, 8, 7};
char control[] = {'l','r','u','d','x'};
const int buttonCount = 5;
const int ledCount = 5;

bool ledState[ledCount];
bool buttonState[buttonCount];
bool lastButtonState[buttonCount];

unsigned long lastDebounceTime[buttonCount];
unsigned long debounceDelay = 50;

void setup() {

  Serial.begin(115200);
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(2000); //try to connect every two seconds
  }

  //now that you're connected...
  Serial.println(WiFi.SSID());
  Serial.println(WiFi.localIP());
  if (client.connect(server, 8080)) {
    Serial.println("Woo hoo!");

  }

  for (int i = 0; i < ledCount; i++) {
    pinMode(ledPin[i], OUTPUT);            // use a for loop to initialize each pin as an output:
  }
  for (int i = 0; i < buttonCount; i++) {
    pinMode(buttonPin[i], INPUT_PULLUP);       // use a for loop to initialize each pin as an input
  }

}

void loop() {

  
  // read what the server sends

  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  if (client.connected()) {

  for (byte i = 0; i < buttonCount; i++) {

    bool reading = !digitalRead (buttonPin[i]); 
    if (reading != lastButtonState[i])
    {
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i] > debounceDelay))
    {
      if (reading != buttonState[i])
      {
        Serial.print(F("buttonPin[")); Serial.print(i); Serial.print(F("]=")); Serial.print(buttonPin[i]); Serial.print(F(" --> ")); Serial.println(reading);
        buttonState[i] = reading;

        if (buttonState[i] == HIGH) {
          ledState[i] = !ledState[i];
          digitalWrite(ledPin[i], HIGH);
          client.print(control[i]);
          //Serial.print(F("ledState[")); Serial.print(i); Serial.print(F("]=")); Serial.println(ledState[i]);
        }
        if (buttonState[i] == LOW){
                    ledState[i] = !ledState[i];

          digitalWrite(ledPin[i], LOW);
          }
      }
    }
    lastButtonState[i] = reading;
  }



  }


}
