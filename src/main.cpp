#include <Arduino.h>
#include <APA102.h>

// Define button.
const uint8_t buttonPin = 8;

// Define the LED strip.
const uint8_t dataPin = 16;
const uint8_t clockPin = 15;
const uint16_t ledCount = 5;
APA102<dataPin, clockPin> ledStrip;

const float acceleration = 0.000009; // pixels per millisecond squared.
const float terminalVelocity = 0.07; // pixels per millisecond.

typedef struct State_struct (*ModeFn)(struct State_struct current_state,
                                      unsigned long current_time,
                                      uint8_t button_press);

typedef struct State_struct {
  float pos;                  // The current position of our bouncing blue light.
  float speed;                // The current speed of our bouncing blue light
  bool terminal;              // Has the bouncing blue light hit terminal velocity?
  unsigned long last_time;    // The last time update was called.
  unsigned long started_at;   // The time this mode started.
  ModeFn update;              // The current function used to update state.
} State;

State IdleMode(State current_state, unsigned long current_time, uint8_t button_press);
State AccelerateMode(State current_state, unsigned long current_time, uint8_t button_press);
State FrictionMode(State current_state, unsigned long current_time, uint8_t button_press);
State ExplodeMode(State current_state, unsigned long current_time, uint8_t button_press);

/** IdleMode manipulates our bouncing blue light when at rest.*/
State IdleMode(State current_state, unsigned long current_time, uint8_t button_press) {
  Serial.println("IdleMode");

  State new_state;
  new_state.pos = current_state.pos;
  new_state.speed = 0.0;
  new_state.terminal = false;
  new_state.last_time = current_time;

  if (button_press) {
    new_state.started_at = current_time;
    new_state.update = &AccelerateMode;
    return new_state;
  }

  new_state.update = current_state.update;
  return new_state;
}

State updatePosition(State current_state, unsigned long current_time, int direction) {
  unsigned long deltaT = current_time - current_state.last_time;

  State new_state;
  new_state.speed = current_state.speed + (direction * acceleration * deltaT);
  new_state.pos = (current_state.pos + current_state.speed + (0.5 * direction * acceleration * deltaT * deltaT));
  new_state.terminal = (new_state.speed > terminalVelocity);
  new_state.last_time = current_time;
  new_state.started_at = current_state.started_at;
  new_state.update = current_state.update;

  return new_state;
}

/** AccelerateMode defines how to speed up our bouncing blue light. */
State AccelerateMode(State current_state, unsigned long current_time, uint8_t button_press) {
  Serial.println("AccelerateMode");

  // Update state.
  State new_state = updatePosition(current_state, current_time, 1.0);

  Serial.print("Speed: ");
  Serial.println(new_state.speed);

  if (new_state.terminal) {
    new_state.started_at = current_time;
    new_state.update = &ExplodeMode;
    return new_state;
  }

  if (!button_press) {
    new_state.started_at = current_time;
    new_state.update = &FrictionMode;
    return new_state;
  }

  return new_state;
}

/** FictionMode describes how to slow the bouncing blue light when we encounter friction. */
State FrictionMode(State current_state, unsigned long current_time, uint8_t button_press) {
  Serial.println("FrictionMode");

  State new_state = updatePosition(current_state, current_time, -1.0);

  if (new_state.speed < 0.0) {
    new_state.started_at = current_time;
    new_state.update = &IdleMode;
    return new_state;
  }

  if (button_press) {
    new_state.started_at = current_time;
    new_state.update = &AccelerateMode;
    return new_state;
  }

  return new_state;
}

/** ExplodeMode augments our bouncing blue light when we hit a terminal velocity. */
State ExplodeMode(State current_state, unsigned long current_time, uint8_t button_press) {
  Serial.println("ExplodeMode");

  unsigned long deltaT = current_time - current_state.started_at;
  if (deltaT > 1500) {
    current_state.update = &IdleMode;
    return current_state;
  }

  return current_state;
}

State state;  // The current state of our Arduino/LED strip.

/** setup is exectued once before the loop is started. */
void setup() {
  Serial.begin(9600);

  pinMode(buttonPin, INPUT);

  // Initialise the default resting state.
  state.pos = 0;
  state.speed = 0.0;
  state.terminal = false;
  state.last_time = millis();
  state.started_at = millis();
  state.update = &IdleMode;
}

void render(State current_state) {
  // render.
  rgb_color colours[ledCount] = (rgb_color){0, 0, 0};
  if (state.terminal) {
    for (uint8_t i = 0; i < ledCount; i++) {
      colours[i] = (rgb_color){220, 127, 31};
    }
    ledStrip.write(colours, ledCount, 8);
  } else {
    colours[(uint8_t) (state.pos) %5] = (rgb_color){29, 106, 177};
    ledStrip.write(colours, ledCount, 4);
  }
}

/** loop iterates over and over again till the microcontroller is reset.*/
void loop() {
  unsigned long t = millis();
  state = state.update(state, t, digitalRead(buttonPin));
  render(state);
}
