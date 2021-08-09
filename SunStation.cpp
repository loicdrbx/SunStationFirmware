#include "SunStation.h"
#include "SoftwareSerial.h"
#include "EEPROM.h"
#include "Adafruit_NeoPixel.h"
#include "ArduinoJson.h"

const byte bleRxPin = 4;
const byte bleTxPin = 5;
const byte bleVccPin = A2;
const byte batteryPin = A0;
const byte lightsDataPin = 3;
const byte lightsPmosPin = 2;
const byte usbRelayPin = 7;
const byte buttonPin = 6;

const float SunStation::batteryIdleDraw = 0.005083;
const float SunStation::batteryChargeRate = 0.10;
const float SunStation::batteryDischargeRate = 0.12;
const float SunStation::batteryMaxCapacity = 7330.0;
const float SunStation::currentMeasurementError = 0.42;
const float SunStation::currentMeasurementNoise = 0.20;

SoftwareSerial BleSerial(bleRxPin, bleTxPin);
Adafruit_NeoPixel lights(15 /*# of LEDs*/, lightsDataPin, NEO_GRB + NEO_KHZ800);

void SunStation::begin()
{
  pinMode(buttonPin, INPUT);

  pinMode(usbRelayPin, OUTPUT);
  digitalWrite(usbRelayPin, LOW);

  pinMode(lightsPmosPin, OUTPUT);
  digitalWrite(lightsPmosPin, LOW);
  lights.begin();
  lights.clear();
  lights.show();

  pinMode(bleVccPin, OUTPUT);
  digitalWrite(bleVccPin, LOW);
  BleSerial.begin(9600);

  recoverEnergyProduced();
}

bool SunStation::isButtonPressed()
{
  // Note: monitoring state changes ensures that only one press
  // is detected when continuous force is applied on the button
  static byte lastButtonState = 0;
  bool isButtonPressed = false;
  byte buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState && buttonState == HIGH)
  {
    isButtonPressed = true;
  }
  lastButtonState = buttonState;
  return isButtonPressed;
}

void SunStation::turnUsbOn()
{
  digitalWrite(usbRelayPin, HIGH);
}

void SunStation::turnUsbOff()
{
  digitalWrite(usbRelayPin, LOW);
}

void SunStation::turnBleOn()
{
  digitalWrite(bleVccPin, HIGH);
}

void SunStation::turnBleOff()
{
  digitalWrite(bleVccPin, LOW);
}

void SunStation::turnLightsOn()
{
  digitalWrite(lightsPmosPin, LOW);

  // Set ring LEDs (0-10) according to battery level (0-100%)
  byte numRingBars = round(getBatteryLevel() * 0.11);
  for (byte i = 0; i < numRingBars; i++)
  {
    lights.setPixelColor(i, lights.Color(255, 255, 155));
  }

  // Set central LEDs
  lights.setPixelColor(11, lights.Color(255, 255, 155));
  lights.setPixelColor(12, lights.Color(255, 255, 155));
  lights.setPixelColor(13, lights.Color(255, 255, 155));
  lights.setPixelColor(14, lights.Color(255, 255, 155));

  lights.show();
}

void SunStation::turnLightsOff()
{
  lights.clear();
  lights.show();
  digitalWrite(lightsPmosPin, HIGH);
}

bool SunStation::isUsbOn() { return mIsUsbOn; }

byte SunStation::getBatteryLevel() { return batteryLevel; };

float SunStation::getRawBatteryLevel() { return rawBatteryLevel; };

float SunStation::getBatteryCurrent() { return batteryCurrent; }

float SunStation::getCumulativeCurrent() { return cumulativeCurrent; }

float SunStation::getCarbonSaved() { return carbonSaved; }

unsigned long SunStation::getEnergyProduced() { return energyProduced; }

void SunStation::measureBatteryCurrent()
{

}

void SunStation::updateCumulativeCurrent()
{
  if (batteryCurrent > 0)
  {
    cumulativeCurrent += batteryCurrent;
  }
}

void SunStation::computeBatteryLevels()
{
  float rate = batteryCurrent > 0 ? batteryChargeRate : batteryDischargeRate;
  rawBatteryLevel += (batteryCurrent * rate - batteryIdleDraw);
  rawBatteryLevel = constrain(rawBatteryLevel, 0, batteryMaxCapacity);
  batteryLevel = rawBatteryLevel * 100 / batteryMaxCapacity;
}

/**
 * Computes the amount of CO2 (kg) saved by station's energy production
 * Energy to carbon emissions conversion factor from the EPA:
 * 7.09 × 10^-4 metric tons CO2/kWh -> 7.09 × 10^-4  kg CO2/Wh
 * https://www.epa.gov/energy/greenhouse-gases-equivalencies-calculator-calculations-and-references
 */
void SunStation::computeCarbonSaved()
{
  // convert and round result to nearest gram
  carbonSaved = ((int)(energyProduced * 0.000709 * 1000.0 + 0.5) / 1000.0);
}

/**
 * Computes the amount energy produced by the sation in its lifetime (wH)
 */
void SunStation::computeEnergyProduced()
{
  // TODO: better math explanation needed (sources too)
  unsigned long result = (unsigned long)(cumulativeCurrent / 10800 * 3.7) + energyProduced;
  energyProduced = constrain(result, 0, 999999999); // values > 999999999 overflow blePacket
}

/**
 * Save amount energy of energy produced by station in non-volatile 
 * memory (so that it can persist when power is off)
 */
void SunStation::saveEnergyProduced()
{
  EEPROM.put(0, energyProduced);
}

/**
 * Retrieve amount of energy produced by station from non-volatile memory 
 */
void SunStation::recoverEnergyProduced()
{
  EEPROM.get(0, energyProduced);
}

template <typename T>
void SunStation::sendBlePacket(char *name, T value)
{
  StaticJsonDocument<20> blePacket; // max ble packet size is 20 bytes
  blePacket[name] = value;
  serializeJson(blePacket, BleSerial);
}

template void SunStation::sendBlePacket<byte>(char *name, byte value);
template void SunStation::sendBlePacket<float>(char *name, float value);
template void SunStation::sendBlePacket<unsigned long>(char *name, unsigned long value);
