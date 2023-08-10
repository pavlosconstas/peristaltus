#include <Encoder.h>
#include <Wire.h>
#include <AccelStepper.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define the connections for the first motor
#define EN_PIN_SOL 5
#define STP_PIN_SOL 6
#define DIR_PIN_SOL 7

#define EN_PIN_H2O 8
#define STP_PIN_H2O 9
#define DIR_PIN_H2O 10

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define ENCODER_BTN 3

// Create instances of AccelStepper for both motors
AccelStepper stepper1(1, STP_PIN_SOL, DIR_PIN_SOL);
AccelStepper stepper2(1, STP_PIN_H2O, DIR_PIN_H2O);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Encoder myEnc(2, 4);

// variables

static int params[] = { 180, 0, 0 };  // time, water, solution
static bool start = false;            // motors running?
static int selected = false;          // selected
static int selected_num = -1;         // selected what?
static int cursor = 0;                // location of cursor
static long oldPos = 0;               // old position of encoder
static bool selectedTrigger = false;
static bool startTrigger = false;

static long loop_time = millis();


void setup() {
  Serial.begin(115200);

  // initialize steppers
  stepper1.setMaxSpeed(200);
  stepper2.setMaxSpeed(200);

  pinMode(EN_PIN_SOL, OUTPUT);
  pinMode(EN_PIN_H2O, OUTPUT);
  digitalWrite(EN_PIN_SOL, LOW);  // LOW enables the driver
  digitalWrite(EN_PIN_H2O, LOW);
  digitalWrite(DIR_PIN_SOL, HIGH);  // Change this based on your motor's rotation
  digitalWrite(DIR_PIN_H2O, HIGH);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  // initialize display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.fillScreen(WHITE);

  // initialize encoder
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  myEnc.write(0);
}

void drawDisplay() {
  display.clearDisplay();
  display.setCursor(0, 10 * cursor + 10);
  display.println(">");
  display.setCursor(10, 10);
  display.println("Time: " + (String)(params[0] / 60) + ":" + (String)(params[0] % 60));
  display.setCursor(10, 20);
  display.println("Pump 1: " + (String)params[1]);
  display.setCursor(10, 30);
  display.println("Pump 2: " + (String)params[2]);
  display.setCursor(10, 40);
  display.println(!start ? "Start" : "Stop");
  display.display();
}

void loop() {
  while (!start) {
    if (digitalRead(ENCODER_BTN) == LOW && !selectedTrigger) {
      selected = !selected;
      selectedTrigger = true;
      selected_num = cursor;
      if (selected_num == 3) {
        start = true;
        startTrigger = true;
        loop_time = millis();
      }
    }

    if (digitalRead(ENCODER_BTN) != LOW) {
      selectedTrigger = false;
    }

    long pos = myEnc.read();
    if (selected == false) {
      if (pos > oldPos + 2) {
        if ((cursor + 1) > 3) {
          cursor = 0;
        } else {
          cursor++;
        }
        oldPos = pos;
      }
      if (pos < oldPos - 2) {
        if ((cursor - 1) < 0) {
          cursor = 3;
        } else {
          cursor--;
        }
        oldPos = pos;
      }
    } else {
      if (pos != oldPos) {
        if (cursor != 0) {
          if (cursor != 3) {
            if ((params[cursor] + (oldPos - pos)) >= 0 && (params[cursor] + (oldPos - pos)) <= 100) {
              params[cursor] += (oldPos - pos);
            }
            oldPos = pos;
          }
        }
        if (cursor == 0) {
          if ((params[cursor] + (oldPos - pos)) >= 0 && (params[cursor] + (oldPos - pos)) <= 600) {
            params[cursor] += (oldPos - pos);
          }
          oldPos = pos;
        }
      }

      if (cursor == 3) {
        start = true;
      }
    }

    drawDisplay();
  }

  while (start) {

    if (digitalRead(ENCODER_BTN) == LOW && !startTrigger) {
      start = false;
      startTrigger = true;
      cursor = 0;
      selected = false;
      selectedTrigger = false;
      break;
    }

    if (digitalRead(ENCODER_BTN) != LOW) {
      startTrigger = false;
    }

    if ((millis() - 1000) > loop_time) {
      --params[0];
      loop_time = millis();
      drawDisplay();
    }

    if (params[0] <= 0) {
      start = false;
      break;
    }

    stepper2.setSpeed(2 * params[1]);
    stepper1.setSpeed(2 * params[2]);

    stepper2.runSpeed();
    stepper1.runSpeed();
  }
}
