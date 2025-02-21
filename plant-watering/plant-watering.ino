#include <LowPower.h>

#define DEBUG
#ifdef DEBUG
  #define dprint(x) Serial.print(x)
  #define dprintln(x) Serial.println(x)
#else
  #define dprint(x)
  #define dprintln(x)
#endif

// ALARMS
const int DRY_SOIL = 1;
const int WET_SOIL = 2;
const int WATERING_FAILED = 3;
const int BATTERY_LOW = 4;

// 📌 Pin Definitions
const int SENSOR_PIN = A0;
const int SENSOR_POWER = 13;   
const int WATER_LEVEL_POWER = 12;
const int WATER_LEVEL_PIN = A1;
const int PUMP_PIN = 5;
const int BUZZER_PIN = 10; 
const int BUTTON_PIN = 4;     

// Light Sleep Settings
const long SLEEP_CYCLE_SECONDS = 8L;
const long TOTAL_SLEEP_SECONDS = 8L * 60L * 60L;

// SENSOR AND PUMP
const int WATERPUMP_SECONDS = 6;
const int SENSOR_STABILIZATION = 200;
const int MOISTURE_INCREASE_WAIT = 10;
const int MOISTURE_INCREASE_THRESHOLD = 5;
const int MOISTURE_THRESHOLD = 35;

const int AIR_VALUE = 630;
const int WATER_VALUE = 270;

int lastMoistureRead = 0;

void sleepSeconds(int seconds) {
  for (int i = 0; i < seconds / SLEEP_CYCLE_SECONDS; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    delay(10);
  }
}

void setup() {
  // PINS SETUP
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  pinMode(SENSOR_POWER, OUTPUT);
  digitalWrite(SENSOR_POWER, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  #ifdef DEBUG
    Serial.begin(9600);
    delay(2500);
  #endif
}

void loop() {
  if (mustWaterPlant(MOISTURE_THRESHOLD)) {
    pumpWaterForSeconds(WATERPUMP_SECONDS);
    sleepSeconds(MOISTURE_INCREASE_WAIT);
    warnIfNoWater();
  } else {
    alarm(WET_SOIL);
  }
  sleepSeconds(TOTAL_SLEEP_SECONDS);
}

int getMoisturePercent() {
  digitalWrite(SENSOR_POWER, HIGH);
  analogRead(SENSOR_PIN);
  delay(SENSOR_STABILIZATION);

  int moistureValue = analogRead(SENSOR_PIN);
  digitalWrite(SENSOR_POWER, LOW);
  dprint("Sensor: ");
  dprintln(moistureValue);

  int moisturePercent = map(moistureValue, AIR_VALUE, WATER_VALUE, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);
  dprint("Soil Moisture: ");
  dprint(moisturePercent);
  dprintln("%");

  return moisturePercent;
}

bool mustWaterPlant(int moistureThreshold) {
    int moisturePercent = getMoisturePercent();
    bool drySoil = moisturePercent < moistureThreshold;
    if (drySoil) {
      alarm(DRY_SOIL);
    } else {
      alarm(WET_SOIL);
    }
    return drySoil;
}

void warnIfNoWater() {
    int moisturePercent = getMoisturePercent();
    if (lastMoistureRead - moisturePercent < MOISTURE_INCREASE_THRESHOLD) {
      alarm(WATERING_FAILED);
    }
    lastMoistureRead = moisturePercent;
}

void alarm(int pulses) {
  for (int i = 0; i < pulses; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
  }
}

void pumpWaterForSeconds(int seconds) {
    dprintln("Watering the plant...");
    digitalWrite(PUMP_PIN, HIGH);
    delay(seconds * 1000);
    digitalWrite(PUMP_PIN, LOW);
    dprintln("Watering complete.");
}
