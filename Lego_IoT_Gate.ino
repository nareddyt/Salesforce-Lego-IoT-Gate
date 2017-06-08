#include "Timer.h"
#include "ESP8266WiFi.h"

Timer t;
// WiFi parameters to be configured
const char* ssid = "GS6";
const char* password = "legolego";
const char* host = "iot-gate-proxy.azurewebsites.net";
WiFiClient client;

// Constants
const int leftLEDPin = D2;
const int leftLightPin = D1;
const int middleLEDPin = D4;
const int middleLightPin = D3;
const int rightLEDPin = D6;
const int rightLightPin = D5;

// Enums
enum state {IN, OUT};
// Global vars
int timeStamp = 0;
int outCount = 1;
int inCount = 1;
enum state middleGateState = IN;

// Bit history of gates
int leftHistory = 0b1;
int middleHistory = 0b1;
int rightHistory = 0b1;
int leftTrig = false;
int middleTrig = false;
int rightTrig = false;

void setup() {
    // set the digital pin as output:
    pinMode(leftLEDPin, OUTPUT);
    pinMode(middleLEDPin, OUTPUT);
    pinMode(rightLEDPin, OUTPUT);
    pinMode(leftLightPin, INPUT_PULLUP);
    pinMode(middleLightPin, INPUT_PULLUP);
    pinMode(rightLightPin, INPUT_PULLUP);
    digitalWrite(leftLEDPin, HIGH);
    digitalWrite(middleLEDPin, HIGH);
    digitalWrite(rightLEDPin, LOW);
    // Start serial mode
    Serial.begin(9600);
    wifiSetup();
    t.every(10000, changeState);
}

void loop() {
    timeStamp = millis();
    t.update();
    
    // Read input from light sensors
    int leftLightInput = digitalRead(leftLightPin);
    leftLightInput ^= 1;
    int middleLightInput = digitalRead(middleLightPin);
    int rightLightInput = digitalRead(rightLightPin);
    // Check for spikes in values
    if (leftHistory == 0 && !leftTrig) {
      inCount += 1;
      leftTrig = true;
      Serial.println("Left gate triggered");
      
    }
    if (rightHistory == 0 && !rightTrig) {
      outCount += 1;
      rightTrig = true;
      Serial.println("Right gate triggered");
    }
    if (middleHistory == 0 && !middleTrig) {
      Serial.println("Middle gate triggered");
      if (middleGateState == IN) {
        inCount += 1;
      } else {
        outCount += 1;
      }
      middleTrig = true;
    }
    // Reset triggers
    if (leftHistory == 0b11111111111111111111 && leftTrig) {
      leftTrig = false;
//      Serial.println("Left gate reset");
    }
    if (rightHistory == 0b11111111111111111111 && rightTrig) {
      rightTrig = false;
//      Serial.println("Right gate reset");
    }
    if (middleHistory == 0b11111111111111111111 && middleTrig) {
//      Serial.println("Middle gate reset");
      middleTrig = false;
    }
    // Update histories
    leftHistory = leftHistory << 1;
    leftHistory &= 0b11111111111111111111;
    leftHistory |= leftLightInput;
    middleHistory = middleHistory << 1;
    middleHistory &= 0b11111111111111111111;;
    middleHistory |= middleLightInput;
    rightHistory = rightHistory << 1;
    rightHistory &= 0b11111111111111111111;;
    rightHistory |= rightLightInput;
}

void changeState() {
    pprint("IN", inCount);
    pprint("OUT", outCount);
    if (inCount >= outCount * 2 && middleGateState == OUT) {
      // Too many INs
      middleGateState = IN;
      digitalWrite(middleLEDPin, HIGH);
      Serial.println("Changing state to IN");
      sendEvent(2 , "IN" , inCount , outCount);
    } else if (outCount >= inCount * 2 && middleGateState == IN) {
      // Too many OUTs
      middleGateState = OUT;
      digitalWrite(middleLEDPin, LOW);
      Serial.println("Changing state to OUT");
      sendEvent(2 , "OUT" , inCount , outCount);
    } else {
      // Else gate will stay the same
      pprinte("Keeping state the same", middleGateState);
      //sendEvent(2 , (String)middleGateState, inCount , outCount);
      if (middleGateState == IN) {
        sendEvent(2 , "IN", inCount , outCount);
      } else {
        sendEvent(2 , "OUT", inCount , outCount);
      }
    }
    // Reseting counts
    inCount = 1;
    outCount = 1;
}

void pprint(String label, int val) {
    String toPrint = label + ": ";
    toPrint += val;
    Serial.println(toPrint);
}

void pprinte(String label, enum state val) {
    String toPrint = label + ": ";
    toPrint += val;
    Serial.println(toPrint);
}

void wifiSetup() {
  WiFi.begin(ssid, password);
  // while wifi not connected yet, print '.'
  // then after it connected, get out of the loop
  while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
  }
  //print a new line, then print WiFi connected and the IP address
  Serial.println("");
  Serial.println("WiFi connected");
  // Print the IP address
  Serial.println(WiFi.localIP());
}

void sendEvent(int gateID , String gateStatus , int riderCountIN , int riderCountOUT) {
    String json = buildJSON(gateID , gateStatus ,riderCountIN , riderCountOUT);
    Serial.println("This is the json file: " + json + "\n");
    Serial.println("Requesting POST\n");
     // Send request to the server:
     if (client.connect(host,80)) {

       String req = "";
       req += "POST /endpoint HTTP/1.1\r\n";
       req += "Host: iot-gate-proxy.azurewebsites.net\r\n";
       req += "Accept: */*\r\n";
       req += "Content-Type: application/json\r\n";
       req += "Content-Length: " + String(json.length());
       req += "\r\n\r\n";
       req += json + "\r\n";

//       Serial.println(req);
       client.print(req);
     }
//
//     unsigned long timeout = millis();
//  while (client.available() == 0) {
//    yield();
//    if (millis() - timeout > 2000) {
//      Serial.println(">>> Client Timeout !");
//      client.stop();
//      return;
//    }
//  }
//
//      // Read all the lines of the reply from server and print them to Serial
//      Serial.println("Response:");
//      while(client.available()){
//        yield();
//        String line = client.readStringUntil('\r');
//        Serial.print(line);
//      }
//    
//     Serial.println();
     
 }

String buildJSON(int gateID , String gateStatus , int riderCountIN , int riderCountOUT) {
  String json = "{\"gateID__c\":";
  json+=gateID;
  json+=",\"gateStatus__c\":";
  json+=gateStatus;
  json+=",\"riderCountIN__c\":";
  json+=riderCountIN;
  json+=",\"riderCountOut__c\":";
  json+=riderCountOUT;
  json+="}";
  return json;
}
