#include <ArduinoBLE.h>
#include <Arduino_HS300x.h>
#include <Arduino_APDS9960.h>

#define VALUE_SIZE 20

BLEService temperatureService = BLEService("00000000-5EC4-4083-81CD-A10B8D5CF6EC");
BLECharacteristic temperatureCharacteristic = BLECharacteristic("00000001-5EC4-4083-81CD-A10B8D5CF6EC", BLERead | BLENotify, VALUE_SIZE);
BLEService proximityService = BLEService("00000003-5EC4-4083-81CD-A10B8D5CF6EC");
BLECharacteristic proximityCharacteristic = BLECharacteristic("00000004-5EC4-4083-81CD-A10B8D5CF6EC", BLERead | BLENotify, VALUE_SIZE);


// last temperature reading
int oldTemperature = 0;
// last proximity reading
uint8_t oldProximity = 0;
// last time the temperature was checked in ms
long previousMillis = 0;

void setup() {
  // initialize serial communication
  Serial.begin(9600);
  while (!Serial)
    ;

  if (!HS300x.begin()) {
    Serial.println("Failed to initialize HS300x.");
    while (1)
      ;
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960.");
    while (1);
  }

  // initialize the built-in LED pin to indicate when a central is connected
  pinMode(LED_BUILTIN, OUTPUT);

  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  BLE.setLocalName("BLE-TEMP");
  BLE.setDeviceName("BLE-TEMP");

  // add the temperature & proximity characteristics
  temperatureService.addCharacteristic(temperatureCharacteristic);
  proximityService.addCharacteristic(proximityCharacteristic);

  // add temperature & proximity services
  BLE.addService(temperatureService);
  BLE.addService(proximityService)

  // set initial value for this characteristic
  temperatureCharacteristic.writeValue("0.0");
  proximityCharacteristic.writeValue("0");

  // start advertising
  BLE.advertise();
  Serial.println("Bluetooth® device active, waiting for connections...");
}

void loop() {
  // wait for a Bluetooth® Low Energy central
  BLEDevice central = BLE.central();

  // if a central is connected to the peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's BT address:
    Serial.println(central.address());
    // turn on the LED to indicate the connection
    digitalWrite(LED_BUILTIN, HIGH);


    // while the central is connected
    // update temperature & proximity every 200ms
    while (central.connected()) {
      long currentMillis = millis();
      if (currentMillis - previousMillis >= 200) {
        previousMillis = currentMillis;
        updateTemperature();
        updateProximity();
      }
    }

    // turn off the LED after disconnect
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}

void updateTemperature() {
  float temperature = HS300x.readTemperature();

  if (temperature != oldTemperature) {
    char buffer[VALUE_SIZE];
    int ret = snprintf(buffer, sizeof buffer, "%f", temperature);
    if (ret >= 0) {
      temperatureCharacteristic.writeValue(buffer);
      Serial.print("Temperature: ");
      Serial.println(buffer); // Print to Serial for debugging
      oldTemperature = temperature;
    }
  }
}

void updateProximity() {
  // Read proximity value
  uint8_t proximity = APDS.readProximity();

  // If the proximity value is valid (within range)
  if (proximity <= 255) {
    // Format the proximity value as a string
    char buffer[VALUE_SIZE];
    int ret = snprintf(buffer, sizeof(buffer), "%u", proximity); // Convert proximity to string

    if (ret >= 0) {
      // Print the proximity value to the Serial Monitor
      Serial.print("Proximity: ");
      Serial.println(buffer);
      // Send the proximity value over BLE
      proximityCharacteristic.writeValue(buffer);
      oldProximity = proximity;
    }
  }
}
