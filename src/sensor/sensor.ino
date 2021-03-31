#include <ArduinoBLE.h>
#include <Arduino_HTS221.h>
#include <Arduino_LPS22HB.h>

const char* LocalName = "Bonsai Sense";
const int ActiveMilliseconds = 1000;
const int SleepingMilliseconds = 5000;

BLEService _service("181A");
BLEUnsignedLongCharacteristic _pressureCharacteristic("2A6D", BLERead | BLENotify);
BLEIntCharacteristic _temperatureCharacteristic("2A6E", BLERead | BLENotify);
BLEUnsignedIntCharacteristic _humidityCharacteristic("2A6F", BLERead | BLENotify);

uint32_t _pressure;
int16_t _temperature;
uint16_t _humidity;

void setup() {
  digitalWrite(LED_PWR, LOW);
  
  Serial.begin(9600);
  delay(3000);
  
  while (!BLE.begin()) {
    Serial.println("Error starting BLE");
    delay(1000);
  }

  if (!HTS.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }

  if (!BARO.begin()) {
    Serial.println("Failed to initialize pressure sensor!");
    while (1);
  }

  Serial.println("Configuring BLE...");
  BLE.setLocalName(LocalName);
  _service.addCharacteristic(_pressureCharacteristic);
  _service.addCharacteristic(_temperatureCharacteristic);
  _service.addCharacteristic(_humidityCharacteristic);

  BLE.addService(_service);
  BLE.setAdvertisedService(_service);

  Serial.println("Advertising...");
  BLE.advertise();
}

void loop() {
  // wake up
  auto started = millis();
  digitalWrite(LED_PWR, HIGH);

  // read values while cold :)
  auto temperature = (int16_t)(HTS.readTemperature() * 100);
  auto pressure = (uint32_t)(BARO.readPressure() * 10000);
  auto humidity = (uint16_t)(HTS.readHumidity() * 100);

  // loop for messages
  while (millis() - started < ActiveMilliseconds / 2) {
    BLE.poll();
  }

  // drop stale connections
  auto central = BLE.central();
  if (central && central.connected() && central.rssi() == 0) {
    central.disconnect();
  }

  // update values
  if (temperature != _temperature) {
    _temperatureCharacteristic.writeValue(_temperature = temperature);
    _temperature = temperature;
  }
  
  if (pressure != _pressure) {
    _pressureCharacteristic.writeValue(_pressure = pressure);
    _pressure = pressure;
  }
  
  if (humidity != _humidity) {
    _humidityCharacteristic.writeValue(_humidity = humidity);
    _humidity = humidity;
  }

  // wait till the end of active time frame
  while (millis() - started < ActiveMilliseconds) {
    BLE.poll();
  }

  // sleep
  digitalWrite(LED_PWR, LOW);
  delay(SleepingMilliseconds);
}
