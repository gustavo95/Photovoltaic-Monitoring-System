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
* PURPOSE     : Code responsible for data storage and transmission
*****************************************************************************/

#include <RTClib.h>
#include <SD.h>
#include <SPI.h>
#include <LoRa.h>
#include "SSD1306.h"
#include "DataEncDec.h"

// Pin definitions
#define SDPIN   17   // GPIO17 -- SD - CS 17(satation)
// #define SDPIN   23   // GPIO17 -- SD - CS 23(pv datalogger)
#define SCK     5    // GPIO5  -- SCK
#define MISO    19   // GPIO19 -- MISO
#define MOSI    27   // GPIO27 -- MOSI
#define SS      18   // GPIO18 -- LoRa - CS
#define RST     14   // GPIO14 -- RESET
#define DI00    26   // GPIO26 -- IRQ(Interrupt Request)
#define SDA     4    // GPIO4  -- SDA
#define SCL     15   // GPIO15 -- SCL

#define dataPath "/data_buffer.csv"
#define auxPath "/aux.csv"
#define settingsPath "/settings.csv"
#define dataHeader "DIA,MES,ANO,HORA,MINUTO,SEGUNDO,TEMPERATURA,PRESSAO,UMIDADE,IRRADIANCIA,VELOCIDADE,DIRECAO,CHUVA,PVTEMP,TENSAO,CORRENTE\n"
#define BAND    915E6  //Radio frequency - 433E6, 868E6, 915E6

#define ThisDevice STATION

#define INTERVAL 500

class Log
{
private:
  //Log Objects
  RTC_DS1307 rtc;
  DataEncDec* decoder;

  //Log variables
  DateTime now;
  long lastSendTime = 0;
  boolean usingRTC = false;
  float transducer_settings[4] = {2, 40, 50, 3600};


  //Log functions
public:
  void init();
  void setTime(int year, int month, int day, int hour, int min, int sec);
  String getTime();
  int getYear();
  int getMonth();
  int getDay();
  int getSecond();
  int getMin();
  int getHour();
  float *getSettings();
  bool saveData(String data);
  String readData();
  void removeSentData();
  int stationDataSend(long date, float amb_temp, int humi, float irrad, float w_spe, int w_dir, float rain, float pv_temp);
  int dataloggerDataSend(long date, float curr1, float curr2, float volt1, float volt2, float power);

private:
  int sendPacket(char* buffer, int len);
  int receive();

  //File functions
  void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
  void createDir(fs::FS &fs, const char * path);
  void removeDir(fs::FS &fs, const char * path);
  void readFile(fs::FS &fs, const char * path);
  String readFileLine(fs::FS &fs, const char * path);
  bool fileExists(fs::FS &fs, const char * path);
  void writeFile(fs::FS &fs, const char * path, const char * message);
  void appendFile(fs::FS &fs, const char * path, const char * message);
  void renameFile(fs::FS &fs, const char * path1, const char * path2);
  void deleteFile(fs::FS &fs, const char * path);
  void removeFileLine(fs::FS &fs, const char * path);
  void testFileIO(fs::FS &fs, const char * path);
};
