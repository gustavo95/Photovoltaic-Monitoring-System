/******************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
******************************************************************************
* AUTHOR      : Gustavo Costa Gomes de Melo (gustavocosta@ic.ufal.br)
* CREATE DATE : 03/22/2021
* PURPOSE     : Code for read all station sensors
*****************************************************************************/

#include "sensors.h"

unsigned int Sensors::anemCounter = 0;
unsigned int Sensors::pluvCounter = 0;

// ========= Init ============
void Sensors::init()
{
  try
  {
    ads1 = Adafruit_ADS1115(adc1Add);
    ads1.setGain(GAIN_SIXTEEN);
    ads1.begin();
  }
  catch(int e)
  {
    throw "Could not find ADS1117(0x48)!";
  }

  try
  {
    ads2 = Adafruit_ADS1115(adc2Add);
    ads2.setGain(GAIN_TWOTHIRDS);
    ads2.begin();
  }
  catch(int e)
  {
    throw "Could not find ADS1117(0x49)!";
  }

  /*try
  {
    bool status = bme.begin(0x76);
    if (!status) {
        throw "Could not find a valid BME280 sensor, check wiring!";
    }
  }
  catch(String e)
  {
    throw e;
  }*/

  try
  {
    sht20.begin();
  }
  catch(String e)
  {
    throw e;
  }

  pinMode(anemoPin, INPUT_PULLUP);
  pinMode(pluvPin, INPUT_PULLUP);
  anemCounter = 0;
  act_time = 0;
  pluvCounter = 0;
  attachInterrupt(digitalPinToInterrupt(anemoPin), addAnemCounter, RISING);
  attachInterrupt(digitalPinToInterrupt(pluvPin), addPluvCounter, FALLING);

  readTimes = 0;
  temp = 0;
  pressure = 0;
  humidity = 0;
  irradiance = 0;
  windSpeed = 0;
  windDirection = new int[8];
  windDirection[0] = 0;
  windDirection[1] = 0;
  windDirection[2] = 0;
  windDirection[3] = 0;
  windDirection[4] = 0;
  windDirection[5] = 0;
  windDirection[6] = 0;
  windDirection[7] = 0;
  rain = 0;
  PVtemp = 0;
  // voltage = 0;
  // current = 0;
}

// ========= Read all data each second ============
void Sensors::readAllData(float currGain, float voltGain)
{
  readTemp();
  readHumidity();
  readIradiance();
  readWindDirection();
  readRain();
  readPVtemp();
  // readVoltage(voltGain);
  // readCurrent(currGain);

  readTimes++;
}

// ========= Get average data ============
String Sensors::getAvgData(){
  String data = "";

  if(readTimes > 0){
    data += getTemp();
    data += ",";
    data += getHumidity();
    data += ",";
    data += getIradiance();
    data += ",";
    data += getWindSpeed();
    data += ",";
    data += getWindDirection();
    data += ",";
    data += getRain();
    data += ",";
    data += getPVtemp();
    // data += ",";
    // data += getVoltage();
    // data += ",";
    // data += getCurrent();

    readTimes = 0;
    temp = 0;
    pressure = 0;
    humidity = 0;
    irradiance = 0;
    windSpeed = 0;
    windDirection[0] = 0;
    windDirection[1] = 0;
    windDirection[2] = 0;
    windDirection[3] = 0;
    windDirection[4] = 0;
    windDirection[5] = 0;
    windDirection[6] = 0;
    windDirection[7] = 0;
    rain = 0;
    PVtemp = 0;
    // voltage = 0;
    // current = 0;
  }

  return data;
}


// ========= Read sensors data ============
void Sensors::readTemp()
{
  // temp += bme.readTemperature();
  // temp += 0;
  temp += sht20.temperature();

}

void Sensors::readHumidity()
{
  // humidity += bme.readHumidity();
  // humidity += 0;
  humidity += sht20.humidity();
}

void Sensors::readIradiance()
{
  double AIN01 = ads1.readADC_Differential_0_1()*adsGain1;
  irradiance += AIN01*1000/(1.69/100);
}

void Sensors::readWindDirection()
{
  unsigned int adcRead;

  adcRead = analogRead(32); // get the current reading from the sensor

  // The following table is ADC readings for the wind direction sensor output, sorted from low to high.
  // Each threshold is the midpoint between adjacent headings. The output is degrees for that ADC reading.
  // Note that these are not in compass degree order! See Weather Meters datasheet for more information.
  // Serial.print("adc: ");Serial.println(adc);
  float voltage = ((3.3/4095)*adcRead);

  if (voltage < 0.25) windDirection[7]++;       //315 - NO
  else if (voltage < 0.29) windDirection[6]++;  //270 - O
  else if (voltage < 0.37) windDirection[5]++;  //225 - SO
  else if (voltage < 0.45) windDirection[4]++;  //180 - S
  else if (voltage < 0.57) windDirection[3]++;  //135 - SE
  else if (voltage < 0.77) windDirection[2]++;  //90 - E
  else if (voltage < 1.2) windDirection[1]++;   //45 - NE
  else if (voltage < 2) windDirection[0]++;     //0 - N
}

void Sensors::readRain()
{
  rain = pluvCounter*0.25;
}

void Sensors::readPVtemp()
{
  unsigned int adcRead;
  adcRead = analogRead(tempPin);
  float voltage = ((3.3/4095)*adcRead);
  float Rt = 4590*(3.3-voltage)/voltage;
  float steinhart;
  steinhart = Rt / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;    
  PVtemp +=  (steinhart);
}

void Sensors::readVoltage(float gain)
{ 
  int adc_read = ads2.readADC_SingleEnded(1);
  if(adc_read > 32767){
    adc_read = adc_read - 65536;
  }
  //double AIN01 = ads1.readADC_Differential_0_1()*adsGain1;
  //double AIN02 = ads1.readADC_Differential_2_3()*adsGain1;
  //double OCV1 = ads2.readADC_SingleEnded(0)*adsGain2;
  //double OCV2 = ads2.readADC_SingleEnded(1)*adsGain2;
  //double SCC1 = ads2.readADC_SingleEnded(2)*adsGain2;
  //double SCC2 = ads2.readADC_SingleEnded(3)*adsGain2;

  double OCV2 = adc_read*adsGain2;
  
  voltage += OCV2*gain;
}

void Sensors::readCurrent(float gain)
{
  int adc_read = ads2.readADC_SingleEnded(0);
  if(adc_read > 32767){
    adc_read = adc_read - 65536;
  }
  double OCV1 = adc_read*adsGain2;
  current += OCV1*gain;
}


// ========= Aux functions ============
void Sensors::setAnemCounter0()
{
  anemCounter = 0;
}

void Sensors::setPluvCounter0()
{
  pluvCounter = 0;
}

void Sensors::addAnemCounter()
{
  anemCounter++;
}

void Sensors::addPluvCounter()
{
  pluvCounter++;
}


// ========= Get sensors data ============
String Sensors::getTemp()
{
  temp = temp/readTimes;
  if(temp < -40 ){
    temp = -40;
  }
  if(temp > 125){
    temp = 125;
  }
  return String(temp);
}

String Sensors::getHumidity()
{
  humidity = humidity/readTimes;
  if(humidity < 0){
    humidity = 0;
  }
  if(humidity > 100){
    humidity = 100;
  }
  return String(humidity);
}

String Sensors::getIradiance()
{
  irradiance = irradiance/readTimes;
  if(irradiance < 0.47)
  {
    irradiance = 0.0;
  }
  if(irradiance > 6553.5){
    irradiance = 6553.5;
  }
  return String(irradiance, 5);
}

String Sensors::getWindSpeed()
{
  int RPM = (anemCounter*60)/readTimes;
  anemCounter = 0;
  attachInterrupt(digitalPinToInterrupt(anemoPin), addAnemCounter, RISING);
  windSpeed = (((4 * pi * radius * RPM)/60) / 1000)*3.6; //Km/h
  
  if(windSpeed < 0){
    windSpeed = 0;
  }
  if(windSpeed > 255){
    windSpeed = 255;
  }
  return String(windSpeed);
}

String Sensors::getWindDirection()
{
  int direction = 0;
  int mostDirection = 0;

  for(int i=0; i<8; i++){
    if(windDirection[i] > mostDirection){
      mostDirection = windDirection[i];
      if(i == 0) direction = 0;
      if(i == 1) direction = 45;
      if(i == 2) direction = 90;
      if(i == 3) direction = 135;
      if(i == 4) direction = 180;
      if(i == 5) direction = 225;
      if(i == 6) direction = 270;
      if(i == 7) direction = 315;
    }
  }

  return String(direction);
}

String Sensors::getRain()
{
  if(rain < 0){
    rain = 0;
  }
  if(rain > 63.75){
    rain = 63.75;
  }
  return String(rain);
}

String Sensors::getPVtemp()
{
  PVtemp = PVtemp/readTimes;
  if(PVtemp < -40 ){
    PVtemp = -40;
  }
  if(PVtemp > 125){
    PVtemp = 125;
  }
  return String(PVtemp);
}

String Sensors::getVoltage()
{
  voltage = voltage/readTimes;
  if(voltage < 0){
    voltage = 0;
  }
  if(voltage > 6553.5){
    voltage = 6553.5;
  }
  return String(voltage, 5);
}

String Sensors::getCurrent()
{
  current = current/readTimes;
  if(current < 0){
    current = 0;
  }
  if(current > 25.5){
    current = 25.5;
  }
  return String(current, 5);
}
