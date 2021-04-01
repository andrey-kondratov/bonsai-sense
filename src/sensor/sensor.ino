#include <ArduinoBLE.h>
#include <Arduino_HTS221.h>
#include <Arduino_LPS22HB.h>

const char* LocalName = "Bonsai Sense";
const int PollingMilliseconds = 500;
const int SleepMilliseconds = 500;
const int SleepLongMilliseconds = 5000;
const int HealthCheckIntervalMilliseconds = 1000;
const int HealthCheckFailCount = 3;

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

  BLE.setEventHandler(BLEConnected, onConnected);
  BLE.setEventHandler(BLEDisconnected, onDisconnected);

  Serial.println("Advertising...");
  BLE.advertise();
}

void onConnected(BLEDevice device) {
  Serial.print(device.address());
  Serial.println(": connected.");
}

void onDisconnected(BLEDevice device) {
  Serial.print(device.address());
  Serial.println(": disconnected.");
}

void loop() {
  int wokeUpAt = wakeUp();

  BLEDevice central = BLE.central();
  if (central && central.connected()) {
    if (healthy(central)) {
      loopConnected(central, wokeUpAt);
      return;
    }

    central.disconnect();
  }

  while (millis() - wokeUpAt < PollingMilliseconds) {
    BLE.poll();
  }

  sleepNormal();
}

bool healthy(BLEDevice central) {
  int count = 0;
  while (central.rssi() == 0 && ++count < HealthCheckFailCount) {
    Serial.print(central.address());
    Serial.println(": no connection, will try again.");
    
    delay(HealthCheckIntervalMilliseconds);
  }

  if (count > 0 && count < HealthCheckFailCount) {
    Serial.print(central.address());
    Serial.println(": connection restored.");
  }

  return count < HealthCheckFailCount;
}

void loopConnected(BLEDevice central, int wokeUpAt){
  // read values
  auto temperature = (int16_t)(HTS.readTemperature() * 100);
  auto pressure = (uint32_t)(BARO.readPressure() * 10000);
  auto humidity = (uint16_t)(HTS.readHumidity() * 100);

  // polling in connected mode
  while (millis() - wokeUpAt < PollingMilliseconds / 2) {
    central.poll();
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

  // finish polling
  while (millis() - wokeUpAt < PollingMilliseconds) {
    central.poll();
  }

  // if central has subscribed to all characteristics,
  // switch to long sleep intervals
  if (_temperatureCharacteristic.subscribed() &&
    _pressureCharacteristic.subscribed() && 
    _humidityCharacteristic.subscribed()) {
    sleepLong();
    return;
  }

  sleepNormal();
}

int wakeUp() {
  digitalWrite(LED_PWR, HIGH);
  return millis();
}

void sleepNormal() {
  digitalWrite(LED_PWR, LOW);
  delay(SleepMilliseconds);
}

void sleepLong() {
  digitalWrite(LED_PWR, LOW);
  delay(SleepLongMilliseconds);
}
