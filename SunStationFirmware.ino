/**
 * @file SunStationFirmware.cpp
 * 
 * This is the main program for the SunStation solar phone charger. 
 */

#include "SunStation.h"
#include <arduino-timer.h>

SunStation station;

auto timer = timer_create_default();

void setup()
{
  Serial.begin(9600);

  station.begin();

  timer.every(300, computeBatteryData);
  timer.every(3600000, computeEnergyData);
  timer.every(6000, broadcastStationData);
}

void loop()
{
  timer.tick();
  if (station.isButtonPressed())
  {
    toggleBle(60000);
    toggleLights(10000);
  }
}

void toggleBle(unsigned int countDown)
{
  static Timer<>::Task bleCountDownTask;
  station.turnBleOn();
  timer.cancel(bleCountDownTask); // limits countDownTasks to 1 at a time
  bleCountDownTask = timer.in(countDown, turnBleOff);
}

bool turnBleOff(void *)
{
  station.turnBleOff();
  return false;
}

void toggleLights(unsigned int countDown)
{
  static Timer<>::Task lightsCountDownTask;
  station.turnLightsOn();
  timer.cancel(lightsCountDownTask); // limits countDownTasks to 1 at a time
  lightsCountDownTask = timer.in(countDown, turnLightsOff);
}

bool turnLightsOff(void *)
{
  station.turnLightsOff();
  if (!station.isUsbOn())
  {
    station.turnUsbOn();
  }
  return false;
}

bool computeBatteryData(void *)
{
  station.measureBatteryCurrent();
  station.updateCumulativeCurrent();
  station.computeBatteryLevels();
  return true;
}

bool computeEnergyData(void *)
{
  station.computeEnergyProduced();
  station.computeCarbonSaved();
  station.resetCumulativeCurrent();
  return true;
}

bool broadcastStationData(void *)
{
  timer.in(0, sendBatteryCurrent);
  timer.in(1500, sendBatteryLevel);
  timer.in(3000, sendEnergyProduced);
  timer.in(4500, sendCarbonSaved);
  return true;
}

bool sendBatteryCurrent(void *)
{
  station.sendBlePacket("current", station.getBatteryCurrent());
  return true;
}

bool sendBatteryLevel(void *)
{
  station.sendBlePacket("battery", station.getBatteryLevel());
  return true;
}

bool sendEnergyProduced(void *)
{
  station.sendBlePacket("energy", station.getEnergyProduced());
  return true;
}

bool sendCarbonSaved(void *)
{
  station.sendBlePacket("carbon", station.getCarbonSaved());
  return true;
}
