#include "pitches.h"

#define ARRAY_LEN(arr) sizeof(arr) / sizeof(*arr)

#define SIMON_MAX_SEQUENCE_LENGTH 255
#define SIMON_PLAY_SPEED 250
#define SIMON_NEXT_PAUSE 1000
#define SIMON_DISPLAY_RATE 1
#define SIMON_SOUND_START 1
#define SIMON_SOUND_FAIL 2
#define SIMON_NO_INPUT -1
#define SIMON_POLL_INTERVAL 100
#define SIMON_BTN_PRESS 0
#define SIMON_BTN_RELEASE 1
#define SIMON_BUZZER_PIN 6

struct Note {
  unsigned int note;
  double duration;
};

struct Event {
  uint8_t btn;
  uint8_t type;
};

// Resets the game state
void reset();

// Initializes an array of pins
void setPins(uint8_t pins[], int length, uint8_t mode, uint8_t val = -1);

// Sets the current playing sound
void setSound(int id);

// Display current score
void displayScore();

// Plays the current sound
bool playSound();

// Plays a melody
bool playNotes(Note notes[], int length, int bpm = 120);

// Plays the current sequence
bool playSequence();

// Checks player input
bool assertSequence(Event* event);

// Gets the next available event
bool pollEvent(Event* event);

// Gets the button being pressed, if any
int readInput();

// BCD numbers
uint8_t numbers[][4] = {
  {0, 0, 0, 0},
  {0, 0, 0, 1},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 1, 0, 0},
  {0, 1, 0, 1},
  {0, 1, 1, 0},
  {0, 1, 1, 1},
  {1, 0, 0, 0},
  {1, 0, 0, 1}
};

// BCD decoder pins
uint8_t decoderPins[] = {10, 9, 8, 7};

// Pins connected to the tens/units displays
uint8_t digitsPins[] = {12, 11};

// Pins connected to the LEDs/pushbuttons
uint8_t pins[] = {2, 3, 4, 5};
uint8_t pinsNotes[] = {NOTE_C3, NOTE_D3, NOTE_E3, NOTE_F3};

// Sounds
Note startSound[] = { 
  {NOTE_C3, 16}, {NOTE_D3, 16}, {NOTE_F3, 4} 
};

Note failSound[] = { 
  {NOTE_F3, 16}, {NOTE_E3, 16}, {NOTE_D3, 16}, {NOTE_A2, 4} 
};

// Game state
uint8_t sequence[SIMON_MAX_SEQUENCE_LENGTH];
uint8_t sequenceLength;
uint8_t asserts;
uint8_t score;
uint8_t sound;

bool isGameOver;
bool isSequencePlaying;
bool isPlayerTurn;

// Loop state
unsigned long noteDuration;
bool isLedOn;
int currentNote;
int currentLed;

unsigned long lastAssertTime = 0;
unsigned long currentTime = 0;

void setup() { 
  randomSeed(analogRead(0));
  Serial.begin(9600);
  
  reset();
  setPins(decoderPins, ARRAY_LEN(decoderPins), OUTPUT);
  setPins(digitsPins, ARRAY_LEN(digitsPins), OUTPUT);
}

void loop() {
  Event event;
  bool hasEvent = false;

  currentTime = millis();
  hasEvent = pollEvent(&event);
  
  displayScore();

  if (isGameOver && hasEvent && event.type == SIMON_BTN_RELEASE) {
    reset();
  }

  if (sound && !playSound()) {
    sound = 0;
  }
  
  if (!isGameOver 
    && !isPlayerTurn 
    && !isSequencePlaying 
    && (currentTime - lastAssertTime) >= SIMON_NEXT_PAUSE
  ) {
    sequence[sequenceLength++] = random(0, ARRAY_LEN(pins));
    setPins(pins, ARRAY_LEN(pins), OUTPUT, HIGH);
    isSequencePlaying = true;
    asserts = 0;
  }

  if (isSequencePlaying) {
    isSequencePlaying = playSequence();

    if (!isSequencePlaying) {
      setPins(pins, ARRAY_LEN(pins), INPUT_PULLUP);
      isPlayerTurn = true;
    }
  }

  if (isPlayerTurn && hasEvent) {
    isPlayerTurn = assertSequence(&event);

    if (!isPlayerTurn) {
      if (asserts == sequenceLength) {
        score++;
        lastAssertTime = currentTime;
      } else {
        isGameOver = true;
      }
    }
  }
}

void reset() {
  isGameOver = false;
  isSequencePlaying = false;
  isPlayerTurn = false;
  sound = SIMON_SOUND_START;
  asserts = 0;
  sequenceLength = 0;
  score = 0;
  isLedOn = false;
  currentLed = 0;
  lastAssertTime = currentTime;
  
  setPins(pins, ARRAY_LEN(pins), INPUT_PULLUP);
  setSound(SIMON_SOUND_START);
}

void setPins(uint8_t pins[], int length, uint8_t mode, uint8_t val = -1) {
  int i;

  for (i = 0; i < length; i++) {
    pinMode(pins[i], mode);

    if (mode == OUTPUT && val != -1) {
      digitalWrite(pins[i], val);
    }
  }
}

void setSound(int id) {
  sound = id;
  noteDuration = 0;
  currentNote = 0;
}

void displayScore() {
  static unsigned long previousTime = 0;
  static uint8_t digit = 0; // 0 = units, 1 = tens... etc.
  int i;
  int rest = score;
  int number;

  if (currentTime - previousTime < SIMON_DISPLAY_RATE) {
    return;
  }

  previousTime = currentTime;

  // Get the number for the current digit
  for (i = 0; i <= digit; i++) {
    number = rest % 10;
    rest = rest / 10;
  }

  // Turn off displays
  for (i = 0; i < ARRAY_LEN(digitsPins); i++) {
    digitalWrite(digitsPins[i], LOW);
  }

  // Update number
  for (i = 0; i < ARRAY_LEN(decoderPins); i++) {
    digitalWrite(decoderPins[i], numbers[number][i]);
  }

  // Turn on display for the current digit (leading zeros are not displayed)
  if (number != 0 || digit == 0) {
    digitalWrite(digitsPins[digit], HIGH);
  }

  // Alternate digits
  digit = (digit + 1) % ARRAY_LEN(digitsPins);
}

bool playSound() {
  if (sound == SIMON_SOUND_START) {
    return playNotes(startSound, ARRAY_LEN(startSound));
  }
  
  if (sound == SIMON_SOUND_FAIL) {
    return playNotes(failSound, ARRAY_LEN(failSound));
  }

  return false;
}

bool playNotes(Note notes[], int length, int bpm = 120) {
  static unsigned long previousTime = 0;

  if (currentNote >= length) {
    currentNote = 0;
    
    return false;
  }

  if (currentTime - previousTime >= noteDuration) {
    previousTime = currentTime;    
    noteDuration = (4.0 / notes[currentNote].duration) * (60000 / bpm);
    tone(SIMON_BUZZER_PIN, notes[currentNote].note, noteDuration);
    currentNote++;
  }

  return true;
}

bool playSequence() {
  static unsigned long previousTime = 0;
  uint8_t pin;

  if (currentLed >= sequenceLength) {
    isLedOn = false;
    currentLed = 0;
    
    return false;
  }

  pin = sequence[currentLed];

  if (currentTime - previousTime >= SIMON_PLAY_SPEED) {
    previousTime = currentTime;

    if (!isLedOn) {
      digitalWrite(pins[pin], LOW);
      tone(SIMON_BUZZER_PIN, pinsNotes[pin]);
    } else {
      digitalWrite(pins[pin], HIGH);
      noTone(SIMON_BUZZER_PIN);
      currentLed++;
    }

    isLedOn = !isLedOn;
  }

  return true;
}

bool assertSequence(Event* event) {
  if (event->type == SIMON_BTN_PRESS) {
    if (event->btn != sequence[asserts]) {
      setSound(SIMON_SOUND_FAIL);
    } else {
      tone(SIMON_BUZZER_PIN, pinsNotes[event->btn]);
    }
  } else if (event->type == SIMON_BTN_RELEASE) {
    if (event->btn != sequence[asserts]) {
      return false;
    }

    asserts++;
    noTone(SIMON_BUZZER_PIN);
    
    if (asserts == sequenceLength) {
      return false;
    }
  }

  return true;
}

bool pollEvent(Event* event) {
  static int previousInput = SIMON_NO_INPUT;
  static unsigned long previousTime = 0;
  int input;

  if (currentTime - previousTime < SIMON_POLL_INTERVAL) {
    return false;
  }
  
  previousTime = currentTime;
  input = readInput();

  if (input == previousInput) {
    return false;
  }

  if (input == SIMON_NO_INPUT) {
    event->type = SIMON_BTN_RELEASE;
    event->btn = previousInput;
  } else {
    event->type = SIMON_BTN_PRESS;
    event->btn = input;
  }

  previousInput = input;

  return true;
}

int readInput() {
  // Ignore pins while the sequence is playing as the pins are configured as outputs
  for (int i = 0; !isSequencePlaying && i < ARRAY_LEN(pins); i++) {
    if (digitalRead(pins[i]) == LOW) {
      return i;
    }
  }

  return SIMON_NO_INPUT;
}