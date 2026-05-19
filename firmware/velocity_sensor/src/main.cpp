#include <Arduino.h>
#include <Wire.h>
#include "wiring_private.h"
#include "velSensor.h"
#include "velSensor.c"
// Clean version Win

// NOTE: Second I2C port: SDA 11; SCL 13;
//  Seting second I2C port to use pins 11 and 13
#define digiPot 0x2F // I2C address of the digital potentiometers
TwoWire myWire(&sercom1, 11, 13);

// NOTE: I/O pins of new Arduino Shield:
const uint8_t LED_R = 9;
const uint8_t LED_G = 8;
const uint8_t LED_Y = 12;
const uint8_t LED_B = A5;
const uint8_t LED_VEL = 10; // Light source for Vel Sensor
const uint8_t LED_OFS = A4; // Light source for OFS TSL2561
const uint8_t button1 = 2;  // Button 1
const uint8_t button2 = 3;  // Button 2
const uint8_t button3 = 5;  // Button 3

// NOTE: Velocity Sensor Comparator output pins:

const uint8_t comp_s1 = 7; // COMP_A
const uint8_t comp_s2 = 6; // COMP_B

// Globals
volatile unsigned long lastTriggerTime_s1 = 0; // Last trigger time for Sensor 1
volatile unsigned long lastTriggerTime_s2 = 0; // Last trigger time for Sensor 2
volatile unsigned long lastValidS1Time = 0;    // Timestamp of last accepted ISR1
volatile unsigned long lastValidS2Time = 0;    // Timestamp of last accepted ISR2

// Debounce timing constants
const unsigned long DEBOUNCE_TIME_US = 1000; // 1ms debounce for high-speed droplets
const unsigned long MIN_INTER_S1_US = 3000;   // Ignore S1 edges <3ms after previous valid S1
const unsigned long MIN_INTER_S2_US = 3000;   // Ignore S2 edges <3ms after previous valid S2

// NOTE: Digipots Wiper values:
// CONFIG: W1 -> DigiPot1 -> COMP_1 -> Sensor 2 (Second sensor contacted to the droplet)
// CONFIG: W2 -> DigiPot2 -> COMP_2 -> Sensor 1 (First sensor contacted to the droplet)
// CONFIG: n -> Number of droplets to reset timer 2.

const uint8_t WIPER1 = 76; // RED
const uint8_t WIPER2 = 83; // GREEN
const uint8_t n = 2;       // Number of sensors (1 or 2)

// NOTE: SCADE init variables
inC_velSensor inC;
outC_velSensor outC;
kcg_uint32 Time = 0;
kcg_bool fs1 = false;
kcg_bool fs2 = false;
float prevVel;

void isr_1()
{
  // Interrupt for COMP_A (Sensor 1 - First sensor contacted by droplet)
  unsigned long currentTime = micros();
  unsigned long elapsedTime = currentTime - lastTriggerTime_s1;

  // Improved debouncing and per-sensor inter-event guard
  if (elapsedTime > DEBOUNCE_TIME_US && (currentTime - lastValidS1Time) > MIN_INTER_S1_US && digitalRead(comp_s1) == HIGH)
  {
    lastTriggerTime_s1 = currentTime;
    lastValidS1Time = currentTime;
    fs1 = true;     // Set SCADE input flag

    // Lightweight ISR: capture timestamp and signal main loop to process SCADE
    Time = currentTime;     // Capture timestamp associated with this event
  }
}

void isr_2()
{
  // Interrupt for COMP_B (Sensor 2 - Second sensor contacted by droplet)
  unsigned long currentTime = micros();
  unsigned long elapsedTime = currentTime - lastTriggerTime_s2;

  // Improved debouncing and per-sensor inter-event guard
  if (elapsedTime > DEBOUNCE_TIME_US && (currentTime - lastValidS2Time) > MIN_INTER_S2_US && digitalRead(comp_s2) == HIGH)
  {
    lastTriggerTime_s2 = currentTime;
    lastValidS2Time = currentTime;
    fs2 = true;     // Set SCADE input flag

    // Lightweight ISR: capture timestamp and signal main loop to process SCADE
    Time = currentTime;     // Capture timestamp associated with this event
  }
}

void setWiper(TwoWire &bus, uint8_t value)
{
  // Set the wiper value for the digital potentiometer
  bus.beginTransmission(digiPot);
  bus.write(value); // Set the wiper value
  bus.endTransmission();
}

void setupPins()
{
  // Set pin modes for all I/O pins
  pinMode(comp_s1, INPUT);        // Comparator output for Sensor 1
  pinMode(comp_s2, INPUT);        // Comparator output for Sensor 2
  pinMode(LED_VEL, OUTPUT);       // Velocity sensor LED
  pinMode(LED_B, OUTPUT);         // Blue LED indicator
  pinMode(LED_Y, OUTPUT);         // Yellow LED indicator
  pinMode(LED_R, OUTPUT);         // Red LED indicator
  pinMode(LED_G, OUTPUT);         // Green LED indicator
  pinMode(LED_OFS, OUTPUT);       // OFS TSL2561 LED
  pinMode(button1, INPUT_PULLUP); // Button 1 with pullup
  pinMode(button2, INPUT_PULLUP); // Button 2 with pullup
  pinMode(button3, INPUT_PULLUP); // Button 3 with pullup

  // Initialize LED states
  digitalWrite(LED_VEL, HIGH); // Turn on velocity sensor LED
  digitalWrite(LED_B, LOW);    // Turn off other LEDs initially
  digitalWrite(LED_Y, LOW);
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_OFS, LOW);
}

void setup()
{
  // SCADE function initialization
  velSensor_init(&outC); // Initialize SCADE function
  // Initialize I2C
  Wire.begin();                  // Initialize I2C
  myWire.begin();                // Initialize second I2C port
  pinPeripheral(11, PIO_SERCOM); // Set pin 11 to SERCOM mode
  pinPeripheral(13, PIO_SERCOM); // Set pin 13 to SERCOM mode

  // Initialize serial communication
  Serial.begin(38400);
  Serial.println(">>> Optical Droplet Velocity Sensor [OFS 3.0] <<<");
  delay(10);

  // Set digiPot wiper values
  setWiper(Wire, WIPER1);   // Set wiper value for DigiPot1
  setWiper(myWire, WIPER2); // Set wiper value for DigiPot2
  myWire.end();
  Wire.end();

  // Set up all pin modes and initial states
  setupPins();

  // Set up interrupts
  attachInterrupt(digitalPinToInterrupt(comp_s1), isr_1, RISING); // Attach interrupt for COMP_A
  attachInterrupt(digitalPinToInterrupt(comp_s2), isr_2, RISING); // Attach interrupt for COMP_B

  inC.n = n; // Set input count for Sensor 2

  // Print initial message
  Serial.print("Digital Potentiometer 1: ");
  Serial.println(WIPER1);
  Serial.print("Digital Potentiometer 2: ");
  Serial.println(WIPER2);
  Serial.print("n: ");
  Serial.println(inC.n);
  Serial.println("=== Velocity Measurements with ISR Debug ===");
  Serial.println("Format: Vel: [velocity] mm/s, ISR1 calls: [count], ISR2 calls: [count], n: [counter], startTime: [us], stopTime: [us], timeDiff: [us]");
  Serial.println("DEBUG format: TimerInit, TimerStop, Counter_s2, ISR1 total, ISR2 total, fs1, fs2");
  Serial.println("ISR Debug: ISR1/ISR2 with count, Time, timer states, and timing values");
  delay(1000);
}

void loop()
{
  // Update LED indicators based on SCADE outputs
  digitalWrite(LED_B, outC.timerInit);
  digitalWrite(LED_Y, outC.timerStop);

  // Process SCADE in main loop when flags are set (no debug prints)
  if (fs1 || fs2)
  {
    velSensor(&inC, &outC);
  }

  // Print velocity only if it changed and is valid
  if (outC.vel != prevVel && outC.vel > 0.0)
  {
    Serial.println(outC.vel);
    prevVel = outC.vel;
  }

  // No periodic debug to keep serial line quiet

  // Reset sensor flags in main loop (safer than in ISR)
  // This prevents potential race conditions
  if (fs1 == true)
  {
    fs1 = false;     // Reset SCADE flag for Sensor 1
  }
  if (fs2 == true)
  {
    fs2 = false;     // Reset SCADE flag for Sensor 2
  }
}
