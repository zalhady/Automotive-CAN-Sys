 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Declare BLE server and characteristic objects
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;

// Server callbacks to handle connection status
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        deviceConnected = true;
        Serial.println("Device connected");
    }

    void onDisconnect(BLEServer *pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected");
        // Restart advertising so another device can connect
        pServer->getAdvertising()->start();
    }
};

// Initialize BLE and set up characteristics
void setupBLE() {
    BLEDevice::init("ESP32_S3_BLE"); // Set BLE device name
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(BLEUUID((uint16_t)0x180D)); // Custom Service UUID

    // Create a BLE Characteristic for notifications
    pCharacteristic = pService->createCharacteristic(
        BLEUUID((uint16_t)0x2A37), // Custom Characteristic UUID
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->start();
    Serial.println("Waiting for a client connection...");
}

void setup() {
    Serial.begin(115200); // Initialize Serial for debugging
    setupBLE();           // Initialize BLE setup
    randomSeed(analogRead(0)); // Seed random number generator
}

void loop() {
    static unsigned long lastNotifyTime = 0;
    unsigned long currentTime = millis();

    if (deviceConnected) {
        // Generate random mock data
        int lightValue = random(0, 3); // Random whole numbers between 0 and 5
        float temperature = random(7709, 7812) / 100.0; // Random numbers between 74.09 and 75.11
        int piezoValue = random(0, 2); // Random 0 or 1

        String analogData = "Light: " + String(lightValue) + 
                            ", Temp (F): " + String(temperature, 2) + 
                            ", Piezo: " + String(piezoValue);

        String ultrasonicData = "";
        for (int i = 1; i <= 5; i++) {
            int distance = random(4, 121); // Random values between 4cm and 120cm
            ultrasonicData += "Sensor " + String(i) + ": " + String(distance) + " cm  ";
        }

        // Combine both data types
        String sensorData = analogData + "\n" + ultrasonicData;

        if (currentTime - lastNotifyTime > 1000) { // Send data every second
            pCharacteristic->setValue(sensorData.c_str()); // Use c_str() for BLE compatibility
            pCharacteristic->notify(); // Notify connected client
            Serial.println("Sent sensor data: \n" + sensorData);
            lastNotifyTime = currentTime;
        }
    }

    delay(100); // Allow BLE communication
}
