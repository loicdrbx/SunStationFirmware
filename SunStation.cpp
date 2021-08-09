/**
 * @file SunStation.cpp
 *
 * @mainpage SunStation Firmware
 *
 * @section intro_sec Introduction
 *
 * This is the documentation for the SunStation solar phone charger. 
 *  
 * @section dependencies Dependencies
 *
 * This program depends on the following external libraries being present on your system:
 *    \n<a href="https://github.com/adafruit/Adafruit_NeoPixel"> Adafruit_Neopixel </a>
 *    \n<a href="https://github.com/bblanchon/ArduinoJson"> ArduinoJson </a>
 *    \n<a href="https://github.com/contrem/arduino-timer"> arduino-timer </a>  \n 
 * Please make sure you have installed the latest versions before running this program.
 *
 * @section author Author
 *
 * Written by Loïc Darboux with guidance from Chris Falconi and Ibrahim Hashim.
 */
#include "SunStation.h"
#include "SoftwareSerial.h"
#include "EEPROM.h"
#include "Adafruit_NeoPixel.h"
#include "ArduinoJson.h"

const byte virtualRxPin = 4;  //!< Pin on which to receive serial data.
const byte virtualTxPin = 5;  //!< Pin on which to transmit serial data.
const byte bleVccPin = A2;    //!< Pin used to power the SunStation's Bluetooth module.
const byte batteryPin = A0;   //!< Pin used read analog values from the SunStation's battery.
const byte lightsDataPin = 3; //!< Pin used to drive data into lights object.
const byte lightsPmosPin = 2; //!< Pin used to toggle P-MOSFET used control SunStation's lights.
const byte usbRelayPin = 7;   //!< Pin used to toggle relay that controls SunStation's USB port.
const byte buttonPin = 6;     //!< Pin used to monitor SunStation's button state.

const float SunStation::batteryIdleDraw = 0.005083;
const float SunStation::batteryChargeRate = 0.10;
const float SunStation::batteryDischargeRate = 0.12;
const float SunStation::batteryMaxCapacity = 7330.0;
const float SunStation::currentMeasurementError = 0.42;
const float SunStation::currentMeasurementNoise = 0.20;

/** 
 * Creates a SoftwareSerial object used to communicate 
 * with the SunStation's Bluetooth module.
 */
SoftwareSerial BleSerial(virtualRxPin, virtualTxPin);

/** 
 * Creates the Adafruit_NeoPixel object used 
 * to control the SunStation's lights.
 */
Adafruit_NeoPixel lights(15, lightsDataPin, NEO_GRB + NEO_KHZ800);

/**
 * Initialises the SunStation's hardware and recovers saved data.
 */
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
  computeCarbonSaved();
}

/** @return True when the SunStation's button state 
 * changes from unpressed to pressed, false otherwise.
 */
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

/** Powers on the SunStation USB port. */
void SunStation::turnUsbOn()
{
  digitalWrite(usbRelayPin, HIGH);
}

/** Powers off the SunStation's USB port. */
void SunStation::turnUsbOff()
{
  digitalWrite(usbRelayPin, LOW);
}

/** Powers on the SunStation's Bluetooth module. */
void SunStation::turnBleOn()
{
  digitalWrite(bleVccPin, HIGH);
}

/** Powers off the SunStation's Bluetooth module. */
void SunStation::turnBleOff()
{
  digitalWrite(bleVccPin, LOW);
}

/** 
 * Turns on the SunStation's lights.
 * The number of LEDs turned on depends on the SunStation's battey level.
 */
void SunStation::turnLightsOn()
{
  digitalWrite(lightsPmosPin, LOW);
  byte numRingBars = round(getBatteryLevel() * 0.11);
  for (byte i = 0; i < numRingBars; i++)
  {
    lights.setPixelColor(i, lights.Color(255, 255, 155));
  }
  lights.setPixelColor(11, lights.Color(255, 255, 155));
  lights.setPixelColor(12, lights.Color(255, 255, 155));
  lights.setPixelColor(13, lights.Color(255, 255, 155));
  lights.setPixelColor(14, lights.Color(255, 255, 155));
  lights.show();
}

/** Turns off the SunStation's lights. */
void SunStation::turnLightsOff()
{
  lights.clear();
  lights.show();
  digitalWrite(lightsPmosPin, HIGH);
}

/** @return True if the SunStation's USB port is powered on, false otherwise. */
bool SunStation::isUsbOn() { return mIsUsbOn; }

/** @return The SunStation's last measured battery level in percentage (0-100). */
byte SunStation::getBatteryLevel() { return batteryLevel; };

/** @return The SunStation's last measured raw battery level. */
float SunStation::getRawBatteryLevel() { return rawBatteryLevel; };

/** @return The SunStation battery's last measured current draw/output (amps). */
float SunStation::getBatteryCurrent() { return batteryCurrent; }

/** @return Currrent accumulated by the SunStation's battery as of last update. */
float SunStation::getCumulativeCurrent() { return cumulativeCurrent; }

/** @return Amount of Carbon (kg of CO2) saved by the SunStation in its lifetime. */
float SunStation::getCarbonSaved() { return carbonSaved; }

/** @return Amount of energy (Wh) produced by the SunStation in its lifetime. */
unsigned long SunStation::getEnergyProduced() { return energyProduced; }

/** Measures the amount of current (amps) being drawn/output by the SunStation's battery. */
void SunStation::measureBatteryCurrent()
{
}

/** Updates the current accumulated by the SunStation's battery. */
void SunStation::updateCumulativeCurrent()
{
  if (batteryCurrent > 0)
  {
    cumulativeCurrent += batteryCurrent;
  }
}

/** Resets the current accumulated by the SunStation's battery. */
void SunStation::resetCumulativeCurrent()
{
  cumulativeCurrent = 0.0;
}

/** Computes the SunStation's battery levels (percentage and raw) in one call. */
void SunStation::computeBatteryLevels()
{
  float rate = batteryCurrent > 0 ? batteryChargeRate : batteryDischargeRate;
  rawBatteryLevel += (batteryCurrent * rate - batteryIdleDraw);
  rawBatteryLevel = constrain(rawBatteryLevel, 0, batteryMaxCapacity);
  batteryLevel = rawBatteryLevel * 100 / batteryMaxCapacity;
}

/**
 * Computes the amount of carbon (kg of CO2) saved by the SunStation in its lifetime.
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
 * Computes the amount energy produced (Wh) by the SunStation in its lifetime.
 * TODO: need sources for the conversion factor used
 */
void SunStation::computeEnergyProduced()
{
  unsigned long result = (unsigned long)(cumulativeCurrent / 10800 * 3.7) + energyProduced;
  energyProduced = constrain(result, 0, 999999999); // values > 999999999 overflow blePacket
}

/** Saves the amount of energy produced by the SunStation in non-volatile memory. */
void SunStation::saveEnergyProduced()
{
  EEPROM.put(0, energyProduced);
}

/** Retrieves the amount of energy produced by station from non-volatile memory. */
void SunStation::recoverEnergyProduced()
{
  EEPROM.get(0, energyProduced);
}

template <typename T>
/**
 * Sends a data packet from the SunStation over Bluetooth.
 * @param name A description of the data to be sent
 * @param value The value of the data to be sent.
 */
void SunStation::sendBlePacket(char *name, T value)
{
  StaticJsonDocument<20> blePacket; // max Bluetooth LE packet size is 20 bytes
  blePacket[name] = value;
  serializeJson(blePacket, BleSerial);
}

template void SunStation::sendBlePacket<byte>(char *name, byte value);
template void SunStation::sendBlePacket<float>(char *name, float value);
template void SunStation::sendBlePacket<unsigned long>(char *name, unsigned long value);
