#include <LowPower.h>

#define DEBUG 0

// 📌 Pin Definitions
const int SENSOR_PIN = A0;
const int SENSOR_POWER = 13;   
const int PUMP_PIN = 5;
const int BUZZER_PIN = 10; 
const int BUTTON_PIN = 4;     
const int ALARM_DISABLED = false;

// Light Sleep Settings
const long int SLEEP_CYCLE_SECONDS = 8;
const long TOTAL_SLEEP_SECONDS = 8L * 60L * 60L;
const long NUM_SLEEP_CYCLES = TOTAL_SLEEP_SECONDS / SLEEP_CYCLE_SECONDS;

// SENSOR AND PUMP
const int WATERPUMP_SECONDS = 6;
const int SENSOR_STABILIZATION = 200;
const int MOISTURE_THRESHOLD = 35;

const int DRY_VALUE = 630;   // Measured in dry air
const int WET_VALUE = 270;   // Measured in water

void executeWithSleep(void (*func)()) {
    func();
    
    for (long int i = 0L; i < NUM_SLEEP_CYCLES; i++) {
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
    //pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(BUZZER_PIN, LOW);
    
    // BATTERY SAVING
    #if DEBUG
    Serial.begin(9600);
    Serial.println("Debugging enabled");
    #endif
}

void loop() {
    executeWithSleep(
        [](){
            if (shouldWater()) {
                alarm(2);
                pumpWaterForSeconds(WATERPUMP_SECONDS);
            } else {
              alarm(1);
            }
        }
    );
}

bool shouldWater() {
    digitalWrite(SENSOR_POWER, HIGH);
    delay(SENSOR_STABILIZATION);
  
    analogRead(SENSOR_PIN);
    delay(SENSOR_STABILIZATION);
   
    
    int moistureValue = analogRead(SENSOR_PIN);
    digitalWrite(SENSOR_POWER, LOW);

    Serial.print("Sensor: ");
    Serial.println(moistureValue);
    int moisturePercent = map(moistureValue, DRY_VALUE, WET_VALUE, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);

    Serial.print("Soil Moisture: ");
    Serial.print(moisturePercent);
    Serial.println("%");

    return moisturePercent < MOISTURE_THRESHOLD;
}

void alarm(int pulses) {
  if (ALARM_DISABLED) return;
  for(int i = 0; i < pulses; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
  }
}
void pumpWaterForSeconds(int seconds) {
    Serial.println("💧 Watering the plant...");
    digitalWrite(PUMP_PIN, HIGH);
    delay(seconds * 1000);
    digitalWrite(PUMP_PIN, LOW);
    Serial.println("🚰 Watering complete.");
}


