#include <TimerOne.h>
#include "pitches.h"

#define ARRAY_LEN(arr) sizeof(arr) / sizeof(*arr)

#define SIMON_MAX_SEQUENCE_LENGTH 255
#define SIMON_NO_INPUT -1
#define SIMON_INPUT_POLL_INTERVAL 100
#define SIMON_PLAY_SPEED 250
#define SIMON_NEXT_PAUSE 1000
#define SIMON_BUZZER_PIN 6
#define SIMON_DISPLAY_INTERRUPT 10000

struct Note {
  unsigned int note;
  double duration;
};

// Plays the current sequence.
void playSequence();

// Display current score
void displayScore();

// Plays a melody
void playMelody(Note notes[], int length, int bpm = 120);

// Checks the pushbuttons' state and returns the first that is pressed, or SIMON_NO_INPUT if none.
int readInput();

// Initializes an array of pins
void setPins(byte pins[], int length, byte mode, byte val = -1);

// BCD numbers
byte numbers[][4] = {
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
byte decoderPins[] = {10, 9, 8, 7};

// Pins connected to the tens/units displays
byte digitsPins[] = {12, 11};

// Pins connected to the LEDs/pushbuttons
byte pins[] = {2, 3, 4, 5};
byte pinsNotes[] = {NOTE_C3, NOTE_D3, NOTE_E3, NOTE_F3};

// Sounds
Note startMelody[] = { 
  {NOTE_C3, 16}, {NOTE_D3, 16}, {NOTE_F3, 4} 
};

Note failMelody[] = { 
  {NOTE_F3, 16}, {NOTE_E3, 16}, {NOTE_D3, 16}, {NOTE_A2, 4} 
};

// Game state
byte sequence[SIMON_MAX_SEQUENCE_LENGTH];
byte sequenceLength;
byte asserts;
byte score;
bool gameOver;

void setup() {
  gameOver = false;
  asserts = 0;
  sequenceLength = 0;
  score = 0;
  
  randomSeed(analogRead(0));
  Serial.begin(9600);
  
  setPins(decoderPins, ARRAY_LEN(decoderPins), OUTPUT);
  setPins(digitsPins, ARRAY_LEN(digitsPins), OUTPUT);
  setPins(pins, ARRAY_LEN(pins), INPUT_PULLUP);
  playMelody(startMelody, ARRAY_LEN(startMelody));

  Timer1.initialize(SIMON_DISPLAY_INTERRUPT);
  Timer1.attachInterrupt(displayScore);
}

void loop() {
  int input;

  if (gameOver) {
    return;
  }

  // When the sequence is completed by the user, add new item
  if (asserts == sequenceLength) {
    delay(SIMON_NEXT_PAUSE);
    sequence[sequenceLength++] = random(0, ARRAY_LEN(pins));
    playSequence();
    asserts = 0;
  }

  // Wait for the user to repeat the sequence
  input = readInput();
  
  if (input == SIMON_NO_INPUT) {
    return;
  }

  if (input == sequence[asserts]) {
    asserts++;
  } else {
    playMelody(failMelody, ARRAY_LEN(failMelody));
    gameOver = true;
    return;
  }

  // Play tone while pressed
  tone(SIMON_BUZZER_PIN, pinsNotes[input]);

  do {
    delay(SIMON_INPUT_POLL_INTERVAL);
  } while (readInput() == input);

  noTone(SIMON_BUZZER_PIN);

  if (asserts == sequenceLength) {
    score++;
  }
}

void playSequence() {
  int i;
  
  // Configure pins as OUTPUT
  setPins(pins, ARRAY_LEN(pins), OUTPUT, HIGH);

  // Play sequence
  for (i = 0; i < sequenceLength; i++) {
    byte pin = sequence[i];
    
    // On
    digitalWrite(pins[pin], LOW);
    tone(SIMON_BUZZER_PIN, pinsNotes[pin]);
    delay(SIMON_PLAY_SPEED);
    
    // Off
    digitalWrite(pins[pin], HIGH);
    noTone(SIMON_BUZZER_PIN);

    // Wait before next
    if ((i + 1) < sequenceLength) {
      delay(SIMON_PLAY_SPEED);
    }
  }

  // Configure pins as INPUT_PULLUP again
  setPins(pins, ARRAY_LEN(pins), INPUT_PULLUP);
}

void displayScore() {
  static byte digit = 0; // 0 = units, 1 = tens... etc.
  int i;
  int rest = score;
  int number;

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

void playMelody(Note notes[], int length, int bpm = 120) {
  for (int i = 0; i < length; i++) {  
    unsigned int duration = (4.0 / notes[i].duration) * (60000 / bpm);

    tone(SIMON_BUZZER_PIN, notes[i].note, duration);
    delay(duration);
  }
}

int readInput() {
  for (int i = 0; i < ARRAY_LEN(pins); i++) {
    if (digitalRead(pins[i]) == LOW) {
      return i;
    }
  }

  return SIMON_NO_INPUT;
}

void setPins(byte pins[], int length, byte mode, byte val = -1) {
  int i;

  for (i = 0; i < length; i++) {
    pinMode(pins[i], mode);

    if (mode == OUTPUT && val != -1) {
      digitalWrite(pins[i], val);
    }
  }
}
