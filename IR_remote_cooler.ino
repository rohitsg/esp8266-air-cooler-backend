#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include "secret.h"

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);

struct Cooler {
  bool onoff;
  bool fan;
  String fanSpeed;
  bool cool;
  bool swing;
  bool mosquitto;
};
Cooler prevCooler = {false, false, "off", false, false, false};

char fanSpeedArr[4][6] = {"off", "high", "mid", "low"};
bool onoff = false;
bool fan = false;
String fanSpeed = "off";
bool cool = false;
bool swing = false;
bool mosquitto = false;
bool synced = true;


void resetControls() {
  prevCooler.onoff = Firebase.getBool("/cooler/onoff");
  prevCooler.fan = Firebase.getBool("/cooler/fan");
  prevCooler.fanSpeed = Firebase.getBool("/cooler/fanSpeed");
  prevCooler.cool = Firebase.getBool("/cooler/cool");
  prevCooler.swing = Firebase.getBool("/cooler/swing");
  prevCooler.mosquitto = Firebase.getBool("/cooler/mosquitto");
}

void connectToWiFiFirebase() {

  WiFi.disconnect();
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting: ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Wifi connected with IP address: ");
  Serial.println(WiFi.localIP());
  resetControls();
}

void setup() {

  Serial.begin(115200);

  while (!Serial) {
    delay(50);
  }

  connectToWiFiFirebase();
  irsend.begin();
}


void loop() {

  if ( WiFi.status() != WL_CONNECTED ) {
    Serial.println("In not connected...");
    connectToWiFiFirebase();
    irsend.begin();
    return;
  }

  synced = Firebase.getBool("/cooler/synced");
  if (synced) {
    Serial.println("Already synced...");
    delay(1000);
    return; // controls are already synced, so no operation is performed
  }

  onoff = Firebase.getBool("/cooler/onoff");
  if (!onoff) {
    Serial.println("Turning off cooler ...");

    prevCooler.onoff = false;
    prevCooler.fan = false;
    irsend.sendSymphony(0xD81); //onoff
    Firebase.setBool("/cooler/fan", false);
    delay(4000);
    Firebase.setBool("/cooler/synced", true);
    return;
  }

//  if (onoff) {
//    resetControls();
//  }
 
  fanSpeed = Firebase.getString("/cooler/fanSpeed");
  if (onoff != prevCooler.onoff && onoff == true) {

    prevCooler.onoff = onoff;
    prevCooler.fan = true;
    Serial.println("Executing on/off...");
    irsend.sendSymphony(0xD81);
    if (fanSpeed != "off") {
      Firebase.setBool("/cooler/fan", true);
      delay(1000);
    }
    delay(4000);
    Firebase.setBool("/cooler/synced", true);
    return;
  }

  if (fanSpeed != prevCooler.fanSpeed) {
    prevCooler.fanSpeed = fanSpeed;
    Serial.println("Executing fanSpeed off/high/med/low...");
    irsend.sendSymphony(0xD82); //fan
    Firebase.setBool("/cooler/fan", fanSpeed == "off" ? false : true);
    delay(4000);
    Firebase.setBool("/cooler/synced", true);
    return;
  }

  cool = Firebase.getBool("/cooler/cool");
  if (cool != prevCooler.cool) {
    prevCooler.cool = cool;
    Serial.println("Executing cool on/off...");
    irsend.sendSymphony(0xD84);
    delay(4000);
    Firebase.setBool("/cooler/synced", true);
    return;
  }

  swing = Firebase.getBool("/cooler/swing");
  if (swing != prevCooler.swing) {
    prevCooler.swing = swing;
    Serial.println("Executing swing on/off...");
    irsend.sendSymphony(0xD88);
    Firebase.setBool("/cooler/synced", true);
    return;
  }

  mosquitto = Firebase.getBool("/cooler/mosquitto");
  if (mosquitto != prevCooler.mosquitto) {
    prevCooler.mosquitto = mosquitto;
    Serial.println("Executing mosquitto on/off...");
    irsend.sendSymphony(0xDC3);
    delay(4000);
    Firebase.setBool("/cooler/synced", true);
    return;
  }

  Serial.println("No operations...");
  delay(1000);

  if (Firebase.failed()) {
    Serial.println("Firebase failed...");
    return;
  }
}
