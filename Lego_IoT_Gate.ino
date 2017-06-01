#include "Timer.h"

Timer t;

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
  digitalWrite(middleLEDPin, LOW);
  digitalWrite(rightLEDPin, LOW);

  // Start serial mode
  Serial.begin(9600);

  t.every(5000, changeState);
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
    Serial.println("Left gate reset");
  }
  if (rightHistory == 0b11111111111111111111 && rightTrig) {
    rightTrig = false;
    Serial.println("Right gate reset");
  }
  if (middleHistory == 0b11111111111111111111 && middleTrig) {
    Serial.println("Middle gate reset");
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
  } else if (outCount >= inCount * 2 && middleGateState == IN) {
    // Too many OUTs
    middleGateState = OUT;
    digitalWrite(middleLEDPin, LOW);
    Serial.println("Changing state to OUT");
  } else {
    // Else gate will stay the same
    pprinte("Keeping state the same", middleGateState);
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


