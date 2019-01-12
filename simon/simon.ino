#define MAX_SEQUENCE_LENGTH 255

#define NO_INPUT 0

#define INPUT_DELAY 100

// Pause before playing the next sequence.
#define PAUSE_DELAY 1000

// Number of milliseconds each LED is on when playing the sequence.
#define PLAY_DELAY 250

// Checks the pushbuttons' state and returns the first that is
// pressed, or NO_INPUT if none.
byte readInput();

// Plays the current sequence.
void playSequence();

// Pins connected to the LEDs/pushbuttons
byte pins[] = {2, 3, 4};

byte sequence[MAX_SEQUENCE_LENGTH];
byte sequenceLength;
byte asserts;

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));

  asserts = 0;
  sequenceLength = 0;
}

void loop() {
  static byte input;

  if (asserts < sequenceLength) {
    if (input = readInput()) {
      do {
        delay(INPUT_DELAY);
      } while (readInput()); // wait for button release

      if (input == sequence[asserts]) {
        asserts++;
      } else {
        Serial.println("Game Over! Press restart.");
        while (true);
      }
    }
  } else {
    delay(PAUSE_DELAY);

    // add random item to the sequence
    sequence[sequenceLength++] = pins[ random(0, sizeof(pins)) ];
    playSequence();
    asserts = 0;
  }
}

void playSequence() {
  byte i;

  // Configure pins as OUTPUT 
  for (i = 0; i < sizeof(pins); i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], HIGH);
  }

  // Play sequence
  i = 0;
  while (true) {
    digitalWrite(sequence[i], LOW);
    delay(PLAY_DELAY);
    digitalWrite(sequence[i], HIGH);

    if (++i < sequenceLength) {
      delay(PLAY_DELAY);
    } else {
      break;
    }
  }

  // Configure pins as INPUT_PULLUP again
  for (i = 0; i < sizeof(pins); i++) {
    pinMode(pins[i], INPUT_PULLUP);
  }
}

byte readInput() {
  for (int i = 0; i < sizeof(pins); i++) {
    if (digitalRead(pins[i]) == LOW) {
      return pins[i];
    }
  }

  return NO_INPUT;
}
