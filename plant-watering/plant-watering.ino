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
const int DRY_SOIL = 2;
const int WET_SOIL = 1;
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
const long LONG_SLEEP_SECONDS = 6L * 60L * 60L;
const long SHORT_SLEEP_SECONDS = 1L * 60L * 60L;

// Sensor and Pump
const int WATERPUMP_SECONDS = 6;
const int SENSOR_STABILIZATION = 200;

const int EEPROM_CALIBRATED_ADDR = 10;
const int EEPROM_DRY_ADDR = 11;

int dryValue;
int soilDrynessValue;

void loadCalibrationValues() {
  boolean calibrated;
  EEPROM.get(EEPROM_CALIBRATED_ADDR, calibrated);
  EEPROM.get(EEPROM_DRY_ADDR, dryValue);

  if (!calibrated || dryValue == 0xFFFF ) {
    dryValue = 378;
  }
  dprint("Dry point: ");
  dprintln(dryValue);
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
    alert(DRY_SOIL);
    if (isBatteryLow()) {
      alert(LOW_BATTERY);
    } else {
      // Since I don't have different watering modes for pot sizes
      // I just water in small amounts, have short sleeps so the soil
      // absorbs it and repeat until I reached the set value.
      // Then I make a 'long sleep.
      pumpWaterForSeconds(WATERPUMP_SECONDS);
      sleepSeconds(SHORT_SLEEP_SECONDS);
    }
  } else {
    alert(WET_SOIL);
  }

  sleepSeconds(LONG_SLEEP_SECONDS);
}

bool mustWaterPlant() {
  return getSoilDryness() >= dryValue;
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

void alert(int pulses) {
  dprint(pulses);
  dprintln(" beeps.");
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


  EEPROM.put(EEPROM_CALIBRATED_ADDR, true);
  EEPROM.put(EEPROM_DRY_ADDR, dryValue);

  digitalWrite(SENSOR_POWER, LOW);
  dprint("Dry value set to ");
  dprintln(dryValue);
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
    if (i % 2 == 0) {
      if (digitalRead(BUTTON_PIN) == LOW) {
        calibrateSensor();
        return;
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
