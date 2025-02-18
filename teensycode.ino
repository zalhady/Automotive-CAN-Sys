
#include <FlexCAN_T4.h> // Include FlexCAN_T4 for CAN communication
#include <SD.h>         // Include the SD library for data logging
#include <SPI.h>        // Include SPI library for SD card communication

// CAN setup
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0; // Standard CAN
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CanFD; // CAN FD
File logFile;

// Ultrasonic sensor pins
const int TRIG_PINS[] = {2, 3, 4, 5, 6};
const int ECHO_PINS[] = {A0, A1, A2, A3, A4};
const int LED_PINS[] = {13, 9, 10, 11, 12};
const int SENSOR_COUNT = sizeof(TRIG_PINS) / sizeof(TRIG_PINS[0]);

// Thresholds for LEDs
const float LED_THRESHOLD_ON = 10.0;  // Turn LED on below this distance (cm)
const float LED_THRESHOLD_OFF = 12.0; // Turn LED off above this distance (cm)

// Array to track LED states to prevent flickering
bool ledStates[SENSOR_COUNT] = {false, false, false, false, false};

// Analog sensor pins
#define LIGHT_SENSOR_PIN A5
#define TEMP_SENSOR_PIN A6
#define PIEZO_SENSOR_PIN A7

// Thermistor constants
#define NOMINAL_RESISTANCE 10000
#define NOMINAL_TEMPERATURE 25
#define B_COEFFICIENT 3470
#define SERIES_RESISTOR 10000
#define REF_VOLTAGE 3.3
#define ADC_RESOLUTION 1023

void setup() {
    Serial.begin(115200);
    Serial7.begin(115200);

    for (int i = 0; i < SENSOR_COUNT; i++) {
        pinMode(TRIG_PINS[i], OUTPUT);
        pinMode(ECHO_PINS[i], INPUT);
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }

    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("SD card initialization failed!");
    } else {
        logFile = SD.open("log.txt", FILE_WRITE);
    }

    Can0.begin();
    CanFD.begin();
    Can0.setBaudRate(500000);
    CanFD.setBaudRate(1000000);
}

void loop() {
    // Analog Sensors
    int lightValue = analogRead(LIGHT_SENSOR_PIN);
    int tempRaw = analogRead(TEMP_SENSOR_PIN);
    float voltage = (tempRaw / (float)ADC_RESOLUTION) * REF_VOLTAGE;
    float thermistorResistance = (SERIES_RESISTOR * (REF_VOLTAGE - voltage)) / voltage;
    float steinhart = log(thermistorResistance / NOMINAL_RESISTANCE) / B_COEFFICIENT + 1.0 / (NOMINAL_TEMPERATURE + 273.15);
    float temperatureFahrenheit = ((1.0 / steinhart - 273.15) * 9.0 / 5.0 + 32.0)*(-1);
    int piezoValue = analogRead(PIEZO_SENSOR_PIN);

    String analogData = "Light: " + String(lightValue) + ", Temp (F): " + String(temperatureFahrenheit) + ", Piezo: " + String(piezoValue);
    Serial.println(analogData);

    // Ultrasonic Sensors
    String ultrasonicData = "";
    for (int i = 0; i < SENSOR_COUNT; i++) {
        digitalWrite(TRIG_PINS[i], LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG_PINS[i], HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PINS[i], LOW);

        unsigned long duration = pulseIn(ECHO_PINS[i], HIGH);
        unsigned int distance = duration / 58;

        // Add sensor data to string
        ultrasonicData += "Sensor " + String(i + 1) + ": " + (distance == 0 ? "No Echo" : String(distance) + " cm") + "  ";

        // LED Control Logic
        if (distance > 0) { // Valid distance received
            if (!ledStates[i] && distance <= LED_THRESHOLD_ON) {
                digitalWrite(LED_PINS[i], HIGH); // Turn on LED
                ledStates[i] = true;
            } else if (ledStates[i] && distance >= LED_THRESHOLD_OFF) {
                digitalWrite(LED_PINS[i], LOW); // Turn off LED
                ledStates[i] = false;
            }
        } else {
            digitalWrite(LED_PINS[i], LOW); // Turn off LED if no echo
            ledStates[i] = false;
        }
    }
    Serial.println(ultrasonicData);

    delay(500);
}
