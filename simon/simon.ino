#include "pitches.h"

#define ARRAY_LEN(arr) sizeof(arr) / sizeof(*arr)

#define SIMON_MAX_SEQUENCE_LENGTH 255
#define SIMON_NO_INPUT -1
#define SIMON_INPUT_POLL_INTERVAL 100
#define SIMON_PLAY_SPEED 250
#define SIMON_NEXT_PAUSE 1000
#define SIMON_BUZZER_PIN 6

struct Note {
  unsigned int note;
  double duration;
};

// Checks the pushbuttons' state and returns the first that is
// pressed, or SIMON_NO_INPUT if none.
int readInput();

// Plays the current sequence.
void playSequence();

// Plays a melody
void playMelody(Note notes[], int length, int bpm = 120);

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
bool gameOver;

void setup() {
  gameOver = false;
  asserts = 0;
  sequenceLength = 0;

  Serial.begin(9600);
  randomSeed(analogRead(0));
  playMelody(startMelody, ARRAY_LEN(startMelody));
}

void loop() {
  int input;

  if (gameOver) {
    return;
  }

  // When the sequence is completed by the user, add new item
  if (asserts == sequenceLength) {
    delay(SIMON_NEXT_PAUSE);
    sequence[sequenceLength++] = random(0, sizeof(pins));
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
}

void playSequence() {
  int i;
  
  // Configure pins as OUTPUT 
  for (i = 0; i < sizeof(pins); i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], HIGH);
  }

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
  for (i = 0; i < sizeof(pins); i++) {
    pinMode(pins[i], INPUT_PULLUP);
  }
}

int readInput() {
  for (int i = 0; i < sizeof(pins); i++) {
    if (digitalRead(pins[i]) == LOW) {
      return i;
    }
  }

  return SIMON_NO_INPUT;
}

void playMelody(Note notes[], int length, int bpm = 120) {
  for (int i = 0; i < length; i++) {  
    unsigned int duration = (4.0 / notes[i].duration) * (60000 / bpm);

    tone(SIMON_BUZZER_PIN, notes[i].note, duration);
    delay(duration);
  }
}
