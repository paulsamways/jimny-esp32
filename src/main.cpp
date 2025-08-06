#include <Arduino.h>

#define PIN_OUTPUT_CRANK_ANGLE D2  // Change to your actual GPIO
#define PIN_OUTPUT_CAM_ANGLE D3    // Change to your actual GPIO
#define PIN_INPUT_INJECTOR_PULSE D4 // Change to your actual GPIO

const int SIGNAL_STOPPED = -1;

volatile unsigned long lastPulseTime = 0;
volatile unsigned long pulsePeriod = 0;
volatile unsigned long pulseCount = 0;

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR injectorPulseISR();

const int CRANK_PULSE_POSITIONS[36] = {
  HIGH,
  LOW, LOW,
  HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH,
  LOW, HIGH, LOW,
  HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH,
  LOW, LOW
};

const int CAM_PULSE_POSITIONS[36] = {
  LOW, LOW, LOW, LOW, LOW, HIGH, LOW, LOW, LOW, LOW, LOW, LOW,
  LOW, LOW, LOW, LOW, LOW, HIGH, LOW, LOW, LOW, LOW, LOW, LOW,
  LOW, LOW, LOW, LOW, LOW, HIGH, LOW, LOW, LOW, LOW, LOW, LOW
};

void setup() {
  Serial.begin(115200);

  pinMode(PIN_INPUT_INJECTOR_PULSE, INPUT);
  pinMode(PIN_OUTPUT_CRANK_ANGLE, OUTPUT);
  pinMode(PIN_OUTPUT_CAM_ANGLE, OUTPUT);

  digitalWrite(PIN_OUTPUT_CRANK_ANGLE, LOW);
  digitalWrite(PIN_OUTPUT_CAM_ANGLE, LOW);

  attachInterrupt(digitalPinToInterrupt(PIN_INPUT_INJECTOR_PULSE), injectorPulseISR, CHANGE);
}

void loop() {
  static long injectorLastPeriod = 0;
  static ulong injectorLastPulseCount = 0;

  portENTER_CRITICAL(&timerMux);
  unsigned long injectorPeriod = pulsePeriod;
  unsigned long injectorPulseCount = pulseCount;
  portEXIT_CRITICAL(&timerMux);

  static long signalPeriod = 0;
  static ulong signalLastHigh = 0;
  static int signalPosition = SIGNAL_STOPPED;

  if (injectorPeriod != injectorLastPeriod) {
    injectorLastPeriod = injectorPeriod;
    signalPeriod = (injectorPeriod * 2) / 36;
  }

  if (injectorPulseCount != injectorLastPulseCount && injectorPulseCount % 2 == 0) {
    signalPosition = 0;
    injectorLastPulseCount = injectorPulseCount;
  }

  ulong now = micros();

  if (signalPosition > SIGNAL_STOPPED && now - signalLastHigh >= signalPeriod) {
    digitalWrite(PIN_OUTPUT_CRANK_ANGLE, CRANK_PULSE_POSITIONS[signalPosition]);
    digitalWrite(PIN_OUTPUT_CAM_ANGLE, CAM_PULSE_POSITIONS[signalPosition]);

    delayMicroseconds(signalPeriod / 4);

    digitalWrite(PIN_OUTPUT_CRANK_ANGLE, LOW);
    digitalWrite(PIN_OUTPUT_CAM_ANGLE, LOW);

    signalLastHigh = now;
    signalPosition = signalPosition < 35 ? signalPosition + 1 : SIGNAL_STOPPED; // Reset after 36 counts
  }
}

void IRAM_ATTR injectorPulseISR() {
  static bool lastState = LOW;
  bool state = digitalRead(PIN_INPUT_INJECTOR_PULSE);
  if (state == HIGH && lastState == LOW) { // Rising edge
    unsigned long now = micros();
    if (lastPulseTime != 0) {
      pulsePeriod = now - lastPulseTime;
      pulseCount++;
    }
    lastPulseTime = now;
  }
  lastState = state;
}