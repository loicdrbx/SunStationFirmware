/**
 * @file SunStation.h
 *
 * Helper class that stores the state and 
 * functions used to interact with the SunStation.
 */
#ifndef SunStation_h
#define SunStation_h

#include "Arduino.h"

/** Interface for SunStation helper class */
class SunStation
{
public:
  void begin();
  bool isButtonPressed();
  bool isUsbOn();
  void turnUsbOn();
  void turnUsbOff();
  void turnBleOn();
  void turnBleOff();
  void turnLightsOn();
  void turnLightsOff();
  byte getBatteryLevel();
  float getRawBatteryLevel();
  float getBatteryCurrent();
  float getCumulativeCurrent();
  float getCarbonSaved();
  unsigned long getEnergyProduced();
  void measureBatteryCurrent();
  void updateCumulativeCurrent();
  void resetCumulativeCurrent();
  void computeBatteryLevels();
  void computeCarbonSaved();
  void computeEnergyProduced();
  void saveEnergyProduced();
  void recoverEnergyProduced();
  template <typename T>
  void sendBlePacket(char *name, T value);

private:
  bool mIsUsbOn = false;             //!< whether or not usb port is delivering power
  byte batteryLevel = 0;             //!< battery level in percentage (0-100)
  float rawBatteryLevel = 0.0;       //!< battey level in range (0-maxCapacity)
  float batteryCurrent = 0.0;        //!< battery's amperage output (amps)
  float cumulativeCurrent = 0.0;     //!< current accumulated by battery in 1 hour
  float carbonSaved = 0.0;           //!< carbon emissions saved by station in its lifetime (kg of CO2)
  unsigned long energyProduced = 0L; //!< energy produced by station in its lifetime (Wh)

  static const float batteryIdleDraw;         //!< battery's amperage draw when station is idle (amps / 300 ms)
  static const float batteryChargeRate;       //!< battery's charge rate (amps)
  static const float batteryDischargeRate;    //!< battery's discharge rate (amps)
  static const float batteryMaxCapacity;      //!< battery's maximum usable capacity  (cummulative amps till full)
  static const float currentMeasurementError; //!< error in battery's current measurement (amps)
  static const float currentMeasurementNoise; //!< values in range -measurementNoise to +measurementNoise are unrealiable
};

#endif
