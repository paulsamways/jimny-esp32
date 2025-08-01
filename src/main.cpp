#include <Arduino.h>

#define PIN_OUTPUT_A LED_BUILTIN
#define PIN_OUTPUT_B D2
#define PIN_INPUT D3

void setup() {
  pinMode(PIN_OUTPUT_A, OUTPUT);
  pinMode(PIN_OUTPUT_B, OUTPUT);
  pinMode(PIN_INPUT, INPUT_PULLDOWN);

  digitalWrite(PIN_OUTPUT_A, LOW);
  digitalWrite(PIN_OUTPUT_B, LOW);

  Serial.begin(9600);
}

int b1 = 0;

bool b2_enabled = false;
int b2 = 0;

int input_high_count = 0;
unsigned long read_input_after = 0;
int input_previous = LOW;

unsigned long write_high_after = 0;
unsigned long write_low_after = millis();

void loop() {

  unsigned long current_millis = millis();

  if (current_millis >= read_input_after && !b2_enabled)
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
    read_input_after = current_millis + 10;
  }

  if (write_high_after > 0 && current_millis >= write_high_after)
  {
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

    write_high_after = 0;
    write_low_after = current_millis + 50;
  }
  
  if (write_low_after > 0 && current_millis >= write_low_after)
  {
    digitalWrite(PIN_OUTPUT_A, LOW);
    digitalWrite(PIN_OUTPUT_B, LOW);

    write_low_after = 0;
    write_high_after = current_millis + 100; // Set next high after 1 second
  }
}