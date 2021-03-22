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
* PURPOSE     : Main code for PV generation data logger
*****************************************************************************/

// //==========================================================
// // ALL LIBS MUST BE INSTALLED TRHOUGH PLATFORMIO
// //==========================================================
//
// //==========================================================================
// //                              I2C ADDR MAP
// //RTC  = 68
// //
// //==========================================================================
//
// //==========================================================================
// //                              SPI CS MAP
// //SD   = GPIO 23
// //LoRa = GPIO 18
// //ADCs = GPIO 17
// //
// //==========================================================================

#include <Arduino.h>
#include <Wire.h>
#include "SSD1306.h"
// #include "images.h" //ANEEL logo, removed because of license
#include <SPI.h>
#include "log.h"


// Pin definitions
#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x's MOSI
#define SS      18   // GPIO18 -- SX127x's LoRa - CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DI00    26   // GPIO26 -- SX127x's IRQ(Interrupt Request)
#define CSadc   17   // GPIO18 -- ADS8668 - CS
#define RSTadc  32   // GPIO14 -- ADS8668 -  RESET

#define SDA     4    // GPIO4  -- SDA
#define SCL     15   // GPIO15 -- SCL

//Tasks declaration
TaskHandle_t readData;
TaskHandle_t sendData;

//Objects declaration
SSD1306 display(0x3c, SDA, SCL);
Log myLog;
hw_timer_t *timer = NULL;
float dataAVG[5];
int sample_num = 0;

//ADS8668 reading
SPISettings settingsA = SPISettings(8000000, MSBFIRST, SPI_MODE0);
int nDisp;
double data[6][8];

//Variable declaration
int prevSecond = 0; //verify if is a new second
boolean usingSPI = false;
boolean indicativeLED = false;

void ADS8668Init();
double getADS8668Data(int adcNum, int adcCH);
void convertAllData();
double convert2Bytes(byte hiByte, byte loByte);
double adcread5ToCurrent10(double adcread);
double adcread10ToVoltage500(double adcread);
double adcread10ToVoltage400(double adcread);
double adcread5ToVoltage250(double adcread);
double adcread5ToPower9k(double adcread);

//Functions declaration
//Print logo
void logo()
{
  display.clear();
  display.drawXbm(0,5,logo_width,logo_height,logo_bits);
  display.display();
  delay(2000);
}

//Watchdog reset
void IRAM_ATTR resetModule(){
    Serial.println("Watchdog Reboot!\n\n");
    esp_restart();
}

// Tasks implementation

void readDataCode( void * parameter) {
  for(;;) {
    int second = myLog.getSecond();
    if (second !=  prevSecond){
      timerWrite(timer, 0);

      prevSecond = second;
      if(indicativeLED){
        digitalWrite(25, LOW);   // indicative LED
        indicativeLED = false;
      }
      else
      {
        digitalWrite(25, HIGH);   // indicative LED
        indicativeLED = true;
      }
      

      // // Loop to read all data
      // String dataString = "";
      // double data[48];
      // int k = 0;
      // convertAllData();
      // for(int i = 0; i < 6; i++){
      //   for(int j = 0; j < 8; j++){
      //     double adc_data = getADS8668Data(i, j);
      //     if((i < 2) || ((i == 2) && (j<4))){
      //       data[k] = adcread5ToCurrent10(adc_data);
      //     }else if(i < 5){
      //       data[k] = adcread10ToVoltage400(adc_data);
      //       if(i == 2 & j == 5){
      //         data[k] = adcread10ToVoltage500(adc_data);
      //       }
      //     }else{
      //       data[k] = adcread5ToPower9k(adc_data);
      //     }

      //     dataString += String(data[k]) + ",";
      //     Serial.print(data[k]);
      //     Serial.print(", ");
      //     k++;
      //   }
      // }

      //Reading only what I'm using

      while (usingSPI){if(!usingSPI) break; delay(10);}
      usingSPI = true;
      convertAllData();
      usingSPI = false;

      dataAVG[0] += adcread5ToCurrent10(getADS8668Data(0, 0));
      dataAVG[1] += adcread5ToCurrent10(getADS8668Data(0, 1));
      dataAVG[2] += adcread10ToVoltage400(getADS8668Data(2, 4));
      dataAVG[3] += adcread10ToVoltage500(getADS8668Data(2, 5));
      dataAVG[4] += adcread5ToPower9k(getADS8668Data(5, 0));
      sample_num++;

      Serial.print("Reading Data - ");
      Serial.println(second);
      
      if (second == 0 )
      {
        String data_string = "";
        if(sample_num > 0){
          float curr1 = dataAVG[0]/sample_num;
          if(curr1 < 0){
            curr1 = 0;
          }
          if(curr1 > 25.5){
            curr1 = 25.5;
          }
          data_string += String(curr1) + ",";
          float curr2 = dataAVG[1]/sample_num;
          if(curr2 < 0){
            curr2 = 0;
          }
          if(curr2 > 25.5){
            curr2 = 25.5;
          }
          data_string += String(curr2) + ",";
          float volt1 = dataAVG[2]/sample_num;
          if(volt1 < 0){
            volt1 = 0;
          }
          if(volt1 > 6553.5){
            volt1 = 6553.5;
          }
          data_string += String(volt1) + ",";
          float volt2 = dataAVG[3]/sample_num;
          if(volt2 < 0){
            volt2 = 0;
          }
          if(volt2 > 6553.5){
            volt2 = 6553.5;
          }
          data_string += String(volt2) + ",";
          float power = dataAVG[4]/sample_num;
          if(power < -9000){
            power = -9000;
          }
          if(power> 9000){
            power = 9000;
          }
          data_string += String(power);
          
          dataAVG[0] = 0;
          dataAVG[1] = 0;
          dataAVG[2] = 0;
          dataAVG[3] = 0;
          dataAVG[4] = 0;
          sample_num = 0;
        }

        while (usingSPI){if(!usingSPI) break; delay(10);}
        usingSPI = true;
        Serial.println("Saving Data");
        myLog.saveData(data_string);
        usingSPI = false;

      }
    }
  }
}

void sendDataCode( void * parameter) {
  for(;;) {
    delay(5000);

    while (usingSPI){delay(10);}
    usingSPI = true;
    String data_string = myLog.readData();
    Serial.println(data_string);
    usingSPI = false;

    if(data_string != ""){
      Serial.println("Sending data");

      int delimiter[5];
      delimiter[0] = data_string.indexOf(",");
      delimiter[1] = data_string.indexOf(",", delimiter[0]+1);
      delimiter[2] = data_string.indexOf(",", delimiter[1]+1);
      delimiter[3] = data_string.indexOf(",", delimiter[2]+1);
      delimiter[4] = data_string.indexOf(",", delimiter[3]+1);

      long date = data_string.substring(0, delimiter[0]).toInt();
      float curr1 = data_string.substring(delimiter[0]+1, delimiter[1]).toFloat();
      float curr2 = data_string.substring(delimiter[1]+1, delimiter[2]).toFloat();
      float volt1 = data_string.substring(delimiter[2]+1, delimiter[3]).toFloat();
      float volt2 = data_string.substring(delimiter[3]+1, delimiter[4]).toFloat();
      float power = data_string.substring(delimiter[4]+1).toFloat();

      int sent = 0;
      while(!sent){
        while (usingSPI){delay(10);}
        usingSPI = true;
        sent = myLog.dataloggerDataSend(date, curr1, curr2, volt1, volt2, power);
        usingSPI = false;

        if(sent) delay(10);
        else delay(2500);
      }

      while (usingSPI){delay(10);}
      usingSPI = true;
      myLog.removeSentData();
      usingSPI = false;
      Serial.println("Data sent");

    }
  }
}

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\nInitianting Data Logger");

  //Config watchdog 30 seconds
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &resetModule, true);
  timerAlarmWrite(timer, 30000000, true);
  timerAlarmEnable(timer);

  Wire.begin(SDA, SCL); //set I2C pins

  pinMode(25,OUTPUT); //LED pin configuration
  pinMode(16,OUTPUT); //OLED RST pin

  pinMode(SS, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
  digitalWrite(SS, HIGH);
  digitalWrite(MOSI, LOW);
  digitalWrite(SCK, LOW);
  pinMode(RSTadc, OUTPUT);
  pinMode(CSadc, OUTPUT);
  digitalWrite(RSTadc, HIGH);
  digitalWrite(CSadc, HIGH);


  digitalWrite(16, LOW);    //OLED reset
  delay(50);
  digitalWrite(16, HIGH); //while OLED is on, GPIO16 must be HIGH

  timerWrite(timer, 0);

  display.init(); //init display
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10); //configure the text font

  //print ANEEL logo
  // logo();

  delay(1500);
  display.clear();

  timerWrite(timer, 0);

  myLog.init();

  timerWrite(timer, 0);

  display.clear();
  display.drawString(0, 0, "Log Setup success!");
  display.display();

  delay(1500);

  ADS8668Init();

  Serial.println("ADS8668 initialized");
  display.clear();
  display.drawString(0, 0, "ADS8668 initialized");
  display.display();

  timerWrite(timer, 0);

  dataAVG[0] = 0;
  dataAVG[1] = 0;
  dataAVG[2] = 0;
  dataAVG[3] = 0;
  dataAVG[4] = 0;

  delay(1500);
  Serial.println("Setup success!");
  display.clear();
  display.drawString(0, 0, "Setup success!");
  display.display();
  delay(1500);
  display.clear();
  display.drawString(0, 0, "Data Logger Runing!");
  display.display();

  xTaskCreatePinnedToCore(
    readDataCode, /* Function to implement the task */
    "readData", /* Name of the task */
    8192,  /* Stack size in words */
    NULL,  /* Task input parameter */
    2,  /* Priority of the task */
    &readData,  /* Task handle. */
    1); /* Core where the task should run */

  disableCore0WDT();
  xTaskCreatePinnedToCore(
    sendDataCode, /* Function to implement the task */
    "sendData", /* Name of the task */
    8192,  /* Stack size in words */
    NULL,  /* Task input parameter */
    1,  /* Priority of the task */
    &sendData,  /* Task handle. */
    0); /* Core where the task should run */
}

void loop() {}

void ADS8668Init()
{
  nDisp = 6;

  digitalWrite(RSTadc, LOW);
  digitalWrite(RSTadc, HIGH);

  SPI.beginTransaction(settingsA);
  digitalWrite(CSadc, LOW);
  SPI.transfer16(0b0000101100000101);
  for(int i = 0; i < (16*nDisp); i++){
    SPI.transfer(0x00);
    SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);

  SPI.endTransaction();
  SPI.beginTransaction(settingsA);
  digitalWrite(CSadc, LOW);
  SPI.transfer16(0b0000110100000101);
  for(int i = 0; i < (16*nDisp); i++){
    SPI.transfer(0x00);
    SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);
  SPI.endTransaction();

  SPI.beginTransaction(settingsA);
  digitalWrite(CSadc, LOW);
  SPI.transfer16(0b0000111100000101);
  for(int i = 0; i < (16*nDisp); i++){
    SPI.transfer(0x00);
    SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);
  SPI.endTransaction();

  SPI.beginTransaction(settingsA);
  digitalWrite(CSadc, LOW);
  SPI.transfer16(0b0001000100000101);
  for(int i = 0; i < (16*nDisp); i++){
    SPI.transfer(0x00);
    SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);
  SPI.endTransaction();

  SPI.beginTransaction(settingsA);
  digitalWrite(CSadc, LOW);
  SPI.transfer16(0b0001001100000101);
  for(int i = 0; i < (16*nDisp); i++){
    SPI.transfer(0x00);
    SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);
  SPI.endTransaction();

  SPI.beginTransaction(settingsA);
  digitalWrite(CSadc, LOW);
  SPI.transfer16(0b0001010100000101);
  for(int i = 0; i < (16*nDisp); i++){
    SPI.transfer(0x00);
    SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);
  SPI.endTransaction();

  SPI.beginTransaction(settingsA);
  digitalWrite(CSadc, LOW);
  SPI.transfer16(0b0001011100000101);
  for(int i = 0; i < (16*nDisp); i++){
    SPI.transfer(0x00);
    SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);
  SPI.endTransaction();

  SPI.beginTransaction(settingsA);
  digitalWrite(CSadc, LOW);
  SPI.transfer16(0b0001100100000101);
  for(int i = 0; i < (16*nDisp); i++){
    SPI.transfer(0x00);
    SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);
  SPI.endTransaction();

  for(int i = 0; i < nDisp; i++){
    data[i][0] = 0;
    data[i][1] = 0;
    data[i][2] = 0;
    data[i][3] = 0;
    data[i][4] = 0;
    data[i][5] = 0;
    data[i][6] = 0;
    data[i][7] = 0;
  }
}

double getADS8668Data(int adcNum, int adcCH)
{
  return data[adcNum][adcCH];
}

void convertAllData()
{
  byte hiByte[nDisp];
  byte loByte[nDisp];

  SPI.beginTransaction(settingsA);

  digitalWrite(CSadc, LOW);
  SPI.transfer16(0xC000); //Write ch0
  for(int i = nDisp-1; i >= 0; i--){
    SPI.transfer(0x00); //read trash
    SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);

  digitalWrite(CSadc, LOW);
  SPI.transfer16(0xC400); //Write ch1
  for(int i = nDisp-1; i >= 0; i--){
    hiByte[i] = SPI.transfer(0x00); //read ch0
    loByte[i] = SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);

  for(int i = 0; i < nDisp; i++){
    data[i][0] = convert2Bytes(hiByte[i], loByte[i]);
  }

  digitalWrite(CSadc, LOW);
  SPI.transfer16(0xC800); //Write ch2
  for(int i = nDisp-1; i >= 0; i--){
    hiByte[i] = SPI.transfer(0x00); //read ch1
    loByte[i] = SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);

  for(int i = 0; i < nDisp; i++){
    data[i][1] = convert2Bytes(hiByte[i], loByte[i]);
  }

  digitalWrite(CSadc, LOW);
  SPI.transfer16(0xCC00); //Write ch3
  for(int i = nDisp-1; i >= 0; i--){
    hiByte[i] = SPI.transfer(0x00); //read ch2
    loByte[i] = SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);

  for(int i = 0; i < nDisp; i++){
    data[i][2] = convert2Bytes(hiByte[i], loByte[i]);
  }

  digitalWrite(CSadc, LOW);
  SPI.transfer16(0xD000); //Write ch4
  for(int i = nDisp-1; i >= 0; i--){
    hiByte[i] = SPI.transfer(0x00); //read ch3
    loByte[i] = SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);

  for(int i = 0; i < nDisp; i++){
    data[i][3] = convert2Bytes(hiByte[i], loByte[i]);
  }

  digitalWrite(CSadc, LOW);
  SPI.transfer16(0xD400); //Write ch5
  for(int i = nDisp-1; i >= 0; i--){
    hiByte[i] = SPI.transfer(0x00); //read ch4
    loByte[i] = SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);

  for(int i = 0; i < nDisp; i++){
    data[i][4] = convert2Bytes(hiByte[i], loByte[i]);
  }

  digitalWrite(CSadc, LOW);
  SPI.transfer16(0xD800); //Write ch6
  for(int i = nDisp-1; i >= 0; i--){
    hiByte[i] = SPI.transfer(0x00); //read ch5
    loByte[i] = SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);

  for(int i = 0; i < nDisp; i++){
    data[i][5] = convert2Bytes(hiByte[i], loByte[i]);
  }

  digitalWrite(CSadc, LOW);
  SPI.transfer16(0xDC00); //Write ch7
  for(int i = nDisp-1; i >= 0; i--){
    hiByte[i] = SPI.transfer(0x00); //read ch6
    loByte[i] = SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);

  for(int i = 0; i < nDisp; i++){
    data[i][6] = convert2Bytes(hiByte[i], loByte[i]);
  }

  digitalWrite(CSadc, LOW);
  SPI.transfer16(0xC000); //Write ch0
  for(int i = nDisp-1; i >= 0; i--){
    hiByte[i] = SPI.transfer(0x00); //read ch7
    loByte[i] = SPI.transfer(0x00);
  }
  digitalWrite(CSadc, HIGH);

  for(int i = 0; i < nDisp; i++){
    data[i][7] = convert2Bytes(hiByte[i], loByte[i]);
  }
  SPI.endTransaction();
}

double convert2Bytes(byte hiByte, byte loByte)
{
  int adcRead = (hiByte<<4) + (loByte>>4);
  double value = adcRead*0.0025;
  return value;
}

double adcread5ToCurrent10(double adcread)
{
  // return (adcread*10.0/5.0);
  return (adcread*myLog.getSettings()[0]);
}

double adcread10ToVoltage400(double adcread)
{
  // return (adcread*400.0/5.0);
  // return adcread*69.4;
  return (adcread*myLog.getSettings()[1]);
}

double adcread10ToVoltage500(double adcread)
{
  // return (adcread*500.0/5.0);
  // return adcread*97.3;
  return (adcread*myLog.getSettings()[2]);
}

double adcread5ToVoltage250(double adcread)
{
  return (adcread*250.0/5.0);
}

double adcread5ToPower9k(double adcread)
{
  // return ((adcread-2.5)*9000.0/2.5);
  // return -((adcread*3589.9)-9000.0);
  return -((adcread*myLog.getSettings()[3])-9000.0);
}
