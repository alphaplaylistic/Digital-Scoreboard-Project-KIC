#include <Adafruit_NeoPixel.h>
#include <IRremote.hpp>

// ====== PIN ASSIGNMENTS ======
#define SCORE1_PIN 6
#define SCORE2_PIN 7
#define FOUL1_PIN  8
#define FOUL2_PIN  9
#define ROUND_PIN  10
#define TIMER_PIN  11
#define IR_RECEIVE_PIN 2

// ====== LED COUNTS ======
#define SCORE_LEDS 14   // 2 digits × 7
#define FOUL_LEDS 7     // 1 digit
#define ROUND_LEDS 7    // 1 digit
#define TIMER_LEDS 28   // 4 digits × 7

// ====== NEOPIXEL OBJECTS ======
Adafruit_NeoPixel score1Strip(SCORE_LEDS, SCORE1_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel score2Strip(SCORE_LEDS, SCORE2_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel foul1Strip(FOUL_LEDS, FOUL1_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel foul2Strip(FOUL_LEDS, FOUL2_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel roundStrip(ROUND_LEDS, ROUND_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel timerStrip(TIMER_LEDS, TIMER_PIN, NEO_GRB + NEO_KHZ800);

// ====== DIGIT SEGMENT PATTERNS (a–g) ======
const byte digits[10][7] = {
  {0,1,1,1,1,1,1}, // 0
  {0,1,0,0,0,0,1}, // 1
  {1,0,1,1,0,1,1}, // 2
  {1,1,1,0,0,1,1}, // 3
  {1,1,0,0,1,0,1}, // 4
  {1,1,1,0,1,1,0}, // 5
  {1,1,1,1,1,1,0}, // 6
  {0,1,0,0,0,1,1}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,0,1,1,1}  // 9
};

// ====== DIGIT MAPPINGS ======
const byte leftDigitMap[7]  = {0,1,2,3,4,5,6};
const byte rightDigitMap[7] = {7,8,9,10,11,12,13};

// Standard single-digit map (used for round display)
const byte singleDigitMap[7] = {0,1,2,3,4,5,6};

// Custom foul digit map based on your layout:
//   c
// d   b
//   a
// e   g
//   f
const byte foulDigitMap[7] = {
  2, // segment a
  1, // segment b
  0, // segment c
  3, // segment d
  4, // segment e
  6, // segment f
  5  // segment g
};

// Timer (4 digits MMSS)
const byte timerDigit1Map[7] = {0,1,2,3,4,5,6};
const byte timerDigit2Map[7] = {7,8,9,10,11,12,13};
const byte timerDigit3Map[7] = {14,15,16,17,18,19,20};
const byte timerDigit4Map[7] = {21,22,23,24,25,26,27};

// ====== SCORE, FOUL, ROUND ======
int scoreTeam1 = 0;
int scoreTeam2 = 0;
int foulTeam1 = 0;
int foulTeam2 = 0;
int roundNumber = 0;

// ====== TIMER VARIABLES ======
unsigned long timerDuration = 0;
unsigned long timerStartTime = 0;
bool timerRunning = false;

// ====== DEBOUNCE ======
unsigned long lastPressTime = 0;
const unsigned long debounceDelay = 300;

// ====== SETUP ======
void setup() {
  Serial.begin(9600);

  score1Strip.begin(); score1Strip.show();
  score2Strip.begin(); score2Strip.show();
  foul1Strip.begin();  foul1Strip.show();
  foul2Strip.begin();  foul2Strip.show();
  roundStrip.begin();  roundStrip.show();
  timerStrip.begin();  timerStrip.show();

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  displayAll();
}

// ====== MAIN LOOP ======
void loop() {
  if (timerRunning) {
    unsigned long elapsed = millis() - timerStartTime;
    if (elapsed >= timerDuration) {
      timerRunning = false;
      displayTimer(0);
      // TODO: Add buzzer or flash alert here
    } else {
      displayTimer(timerDuration - elapsed);
    }
  }

  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
      IrReceiver.resume();
      return;
    }

    unsigned long code = IrReceiver.decodedIRData.command;
    unsigned long currentMillis = millis();
    Serial.print("IR Code: 0x"); Serial.println(code, HEX);

    if (currentMillis - lastPressTime > debounceDelay) {
      if (code == 0x16) { scoreTeam1++; if(scoreTeam1>99)scoreTeam1=99;
        displayTwoDigit(scoreTeam1, score1Strip); }

      else if (code == 0x19) { scoreTeam2++; if(scoreTeam2>99)scoreTeam2=99;
        displayTwoDigit(scoreTeam2, score2Strip); }

      else if (code == 0xC) { foulTeam1++; if(foulTeam1>9)foulTeam1=9;
        displayDigit(foulTeam1, foulDigitMap, foul1Strip); }

      else if (code == 0x18) { foulTeam2++; if(foulTeam2>9)foulTeam2=9;
        displayDigit(foulTeam2, foulDigitMap, foul2Strip); }

      else if (code == 0x1C) { roundNumber++; if(roundNumber>9)roundNumber=9;
        displayDigit(roundNumber, singleDigitMap, roundStrip); }

      else if (code == 0x46) {
        if (!timerRunning && timerDuration < 5999000) {
          timerDuration += 60000;
          displayTimer(timerDuration);
        }
      } 
      else if (code == 0x40) {
        if (timerRunning) timerRunning = false;
        else if (timerDuration > 0) {
          timerStartTime = millis();
          timerRunning = true;
        }
      } 
      else if (code == 0x15) {
        timerRunning = false;
        timerDuration = 0;
        displayTimer(0);
      }

      else if (code == 0x52) {
        scoreTeam1 = scoreTeam2 = 0;
        foulTeam1 = foulTeam2 = 0;
        roundNumber = 0;
        timerDuration = 0;
        timerRunning = false;
        displayAll();
      }

      lastPressTime = currentMillis;
    }

    IrReceiver.resume();
  }
}

// ====== DISPLAY FUNCTIONS ======
void displayDigit(int digit, const byte* map, Adafruit_NeoPixel &strip) {
  for (int seg = 0; seg < 7; seg++) {
    if (digits[digit][seg]) {
      strip.setPixelColor(map[seg], strip.Color(50, 50, 0)); // Yellow-green
    } else {
      strip.setPixelColor(map[seg], 0);
    }
  }
  strip.show();
}

void displayTwoDigit(int number, Adafruit_NeoPixel &strip) {
  int tens = number / 10;
  int units = number % 10;
  displayDigit(tens, leftDigitMap, strip);
  displayDigit(units, rightDigitMap, strip);
}

void displayTimer(unsigned long remainingMs) {
  int totalSeconds = remainingMs / 1000;
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;

  int d1 = (minutes / 10) % 10;
  int d2 = minutes % 10;
  int d3 = (seconds / 10) % 10;
  int d4 = seconds % 10;

  displayDigit(d1, timerDigit1Map, timerStrip);
  displayDigit(d2, timerDigit2Map, timerStrip);
  displayDigit(d3, timerDigit3Map, timerStrip);
  displayDigit(d4, timerDigit4Map, timerStrip);
}

void displayAll() {
  displayTwoDigit(scoreTeam1, score1Strip);
  displayTwoDigit(scoreTeam2, score2Strip);
  displayDigit(foulTeam1, foulDigitMap, foul1Strip);
  displayDigit(foulTeam2, foulDigitMap, foul2Strip);
  displayDigit(roundNumber, singleDigitMap, roundStrip);
  displayTimer(timerDuration);
}
