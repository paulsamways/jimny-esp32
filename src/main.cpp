#include <Arduino.h>

#define PIN_OUTPUT_A LED_BUILTIN
#define PIN_OUTPUT_B D2
#define PIN_INPUT D3

void setup() {
  pinMode(PIN_OUTPUT_A, OUTPUT);
  pinMode(PIN_OUTPUT_B, OUTPUT);
  pinMode(PIN_INPUT, INPUT_PULLDOWN);

  Serial.begin(9600);
}

int b1 = 0;

bool b2_enabled = false;
int b2 = 0;

int input_high_count = 0;
int input_previous = LOW;

void loop() {

  if (!b2_enabled)
  {
    int input_current = digitalRead(PIN_INPUT);

    if (input_current != input_previous)
    {
      input_previous = input_current;
      if (input_current == HIGH)
      {
        input_high_count++;
      }

      if (input_high_count >= 4)
      {
        b2_enabled = true;
        b2 = 0;
        input_high_count = 0;
      }
    }
  }

  if (b1 < 36)
  {
    digitalWrite(PIN_OUTPUT_A, HIGH);
    Serial.print(".");
  }
  else
  {
    Serial.print(" ");
  }

  b1 = b1 < 39 ? b1 + 1 : 0;

  if (b2_enabled)
  {
    digitalWrite(PIN_OUTPUT_B, HIGH);

    if (b2 < 12)
    {
      b2++;
      Serial.print("-");
    }
    else
    {
      b2_enabled = false;
      b2 = 0;
    }
  }
  
  delay(50);
  digitalWrite(PIN_OUTPUT_A, LOW);
  digitalWrite(PIN_OUTPUT_B, LOW);
  

  delay(100);
}