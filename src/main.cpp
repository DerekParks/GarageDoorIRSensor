#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include "ExpFilter.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <MQTT.h>
#include "topics.h"

#define OPEN_DOOR_ALERT 600000            // Send message to alert topic after 10 min
#define MOVING_DOOR_MILLS 6000            // How long does it take the door to open/close
#define FORCE_REFRESH_EVERY_MILLS 600000  // How often should we refresh mqtt (useful if we don't want to deal with mqtt retain but still want to get state updated after an HA reboot)
#define CUT_OFF_VOLTAGE 0.9               // What voltage from the IR sensor should be the cuttoff between open and closed


// WebSerial can be useful for in situ debuging 

//#define USE_WEBSERIAL false

#ifdef USE_WEBSERIAL

#include <WebSerial.h>
#define S WebSerial

AsyncWebServer server(80);

#else

#define S Serial

#endif

// MQTT setup
WiFiClient net;
MQTTClient client(500);

// Turn defines into strings
#define XSTR(x) #x
#define STR(x) XSTR(x)

// Wifi & MQTT creds are read in through build flags. See platofrmio.ini. Print them out when building
#pragma message STR(DP_WIFI_SSID)
#pragma message STR(DP_WIFI_PASS)
#pragma message STR(DP_MQTT_HOST)
#pragma message STR(DP_MQTT_USER)
#pragma message STR(DP_MQTT_PASS)

// Wifi & MQTT creds
const char* ssid = STR(DP_WIFI_SSID);
const char* password = STR(DP_WIFI_PASS);

const char* mqtt_host = STR(DP_MQTT_HOST);
const char* mqtt_user = STR(DP_MQTT_USER);
const char* mqtt_pass = STR(DP_MQTT_PASS);

#define EXP_FILTER_TAU 3000 // Millis decay for exponential filter  

Adafruit_ADS1115 ads;
ExpFilter filter3(EXP_FILTER_TAU);
ExpFilter filter2(EXP_FILTER_TAU);

unsigned long tLast, tNext;      // temp times
unsigned long tLastStateChange1; // last time state 1 changed
unsigned long tLastStateChange2; // last time state 2 changed
unsigned long tLastAvilUpdate;   // last time MQTT was updated
unsigned long tLastAlertUpdate1 = 0; // last time we alerted on state 1
unsigned long tLastAlertUpdate2 = 0; // last time we alerted on state 2

// Temp variables
int16_t adc3;
float volts3, filtered3;

int16_t adc2;
float volts2, filtered2;

// Use to limit logging a bit
int16_t measureCount = 0;

// Possible door states
enum DoorState {closed, opening, open, closing};

// Assume closed to start but shouldn't matter
DoorState s1 = closed;
DoorState s2 = closed;


// Convert state to char*
inline const char* toString(DoorState state)
{
    switch (state)
    {
        case closed:   return "closed";
        case opening:  return "opening";
        case open:     return "open";
        case closing:  return "closing";
        default:       return "[Unknown state]";
    }
}

// Reconnect to MQTT
void connect() {
  S.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    S.print(".");
    delay(1000);
  }

  S.print("\nconnecting...");
  while (!client.connect(STR(MQTT_CLIENT), mqtt_user, mqtt_pass)) {
    Serial.print(".");
    delay(1000);
  }

  S.println("\nconnected!");
}

// Handle possible state change if we have a high voltage
DoorState high_voltage(
  unsigned long tCurrent, 
  unsigned long tLastStateChange,
  DoorState s
  ) {
  switch(s) {
      case closed: return closed;
      case opening:
      case open:
        return closing;
      case closing:
        if((tCurrent - tLastStateChange) > MOVING_DOOR_MILLS) {
          return closed;
        } else {
          return closing;
        }
  }
  return s;
}

// Handle possible state change if we have a low voltage
DoorState low_voltage(
  unsigned long tCurrent, 
  unsigned long tLastStateChange,
  DoorState s
) {
  switch(s) {
      case open: return open;
      case closing:
      case closed:
        return opening;
      case opening:
        if((tCurrent - tLastStateChange) > MOVING_DOOR_MILLS) {
          return open;
        } else {
          return opening;
        }
  }
  return s;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  //WIFI setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //MQTT setup
  client.begin(mqtt_host, net);
  connect();

#ifdef USE_WEBSERIAL
  WebSerialPro.begin(&server);
  server.begin();
#else
  
#endif

  // Say we are Offline until we have meassure state
  client.publish(STR(AVIL_TOPIC1), "Offline");
  client.publish(STR(AVIL_TOPIC2), "Offline");

  // Setup ADC
  Wire.begin(0, 2);
  delay(10);
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1000);
  }
  
  tLast = millis();

  //Tell MQTT we are up and running and current state
  client.publish(STR(AVIL_TOPIC1), "Online");
  client.publish(STR(AVIL_TOPIC2), "Online");

  tLastAvilUpdate = tLast;
  client.publish(STR(STATE_TOPIC1), toString(s1));
  client.publish(STR(STATE_TOPIC2), toString(s2));
}

void loop() {
  client.loop(); // mqtt stay connected

  //Read voltages from ADC
  adc3 = ads.readADC_SingleEnded(3);  // ADC port 3 is connected to state 1
  volts3 = ads.computeVolts(adc3);

  adc2 = ads.readADC_SingleEnded(2);  // ADC port 2 is connected to state 2
  volts2 = ads.computeVolts(adc2);

  tNext = millis(); 

  unsigned long dt = (tNext > tLast) ? tNext - tLast : tLast - tNext; // Find time since last update

  // Exp filter voltages
  filtered3 = filter3.add(volts3, dt);  
  filtered2 = filter2.add(volts2, dt);

  if (measureCount == 100) {

    if (!client.connected()) {
      connect();
    }

    // Check for state 1 changes
    DoorState s1New = s1;
    if (filtered3 <= CUT_OFF_VOLTAGE) {
      s1New = low_voltage(tNext, tLastStateChange1, s1);
    } else {
      s1New = high_voltage(tNext, tLastStateChange1, s1);
    }
    
    // Handle state 1 changes
    if(s1 != s1New) {
      s1 = s1New;
      tLastStateChange1 = tNext;
      tLastAlertUpdate1 = tNext;
      client.publish(STR(STATE_TOPIC1), toString(s1));
      S.print("Changing state:"); S.println(toString(s1));
    }

    // Check for state 2 changes
    DoorState s2New = s2;
    if (filtered2 <= CUT_OFF_VOLTAGE) {
      s2New = low_voltage(tNext, tLastStateChange2, s2);
    } else {
      s2New = high_voltage(tNext, tLastStateChange2, s2);
    }
    
    // Handle state 2 changes
    if(s2 != s2New) {
      s2 = s2New;
      tLastStateChange2 = tNext;
      tLastAlertUpdate2 = tNext;
      client.publish(STR(STATE_TOPIC2), toString(s2));
      S.print("Changing state2:"); S.println(toString(s2));
    }

    // Check if we need to alert because state 1 is open for too long
    if(s1 == open && (tNext - tLastAlertUpdate1) > OPEN_DOOR_ALERT) {
      client.publish(STR(ALERT_TOPIC1), toString(s1));
      tLastAlertUpdate1 = tNext;
      S.print("Alert1:"); S.println(toString(s1));
    }

    // Check if we need to alert because state 2 is open for too long
    if(s2 == open && (tNext - tLastAlertUpdate2) > OPEN_DOOR_ALERT) {
      client.publish(STR(ALERT_TOPIC2), toString(s2));
      tLastAlertUpdate2 = tNext;
      S.print("Alert2:"); S.println(toString(s2));
    }

    // Logging
    S.print("AIN3: "); S.print(adc3); S.print("  "); S.print(volts3); 
    S.print("V "); S.print(filtered3); S.println("V"); 

    S.print("AIN2: "); S.print(adc2); S.print("  "); S.print(volts2); 
    S.print("V "); S.print(filtered2); S.println("V"); 

    // Check if refresh is needed
    if((tNext - tLastAvilUpdate) > FORCE_REFRESH_EVERY_MILLS) {
      client.publish(STR(AVIL_TOPIC1), "Online");
      client.publish(STR(AVIL_TOPIC2), "Online");
      client.publish(STR(STATE_TOPIC1), toString(s1));
      client.publish(STR(STATE_TOPIC2), toString(s2));

      tLastAvilUpdate = tNext;
    }

    measureCount = 0;
  }

  tLast = tNext;
  measureCount++;
}