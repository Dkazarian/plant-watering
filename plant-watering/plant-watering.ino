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
const int MOISTURE_THRESHOLD = 35;

int AIR_VALUE;
int WATER_VALUE;
const int EEPROM_AIR_ADDR = 0;
const int EEPROM_WATER_ADDR = 2;

int lastMoistureRead = 0;

void loadCalibrationValues() {
  EEPROM.get(EEPROM_AIR_ADDR, AIR_VALUE);
  EEPROM.get(EEPROM_WATER_ADDR, WATER_VALUE);

  if (AIR_VALUE == 0xFFFF || WATER_VALUE == 0xFFFF) {
    AIR_VALUE = 630;  // Default values
    WATER_VALUE = 270;
  }
}

void setup() {
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  pinMode(SENSOR_POWER, OUTPUT);
  digitalWrite(SENSOR_POWER, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Internal pull-up resistor

  #ifdef DEBUG
    Serial.begin(9600);
    delay(2500);
  #endif

  loadCalibrationValues();
}

void loop() {
  if (mustWaterPlant(MOISTURE_THRESHOLD)) {
    if (isBatteryLow()) {
      alert(LOW_BATTERY);
    } else {
      pumpWaterForSeconds(WATERPUMP_SECONDS);
      sleepSeconds(MOISTURE_INCREASE_WAIT);
      warnIfNoWater();
    }
  } else {
    alert(WET_SOIL);
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
      alert(DRY_SOIL);
    } else {
      alert(WET_SOIL);
    }
    return drySoil;
}

void warnIfNoWater() {
    int moisturePercent = getMoisturePercent();
    if (moisturePercent - lastMoistureRead < MOISTURE_INCREASE_THRESHOLD) {
      alert(WATERING_FAILED);
    }
}

void pumpWaterForSeconds(int seconds) {
  dprintln("Watering the plant...");
  digitalWrite(PUMP_PIN, HIGH);
  delay(seconds * 1000);
  digitalWrite(PUMP_PIN, LOW);
  dprintln("Watering complete.");
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

  // Measure Air Value
  dprintln("Measuring AIR value. Press button when ready.");
  while (digitalRead(BUTTON_PIN) == HIGH) {
    AIR_VALUE = analogRead(SENSOR_PIN);
    dprint("Current AIR value: ");
    dprintln(AIR_VALUE);
    delay(500); // Slow down readings
  }
  
  alert(1); // Beep once to confirm AIR_VALUE stored
  EEPROM.write(EEPROM_AIR_ADDR, (AIR_VALUE >> 8) & 0xFF);
  EEPROM.write(EEPROM_AIR_ADDR + 1, AIR_VALUE & 0xFF);
  delay(500); // Debounce
  
  // Measure Water Value
  dprintln("Measuring WATER value. Press button when ready.");
  while (digitalRead(BUTTON_PIN) == HIGH) {
    WATER_VALUE = analogRead(SENSOR_PIN);
    dprint("Current WATER value: ");
    dprintln(WATER_VALUE);
    delay(500);
  }
  
  alert(2); // Beep twice to confirm WATER_VALUE stored
  EEPROM.write(EEPROM_WATER_ADDR, (WATER_VALUE >> 8) & 0xFF);
  EEPROM.write(EEPROM_WATER_ADDR + 1, WATER_VALUE & 0xFF);

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