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
* PURPOSE     : Main code for solarimetric station data logger
*****************************************************************************/

// //==========================================================
// // ALL LIBS MUST BE INSTALLED TRHOUGH PLATFORMIO
// //==========================================================
//
// //==========================================================================
// //                              I2C ADDR MAP
// //BME   =  76
// //RTC   =  68
// //ADC's =  48 49 4A 4B
// //
// //==========================================================================
//
// //==========================================================================
// //                              SPI CS MAP
// //SD   = GPIO 17
// //LoRa = GPIO 18
// //
// //==========================================================================

#include <Arduino.h>
#include <Wire.h>
#include "SSD1306.h"
// #include "images.h" //ANEEL logo, removed because of license
#include "sensors.h"
#include "log.h"

// Pin definitions
#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x's MOSI
#define SS      18   // GPIO18 -- SX127x's LoRa - CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DI00    26   // GPIO26 -- SX127x's IRQ(Interrupt Request)

#define SDA     4    // GPIO4  -- SDA
#define SCL     15   // GPIO15 -- SCL

//Tasks declaration
TaskHandle_t readData;
TaskHandle_t sendData;

//Objects declaration
SSD1306 display(0x3c, SDA, SCL);
Sensors mySensors;
Log myLog;
hw_timer_t *timer = NULL;

//Variable declaration
int prevSecond = 0; //verify if is a new second
// unsigned long data_send = 1; //couter for sent packets
// unsigned long operating_hours = 0; //counter for device operating hours
boolean usingSPI = false;

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

      digitalWrite(25, HIGH);   // indicative LED
      prevSecond = second;

      mySensors.readAllData(myLog.getSettings()[0], myLog.getSettings()[1]);
      Serial.print("Reading Data - ");
      Serial.println(second);
      
      if (second == 0 )
      {
        while (usingSPI){if(!usingSPI) break; delay(10);}
        usingSPI = true;
        Serial.println("Saving Data");
        bool newDay = myLog.saveData(mySensors.getAvgData());
        if(newDay){
          mySensors.setPluvCounter0();
        }
        usingSPI = false;
      }

      digitalWrite(25, LOW);   // indicative LED
    }
  }
}

void sendDataCode( void * parameter) {
  for(;;) {
    delay(5000);

    while (usingSPI){delay(10);}
    usingSPI = true;
    String data = myLog.readData();
    Serial.println(data);
    usingSPI = false;

    if(data != ""){
      Serial.println("Sending data");

      int delimiter[9];
      delimiter[0] = data.indexOf(",");
      delimiter[1] = data.indexOf(",", delimiter[0]+1);
      delimiter[2] = data.indexOf(",", delimiter[1]+1);
      delimiter[3] = data.indexOf(",", delimiter[2]+1);
      delimiter[4] = data.indexOf(",", delimiter[3]+1);
      delimiter[5] = data.indexOf(",", delimiter[4]+1);
      delimiter[6] = data.indexOf(",", delimiter[5]+1);
      delimiter[7] = data.indexOf(",", delimiter[6]+1);
      delimiter[8] = data.indexOf(",", delimiter[7]+1);

      long date = data.substring(0, delimiter[0]).toInt();
      float amb_temp = data.substring(delimiter[0]+1, delimiter[1]).toFloat();
      int humi = data.substring(delimiter[1]+1, delimiter[2]).toInt();
      float irrad = data.substring(delimiter[2]+1, delimiter[3]).toFloat();
      float w_spe = data.substring(delimiter[3]+1, delimiter[4]).toFloat();
      int w_dir = data.substring(delimiter[4]+1, delimiter[5]).toInt();
      float rain = data.substring(delimiter[5]+1, delimiter[6]).toFloat();
      float pv_temp = data.substring(delimiter[6]+1, delimiter[7]).toFloat();

      int sent = 0;
      while(!sent){
        while (usingSPI){delay(10);}
        usingSPI = true;
        sent = myLog.stationDataSend(date, amb_temp, humi, irrad, w_spe, w_dir, rain, pv_temp);
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

  Serial.println("\n\nInitianting Solarimetric Station");

  //Config watchdog 30 seconds
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &resetModule, true);
  timerAlarmWrite(timer, 30000000, true);
  timerAlarmEnable(timer);

  Wire.begin(SDA, SCL); //set I2C pins

  pinMode(25, OUTPUT); //LED pin configuration
  pinMode(16, OUTPUT); //OLED RST pin

  digitalWrite(16, LOW);    //OLED reset
  delay(50);
  digitalWrite(16, HIGH); //while OLED is on, GPIO16 must be HIGH

  timerWrite(timer, 0);

  display.init(); //init display
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10); //configure the text font

  //print ANEEL logo
  // logo();

  timerWrite(timer, 0);

  delay(1500);
  display.clear();

  timerWrite(timer, 0);

  myLog.init();

  timerWrite(timer, 0);

  display.clear();
  display.drawString(0, 0, "Log Setup success!");
  display.display();
  delay(1500);

  mySensors.init();

  timerWrite(timer, 0);

  display.clear();
  display.drawString(0, 0, "Sensors Setup success!");
  display.display();
  delay(1500);
  Serial.println("Setup success!");
  display.clear();
  display.drawString(0, 0, "Setup success!");
  display.display();
  delay(1500);
  display.clear();
  display.drawString(0, 0, "Station Runing!");
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

void loop(){}
