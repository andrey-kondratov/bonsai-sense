#include <ArduinoBLE.h>
#include <Arduino_HTS221.h>
#include <Arduino_LPS22HB.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

const char* LocalName = "Bonsai Sense";
const int PollingMilliseconds = 500;
const int SleepMilliseconds = 500;
const int SleepLongMilliseconds = 5000;
const int HealthCheckIntervalMilliseconds = 1000;
const int HealthCheckFailCount = 3;
const int LedsPin = 6;
const int LedsPixelsCount = 20;

BLEService _service("181A");
BLEUnsignedLongCharacteristic _pressureCharacteristic("2A6D", BLERead | BLENotify);
BLEIntCharacteristic _temperatureCharacteristic("2A6E", BLERead | BLENotify);
BLEUnsignedIntCharacteristic _humidityCharacteristic("2A6F", BLERead | BLENotify);
uint32_t _pressure;
int16_t _temperature;
uint16_t _humidity;

BLEService _ledsService("BC1DF15A-C3CF-41BB-AF3F-CE7A15949B79");
BLEUnsignedIntCharacteristic _ledsHueCharacteristic("F76048A9-6446-49D7-A68D-3A0079D4EADC", BLERead | BLEWrite);
BLEByteCharacteristic _ledsSaturationCharacteristic("6F8250E5-E137-4365-8266-C9F81D39453B", BLERead | BLEWrite);
BLEByteCharacteristic _ledsValueCharacteristic("4AD5A9F6-472B-4879-9A8E-789382CD9AFF", BLERead | BLEWrite);
BLEDescriptor _ledsHueDescriptor(_ledsHueCharacteristic.uuid(), "Hue");
BLEDescriptor _ledsSaturationDescriptor(_ledsSaturationCharacteristic.uuid(), "Saturation");
BLEDescriptor _ledsValueDescriptor(_ledsValueCharacteristic.uuid(), "Value");
Adafruit_NeoPixel _leds(LedsPixelsCount, LedsPin, NEO_RGB + NEO_KHZ800);
uint16_t _ledsHue = 0;
uint8_t _ledsSaturation = 0;
uint8_t _ledsValue = 0;

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
  
  _ledsService.addCharacteristic(_ledsHueCharacteristic);
  _ledsService.addCharacteristic(_ledsSaturationCharacteristic);
  _ledsService.addCharacteristic(_ledsValueCharacteristic);
  _ledsHueCharacteristic.addDescriptor(_ledsHueDescriptor);
  _ledsSaturationCharacteristic.addDescriptor(_ledsSaturationDescriptor);
  _ledsValueCharacteristic.addDescriptor(_ledsValueDescriptor);
  BLE.addService(_ledsService);
  _ledsHueCharacteristic.writeValue(_ledsHue);
  _ledsSaturationCharacteristic.writeValue(_ledsSaturation);
  _ledsValueCharacteristic.writeValue(_ledsValue);

  BLE.setEventHandler(BLEConnected, onConnected);
  BLE.setEventHandler(BLEDisconnected, onDisconnected);

  Serial.println("Advertising...");
  BLE.advertise();

  initLeds();
  updateLeds(0, 0, 0);
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

  if (_ledsHueCharacteristic.written() || 
    _ledsSaturationCharacteristic.written() || 
    _ledsValueCharacteristic.written()) {
      updateLeds(_ledsHueCharacteristic.value(), 
        _ledsSaturationCharacteristic.value(), 
        _ledsValueCharacteristic.value());
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

void initLeds() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  _leds.begin();
  _leds.show();

  pinMode(LedsPin, OUTPUT);
  digitalWrite(LedsPin, LOW);
}

void updateLeds(uint16_t hue, uint8_t saturation, uint8_t value) {
  for (int i = 0; i < _leds.numPixels(); i++) {
    _leds.setPixelColor(i, _leds.gamma32(_leds.ColorHSV(hue, saturation, value)));
  }
  
  _leds.show();

  _ledsHue = hue;
  _ledsSaturation = saturation;
  _ledsValue = value;
}
