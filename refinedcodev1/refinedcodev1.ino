#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// LCD pins
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2; 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); 

// Temperature sensors 
#define ONE_WIRE_BUS 8
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Peltier control pins
int peltierCoolingPin = 6;  
int peltierHeatingPin = 7;

// Temperature control settings
float targetTemp = 25.0;      // Middle of range
float hysteresis = 1.0;       // ±1°C dead band
float upperlimit = targetTemp + hysteresis;  // 26°C
float lowerlimit = targetTemp - hysteresis;  // 24°C

// Check T/F
bool cooling = false;
bool heating = false;

void setup() {
    Serial.begin(9600);
    sensors.begin();
    
    pinMode(peltierCoolingPin, OUTPUT);
    pinMode(peltierHeatingPin, OUTPUT);
    
    lcd.begin(16, 2);
    lcd.print("Vaccine Storage");
    lcd.setCursor(0, 1);
    lcd.print("Ready...");
    delay(2000);
    
    Serial.println("System Started");
    Serial.print("Target: ");
    Serial.print(targetTemp);
    Serial.println("C");
}

void loop() {
    sensors.requestTemperatures();
    
    // Mean of sensors
    int numSensors = sensors.getDeviceCount();
    float total = 0;
    int validReadings = 0;
    
    for (int i = 0; i < numSensors; i++) {
        float temp = sensors.getTempCByIndex(i);
        if (temp != -127.0 && temp != 85.0) {
            total += temp;
            validReadings++;
        }
    }
    
    if (validReadings == 0) {
        lcd.clear();
        lcd.print("SENSOR ERROR!");
        Serial.println("ERROR: No valid sensors!");
        delay(2000);
        return;
    }
    
    float celsius = total / validReadings;
    
    // Hysteresis control - prevents rapid on/off switching
    if (celsius > upperlimit + 0.5) {
        // Too hot - start cooling
        cooling = true;
        heating = false;
    } else if (celsius < upperlimit - 0.5 && cooling) {
        // Cooled enough - stop
        cooling = false;
    }
    
    if (celsius < lowerlimit - 0.5) {
        // Too cold - start heating
        heating = true;
        cooling = false;
    } else if (celsius > lowerlimit + 0.5 && heating) {
        // Warmed enough - stop
        heating = false;
    }
    
    // Peltier code
    digitalWrite(peltierCoolingPin, cooling ? HIGH : LOW);
    digitalWrite(peltierHeatingPin, heating ? HIGH : LOW);
    
    // Display on LCD
    lcd.clear();
    lcd.print("Temp: ");
    lcd.print(celsius, 1);
    lcd.print("C");
    
    lcd.setCursor(0, 1);
    if (cooling) lcd.print("COOLING");
    else if (heating) lcd.print("HEATING");
    else lcd.print("OK");
    
    lcd.print(" (");
    lcd.print(validReadings);
    lcd.print("S)");
    
    // Serial output
    Serial.print("Temp: ");
    Serial.print(celsius, 1);
    Serial.print("C | Sensors: ");
    Serial.print(validReadings);
    Serial.print(" | ");
    if (cooling) Serial.println("COOLING");
    else if (heating) Serial.println("HEATING");
    else Serial.println("STANDBY");
    
    delay(2000);
}