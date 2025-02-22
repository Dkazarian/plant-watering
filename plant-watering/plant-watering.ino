#include <EEPROM.h>
#include <LowPower.h>

#define DEBUG
#ifdef DEBUG
  #define dprint(x) Serial.print(x)
  #define dprintln(x) Serial.println(x)
#else
  #define dprint(x)
  #define dprintln(x)
#endif

// Alerts
const int DRY_SOIL = 1;
const int WET_SOIL = 0;
const int WATERING_FAILED = 4;
const int LOW_BATTERY = 3;
const int ENTER_CALIBRATION = 2;

// Battery
const int LOW_BATTERY_THRESHOLD = 3100; 

// 📌 Pin Definitions
const int SENSOR_PIN = A0;
const int SENSOR_POWER = 13;   
const int PUMP_PIN = 5;
const int BUZZER_PIN = 10; 
const int BUTTON_PIN = 4;     

// Light Sleep Settings
const long SLEEP_CYCLE_SECONDS = 8L;
const long TOTAL_SLEEP_SECONDS = 6L * 60L * 60L;

// Sensor and Pump
const int WATERPUMP_SECONDS = 6;
const int SENSOR_STABILIZATION = 200;
const int MOISTURE_INCREASE_WAIT = 120;
const int MOISTURE_INCREASE_THRESHOLD = 5;

const int EEPROM_DRY_ADDR = 0;

int dryValue;
int soilDrynessValue = 0;

void loadCalibrationValues() {
  EEPROM.get(EEPROM_DRY_ADDR, dryValue);

  if (dryValue == 0xFFFF ) {
    dryValue = 378;
  }
}

void setup() {
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  pinMode(SENSOR_POWER, OUTPUT);
  digitalWrite(SENSOR_POWER, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  #ifdef DEBUG
    Serial.begin(9600);
    delay(2500);
  #else
    Serial.end();
  #endif

  loadCalibrationValues();

  if (isBatteryLow()) {
    alert(LOW_BATTERY);
  }
}

void loop() {
  if (mustWaterPlant()) {
    if (isBatteryLow()) {
      alert(LOW_BATTERY);
    } else {
      pumpWaterForSeconds(WATERPUMP_SECONDS);
      sleepSeconds(MOISTURE_INCREASE_WAIT);
      warnIfNoWater(MOISTURE_INCREASE_THRESHOLD);
    }
  } else {
    alert(WET_SOIL);
  }

  sleepSeconds(TOTAL_SLEEP_SECONDS);
}

bool mustWaterPlant() {
  bool drySoil = getSoilDryness() >= dryValue;
  if (drySoil) {
    alert(DRY_SOIL);
  } else {
    alert(WET_SOIL);
  }
  return drySoil;
}

int getSoilDryness() {
  digitalWrite(SENSOR_POWER, HIGH);
  analogRead(SENSOR_PIN);
  delay(SENSOR_STABILIZATION);

  soilDrynessValue = analogRead(SENSOR_PIN);
  digitalWrite(SENSOR_POWER, LOW);
  dprint("Sensor: ");
  dprintln(soilDrynessValue);
  return soilDrynessValue;
}

void pumpWaterForSeconds(int seconds) {
  dprintln("Watering the plant...");
  digitalWrite(PUMP_PIN, HIGH);
  delay(seconds * 1000);
  digitalWrite(PUMP_PIN, LOW);
  dprintln("Watering complete.");
}

void warnIfNoWater(int moistureThreshold) {
    int previousDryness = soilDrynessValue;
    if (getSoilDryness() >= previousDryness + moistureThreshold) {
      alert(WATERING_FAILED);
    }
}

void alert(int pulses) {
  for (int i = 0; i < pulses; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
  }
}

void calibrateSensor() {
  dprintln("Entering Calibration Mode...");
  alert(2); // Beep twice to indicate entering calibration mode

  digitalWrite(SENSOR_POWER, HIGH);
  delay(SENSOR_STABILIZATION);

  dryValue = getSoilDryness();
  alert(1);

  EEPROM.write(EEPROM_DRY_ADDR, (dryValue >> 8) & 0xFF);
  EEPROM.write(EEPROM_DRY_ADDR + 1, dryValue & 0xFF);

  digitalWrite(SENSOR_POWER, LOW);
  dprintln("Calibration complete.");
}

bool isBatteryLow() {
  long batteryVoltage = readVcc();
  dprint("Battery Voltage: ");
  dprint(batteryVoltage);
  dprintln(" mV");

  return batteryVoltage < LOW_BATTERY_THRESHOLD;
}

void sleepSeconds(int seconds) {
  for (int i = 0; i < seconds / SLEEP_CYCLE_SECONDS; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    delay(10);
    if (i % 3 == 0) {
      if (isButtonHeld(1000)) {
        calibrateSensor();
      }
    }
  }
}

long readVcc() {
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);

  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));

  uint16_t result = ADC;
  return (1100L * 1024L) / result;
}

bool isButtonHeld(int durationMs) {
  int count = 0;
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
    count += 10;
    if (count >= durationMs) {
      return true;
    }
  }
  return false;
}