/*
  Open Led Bike Light

  Copyright (c) 2021 Stefano Guglielmetti
  This software is released under the MIT License.
  https://opensource.org/licenses/MIT

  This project is based on the work of:

  ATtiny85 sleep mode and wake on pin change
  Author: Davide Gariselli
  https://github.com/DaveCalaway/ATtiny/tree/master/Examples/AT85_sleep_interrupt
  Original version made by Nick Gammon https://goo.gl/62b0Im
*/

#include <PinChangeInterrupt.h>  // PinChangeInterrupt ( attachPCINT ) Library 1.2.4 https://goo.gl/WhlCwl
#include <avr/power.h>  // Power management https://goo.gl/58Vdvv
#include <avr/sleep.h>  // Sleep Modes https://goo.gl/WJUszs

#define LED 0
#define SWITCH 4
#define MAX_STATE 9  // Remember to update this value when adding new states

int state = 0;
int ledPower = 0;

unsigned long last_button_press_millis = millis();
unsigned int button_press_duration = 0;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(SWITCH, INPUT_PULLUP);  // using the internal pull-up
  digitalWrite(LED, LOW);         // ensure led is off at startup
  goToSleep();                    // starts in sleep mode
}

void loop() {
  checkButtonState();

  if (state == 0) {
    // Wake up only if the button has been pressed for more than 500ms
    // to avoid accidental start-ups
    if (digitalRead(SWITCH) == LOW && button_press_duration >= 500) {
      startupAnimation();
      state = 1;
      button_press_duration = 0;
    } else if (digitalRead(SWITCH) == HIGH && button_press_duration < 500) {
      goToSleep();
    }
  } else {
    if (digitalRead(SWITCH) == HIGH) {
      // a single, quick click switches to the next state
      // the click must be longer than 10ms for debouncing
      if (button_press_duration > 10) {
        goToNextState();
        button_press_duration = 0;
      }
    } else {
      // 2 seconds long press puts the light in sleep mode
      if (button_press_duration > 2000) {
        button_press_duration = 0;
        blink(4, 50);
        delay(1000);
        goToSleep();
      }
    }
  }

  switch (state) {
    case 0:
      // this is a special state used only when waking up
      ledPower = 0;
      break;
    case 1:
      // 100% power
      ledPower = 255;
      break;
    case 2:
      // 50% power
      ledPower = 128;
      break;
    case 3:
      // 2 seconds ramp up (sawtooth) cycle
      ledPower = map(millis() % 2000, 0, 2000, 0, 255);
      break;
    case 4:
      // half of the sine wave (sine blink?)
      ledPower = constrain(128 * sin((millis() / 3000.0) * 2.0 * PI), 0, 255);
      break;
    case 5:
      // full syne wave cycle
      ledPower = 128.0 + 128 * sin((millis() / 2000.0) * 2.0 * PI);
      break;
    case 6:
      // long blink (1000ms)
      blink(1000.0);
      break;
    case 7:
      // fast blink (500ms)
      blink(500.0);
      break;
    case 8:
      // seizure attack blink (100ms)
      blink(100.0);
      break;
    case 9:
      // random power effect (looks like a broken light)
      ledPower = random(20, 255);
      delay(10);
      break;
      // REMEMBER TO UPDATE MAX_STATE WHEN ADDING NEW STATES
  }

  analogWrite(LED, ledPower);
  delay(10);
}

void wakeUpNow() {
  resetTimers();
  state = 0;
  detachPCINT(digitalPinToPinChangeInterrupt(
      SWITCH));  // detaches wakeUpNow() from the interrupt
}

void resetTimers() {
  last_button_press_millis = millis();
  button_press_duration = 0;
}

void checkButtonState() {
  if (digitalRead(SWITCH) == LOW) {
    button_press_duration = millis() - last_button_press_millis;
  } else {
    last_button_press_millis = millis();
  }
}

void goToNextState() {
  state++;
  // never go back to 0, as 0 is a special post-power-up state
  // used to prevent accidental power-ups
  if (state > MAX_STATE) state = 1;
}

void goToSleep() {
  attachPCINT(digitalPinToPinChangeInterrupt(SWITCH), wakeUpNow, FALLING);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // <avr/sleep.h>
  ADCSRA = 0;                           // turn off ADC
  power_all_disable();  // power off ADC, Timer 0 and 1, serial interface
                        // <avr/power.h>
  sleep_enable();       // <avr/sleep.h>
  sleep_cpu();          // <avr/sleep.h>
  //---------------  THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  //---------------
  sleep_disable();     // <avr/sleep.h>
  power_all_enable();  // power everything back on <avr/power.h>
}

void startupAnimation() {
  for (int i = 0; i <= 255; i++) {
    analogWrite(LED, i);
    delay(5);
  }
}

// Blinks the LED for the given number of times for the given duration
// this is a blocking function, and it's used before the sleep mode
void blink(int loops, int milliseconds) {
  for (int i = 0; i < loops; i++) {
    digitalWrite(LED, HIGH);
    delay(milliseconds);
    digitalWrite(LED, LOW);
    delay(milliseconds);
  }
}

// Blinks the led undefinitely, for the given duration
// this is a non blocking function, and it's used for the blink modes
void blink(float duration) {
  if (sin((millis() / duration) * 2.0 * PI) > 0) {
    ledPower = 0;
  } else {
    ledPower = 255;
  }
}
