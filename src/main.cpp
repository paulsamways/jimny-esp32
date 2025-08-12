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

const uint8_t CRANK_PULSE_POSITIONS[72] = {
  HIGH, LOW,
  LOW, LOW, LOW, LOW,
  HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW,
  LOW, LOW, LOW, LOW,
  HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW,
  LOW, LOW, LOW, LOW
};

const uint8_t CAM_PULSE_POSITIONS[144] = {
  HIGH, LOW,
  LOW, LOW, LOW, LOW,
  LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, HIGH, HIGH, LOW,
  LOW, LOW, LOW, LOW,
  HIGH, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, 
  LOW, LOW, LOW, HIGH,

  HIGH, LOW,
  LOW, LOW, LOW, LOW,
  LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW,
  LOW, HIGH, HIGH, LOW,
  LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, HIGH, 
  HIGH, LOW, LOW, LOW
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
  static ulong signalLastSet = 0;

  static int crankSignalPosition = SIGNAL_STOPPED;
  static int camSignalPosition = SIGNAL_STOPPED;


  if (injectorPeriod != injectorLastPeriod) {
    injectorLastPeriod = injectorPeriod;

    if (injectorPeriod > 0) {
      ulong rpms = (60 * 1000000) / (injectorPeriod * 2);
      Serial.print(rpms);
      Serial.println("RPM");
    }

    signalPeriod = (injectorPeriod * 2) / 72;
  }

  if (injectorPulseCount != injectorLastPulseCount) {
    if (injectorPulseCount % 2 == 0) { // Reset crank signal position after 2 injector pulses (1 revolution)
      crankSignalPosition = 0;
    }
    if (injectorPulseCount % 4 == 0) {  // Reset cam signal position after 4 injector pulses (2 revolutions)
      camSignalPosition = 0;
    }
    injectorLastPulseCount = injectorPulseCount;
  }

  ulong now = micros();

  if (now - signalLastSet >= signalPeriod) {
    if (crankSignalPosition > SIGNAL_STOPPED) {
      digitalWrite(PIN_OUTPUT_CRANK_ANGLE, CRANK_PULSE_POSITIONS[crankSignalPosition]);
      crankSignalPosition = crankSignalPosition < 71 ? crankSignalPosition + 1 : SIGNAL_STOPPED; // Reset after 36 counts
    }

    if (camSignalPosition > SIGNAL_STOPPED) {
      digitalWrite(PIN_OUTPUT_CAM_ANGLE, CAM_PULSE_POSITIONS[camSignalPosition]);
      camSignalPosition = camSignalPosition < 143 ? camSignalPosition + 1 : SIGNAL_STOPPED; // Reset after 72 counts
    }

    signalLastSet = now;
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