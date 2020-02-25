/* 
 *  Nicole Cabalquinto
 *  Connected Devices: Hue Light Controller
 *  February 25, 2020
 *  
 *  Modified "HueBlinkWithJsonEncoder.ino"
 *  https://github.com/tigoe/hue-control/blob/master/ArduinoExamples/HueBlinkWithJsonEncoder/HueBlinkWithJsonEncoder.ino
 *  
 */

#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <Arduino_JSON.h>
#include <Encoder.h>
#include "arduino_secrets.h"

// includes and defines for the SSD1306 display:
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

// initialize display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int status = WL_IDLE_STATUS;      // the Wifi radio's status
char hueHubIP[] = "172.22.151.181";  // IP address of the HUE bridge
String hueUserName = "EVVzNhLeRPzX9Rnd4DmyoHq5t0eFI6RbN0zV6EQ9"; // hue bridge username

// make a wifi instance and a HttpClient instance:
WiFiClient wifi;
HttpClient httpClient = HttpClient(wifi, hueHubIP);

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// a JSON object to hold the light state:
JSONVar lightState;

Encoder knob(A1, A2);               // encoder on these two pins
const int encoderButtonPin = 2;     // button input pin
const int sendButtonPin = 3;        // HTTP request send button
int lastEncoderButtonState = HIGH;  // previous button states
int lastSendButtonState = HIGH;
int debounceDelay = 4;              // 4ms debounce for button
int lastKnobState = -1;             // previous knob position

int currentProperty = 0;            // property to be changed
int propertyCount = 4;              // count of properties in use
int ledPin = 5;

String scr[] = {"on", "bri", "hue", "sat"};

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  while (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println("SSD1306 setup...");
    delay(100);
  }

  pinMode(ledPin, OUTPUT);

  while (!Serial);

  //   attempt to connect to Wifi network:
  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to: ");
    displayWrite("connecting:" + String(SECRET_SSID));
    Serial.println(SECRET_SSID);
    WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(2000);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network IP = ");
  displayWrite("Connected!");
  digitalWrite(ledPin, HIGH);
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  // initialize buttons:
  pinMode(encoderButtonPin, INPUT_PULLUP);
  pinMode(sendButtonPin, INPUT_PULLUP);

  // establish the properties in lightState that you want to use, in order:
  lightState["on"] = true;   // true/false
  lightState["bri"] = 255;      // 0 -255
  lightState["hue"] = 0;      // 0-65535
  lightState["sat"] = 255;      // 0-255
  // property count is however many properties you set in lightState:
  propertyCount = lightState.keys().length();
}

void loop() {
  // read the encoder button, look for a state change:
  int encoderButtonState = digitalRead(encoderButtonPin);
  if (encoderButtonState != lastEncoderButtonState) {
    delay(debounceDelay);
    if (encoderButtonState == LOW) {
      // change which property is to be changed by the encoder:
      currentProperty++;
      // if the count goes to high, reset to 0:
      if (currentProperty == propertyCount) {
        currentProperty = 0;
      }
      // print thekey of the property to be changed:
      Serial.print("changing ");
      Serial.println( lightState.keys()[currentProperty]);
    }
    // save current button state for next time:
    lastEncoderButtonState = encoderButtonState;
  }

  // read the encoder:
  int knobState = knob.read();
  // look for the knob to change by 4 stops (1 detent):
  if ( abs(knobState - lastKnobState) >= 4) {
    // get the direction of knob change:
    int knobChange = abs(knobState - lastKnobState) / ( knobState - lastKnobState);
    // change the current property:
    changeLightProperty(currentProperty, knobChange);
    // save current knob state for next time:
    lastKnobState = knobState;
  }

  // read the send button, look for state change:
  int sendButtonState = digitalRead(sendButtonPin);
  if (sendButtonState != lastSendButtonState) {
    delay(debounceDelay);
    if (sendButtonState == LOW) {
      // send the request
      sendRequest(1, lightState);
    }
    // save the current button state for next time:
    lastSendButtonState = sendButtonState;
  }
}

void sendRequest(int light, JSONVar myState) {
  // make a String for the HTTP request path:
  String request = "/api/" + String(hueUserName);
  request += "/lights/";
  request += light;
  request += "/state/";

  String contentType = "application/json";

  // make a string for the JSON command:
  String body  = JSON.stringify(lightState);

  // see what you assembled to send:
  Serial.print("PUT request to server: ");
  Serial.println(request);
  Serial.print("JSON command to server: ");
  Serial.println(body);
  displayWrite("Sent request");

  // make the PUT request to the hub:
  httpClient.put(request, contentType, body);

  // read the status code and body of the response
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();

  Serial.print("Status code from server: ");
  Serial.println(statusCode);
  Serial.print("Server response: ");
  Serial.println(response);
  Serial.println();
  displayWrite("Server response: ");
  displayWrite(response);
}

void changeLightProperty(int whichProperty, int change) {
  // get the list of keys of the JSON var:
  JSONVar thisKey = lightState.keys();
  // get the value of the one to be changed:
  JSONVar value = lightState[thisKey[whichProperty]];
  // temporary variable for changed value:
  int newVal = 0;
  // each property has a different range, so you need to check
  // which property is being changed to set the range:
  switch (whichProperty) {
    case 0:   // on: true/false
      newVal = bool(!value);
      value = !value;
      break;
    case 1:  // bri: 0-255
      newVal = int(value) + change;
      value = constrain(newVal, 0, 255);
      break;
    case 2:  // hue: 0-65535, so take big steps (* 1000):
      newVal = int(value) + change * 1000;
      value = constrain(newVal, 0, 65535);
      break;
    case 3:   //sat: 0-255
      newVal = int(value) + change;
      value = constrain(newVal, 0, 255);
      break;
  }
  // print the key of the property being changed and its value:
  Serial.print(lightState.keys()[whichProperty]);
  Serial.print(" ");
  Serial.println(value);

  // print result to the display:
  String reading = "Setting: ";
  reading += String(scr[whichProperty]);
  reading += "\nValue: ";
  reading += String(newVal);
  displayWrite(reading);
}

void displayWrite(String message) {
  display.clearDisplay();
  display.setRotation(2);
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 11);
  display.println(message);
  display.display();
}
