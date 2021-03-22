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

#include "log.h"

void Log::init()
{

  decoder = new DataEncDec(0);

  try
  {
    SPI.begin(SCK,MISO,MOSI);
    LoRa.setPins(SS,RST,DI00);
    if (!LoRa.begin(BAND))
    {
      throw;
    }
    LoRa.enableCrc();
    // LoRa.receive();
    LoRa.setSignalBandwidth(125E3);
    LoRa.setSpreadingFactor(7);
    LoRa.setTxPower(10);
    LoRa.setSyncWord(0x12);
  }
  catch(String e){
    throw "LoRa initialization error";
  }

  Serial.println("LoRa initialized");

  try
  {
    if (! rtc.begin()) {
      throw "DS1307 not found!";
    }

    if (!rtc.isrunning()) {
      //throw "DS1307 not running!";
      //Update RTC time
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //CAPTURA A DATA E HORA EM QUE O SKETCH É COMPILADO
      //rtc.adjust(DateTime(2019, 2, 6, 17, 35, 00)); //(ANO), (MÊS), (DIA), (HORA), (MINUTOS), (SEGUNDOS)
    }
    //rtc.adjust(DateTime(2020, 12, 31, 11, 25, 00));
    now = rtc.now();
    Serial.print(now.year());
  }
  catch(String e)
  {
    while (1) {
      Serial.println(e);
      delay(5000);
    }
  }

  Serial.println("RTC initialized");

  try
  {
    if(!SD.begin(SDPIN)){
        throw "Card Mount Failed";
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        throw "No SD card attached";
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
  }
  catch(String e){
    while(1)
    {
      Serial.println(e);
      delay(5000);
    }
  }

  Serial.println("SD initialized");

  //Creating data buffer file
  if(!fileExists(SD, dataPath))
  {
    writeFile(SD, dataPath, "");
  }
  //Creating settings file or reading
  if(!fileExists(SD, settingsPath))
  {
    writeFile(SD, settingsPath, "2.0,200.0,0.0,0.0\n");
  }
  else{
    String settings = readFileLine(SD, settingsPath);
    int delimiter[3];
    delimiter[0] = settings.indexOf(",");
    delimiter[1] = settings.indexOf(",", delimiter[0]+1);
    delimiter[2] = settings.indexOf(",", delimiter[1]+1);

    transducer_settings[0] = settings.substring(0, delimiter[0]).toFloat();
    transducer_settings[1] = settings.substring(delimiter[0]+1, delimiter[1]).toFloat();
    transducer_settings[2] = settings.substring(delimiter[1]+1, delimiter[2]).toFloat();
    transducer_settings[3] = settings.substring(delimiter[2]+1).toFloat();
  }
}

void Log::setTime(int year, int month, int day, int hour, int min, int sec)
{
  while (usingRTC){delay(10);}
  usingRTC = true;
  rtc.adjust(DateTime(year, month, day, hour, min, sec));
  usingRTC = false;
}

String Log::getTime()
{
  while (usingRTC){delay(10);}
  usingRTC = true;
  now = rtc.now();
  usingRTC = false;

  String time = String(now.unixtime());

  return time;
}

int Log::getYear()
{
  while (usingRTC){delay(10);}
  usingRTC = true;
  now = rtc.now();
  usingRTC = false;
  return now.year();
}

int Log::getMonth()
{
  while (usingRTC){delay(10);}
  usingRTC = true;
  now = rtc.now();
  usingRTC = false;
  return now.month();
}

int Log::getDay()
{
  while (usingRTC){delay(10);}
  usingRTC = true;
  now = rtc.now();
  usingRTC = false;
  return now.day();
}

int Log::getSecond()
{
  while (usingRTC){delay(10);}
  usingRTC = true;
  now = rtc.now();
  usingRTC = false;
  return now.second();
}

int Log::getMin()
{
  while (usingRTC){delay(10);}
  usingRTC = true;
  now = rtc.now();
  usingRTC = false;
  return now.minute();
}

int Log::getHour()
{
  while (usingRTC){delay(10);}
  usingRTC = true;
  now = rtc.now();
  usingRTC = false;
  return now.hour();
}

float *Log::getSettings(){
  return transducer_settings;
}

bool Log::saveData(String data)
{
  bool newDay = false;
  String dataString = getTime() + "," + data + "\n";

  appendFile(SD, dataPath, dataString.c_str());

  //Return TRUE if is next second is a new day to reset rain counter
  if(now.hour() >= 23 && now.minute() >= 59 && now.second() >= 59)
  {
    newDay = true;
  }

  Serial.println(dataString);

  return newDay;
}

String Log::readData(){
  return readFileLine(SD, dataPath);
}

void Log::removeSentData(){
  removeFileLine(SD, dataPath);
  return;
}

int Log::stationDataSend(long date, float amb_temp, int humi, float irrad, float w_spe, int w_dir, float rain, float pv_temp){
  DataEncDec encoder(15); // 1 + 4 + 2 + 1 + 2 + 1 + 1 + 1 + 2 = 15
  encoder.addHeader(ThisDevice, GATEWAY);
  encoder.addDate(date);
  encoder.addTemp(amb_temp);
  encoder.addHumi(humi);
  encoder.addIrrad(irrad);
  encoder.addWindSpeed(w_spe);
  encoder.addWindDirection(w_dir);
  encoder.addRain(rain);
  encoder.addTemp(pv_temp);

  return sendPacket(encoder.getBuffer(), encoder.getSize());
}

int Log::dataloggerDataSend(long date, float curr1, float curr2, float volt1, float volt2, float power){
  DataEncDec encoder(14); // 1 + 4 + 1 + 1 + 2 + 2 + 3 = 14
  encoder.addHeader(ThisDevice, GATEWAY);
  encoder.addDate(date);
  encoder.addCurrent(curr1);
  encoder.addCurrent(curr2);
  encoder.addVoltage(volt1);
  encoder.addVoltage(volt2);
  encoder.addPower(power);

  return sendPacket(encoder.getBuffer(), encoder.getSize());
}

int Log::sendPacket(char* buffer, int len){
  lastSendTime = millis();

  LoRa.beginPacket();
  for(int i = 0; i < len; i++){
    LoRa.print(buffer[i]);
  }
  LoRa.endPacket();

  Serial.println("Wating ack");
  while ((millis() - lastSendTime) < INTERVAL)
  {
    if (receive()){
      return 1;
    }
  }

  return 0;
}

int Log::receive(){
  int packetSize = LoRa.parsePacket();
   
  if (packetSize > 0){
    char received[packetSize];
    int cursor = 0;

    while(LoRa.available()){
      received[cursor] = (char) LoRa.read();
      cursor++;
    }

    if((decoder->getTo(received[0]) == ThisDevice) && decoder->getACK(received[0])){
      DateTime now_update = decoder->getDate(received[1], received[2], received[3], received[4]);
      setTime(now_update.year(), now_update.month(), now_update.day(), now_update.hour(), now_update.minute(), now_update.second());
      if(decoder->getSettings(received[0])){
        transducer_settings[0] = decoder->getVoltage(received[5], received[6]);
        transducer_settings[1] = decoder->getVoltage(received[7], received[8]);
        transducer_settings[2] = decoder->getVoltage(received[9], received[10]);
        transducer_settings[3] = decoder->getPower(received[11], received[12], received[13]);
        Serial.println(transducer_settings[0]);
        Serial.println(transducer_settings[1]);
        writeFile(SD, settingsPath, (String(transducer_settings[0])+","+String(transducer_settings[1])+
                  ","+String(transducer_settings[2])+ "," +String(transducer_settings[3])+"\n").c_str());
        Serial.println("Settings updated");
      }
      return 1;
    }
  }
  return 0;
}

//==============================================================
// SD FUNCTIONS
//==============================================================
void Log::listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
      Serial.println("Failed to open directory");
      return;
  }
  if(!root.isDirectory()){
      Serial.println("Not a directory");
      return;
  }

  File file = root.openNextFile();
  while(file){
      if(file.isDirectory()){
          Serial.print("  DIR : ");
          Serial.println(file.name());
          if(levels){
              listDir(fs, file.name(), levels -1);
          }
      } else {
          Serial.print("  FILE: ");
          Serial.print(file.name());
          Serial.print("  SIZE: ");
          Serial.println(file.size());
      }
      file = root.openNextFile();
  }
}

void Log::createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
      Serial.println("Dir created");
  } else {
      Serial.println("mkdir failed");
  }
}

void Log::removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
      Serial.println("Dir removed");
  } else {
      Serial.println("rmdir failed");
  }
}

void Log::readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
      Serial.println("Failed to open file for reading");
      return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
      Serial.write(file.read());
  }
  file.close();
}

String Log::readFileLine(fs::FS &fs, const char * path){
  String line = "";
  char aux;

  File file = fs.open(path);
  if(!file){
      Serial.println("Failed to open file for reading");
      return line;
  }
  
  while(file.available()){
      aux = (char) file.read();
      if(aux == '\n'){
        break;
      }
      else{
        line += aux;
      }
  }

  file.close();

  return line;
}

bool Log::fileExists(fs::FS &fs, const char * path){

  File file = fs.open(path);
  if(!file){
      return false;
  }

  file.close();
  return true;
}

void Log::writeFile(fs::FS &fs, const char * path, const char * message){
  //Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
      Serial.println("Failed to open file for writing");
      return;
  }
  file.print(message);
  // if(file.print(message)){
  //     //Serial.println("File written");
  // } else {
  //     Serial.println("Write failed");
  // }
  file.close();
}

void Log::appendFile(fs::FS &fs, const char * path, const char * message){
  //Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
      Serial.println("Failed to open file for appending");
      return;
  }
  if(file.print(message)){
      //Serial.println("Message appended");
  } else {
      Serial.println("Append failed");
  }
  file.close();
}

void Log::renameFile(fs::FS &fs, const char * path1, const char * path2){
  //Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
      // Serial.println("File renamed");
  } else {
      Serial.println("Rename failed");
  }
}

void Log::deleteFile(fs::FS &fs, const char * path){
  //Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
      // Serial.println("File deleted");
  } else {
      Serial.println("Delete failed");
  }
}

void Log::removeFileLine(fs::FS &fs, const char * path){

  File mainFile = fs.open(path, FILE_READ);
  if(!mainFile){
      Serial.println("Failed to open file for reading");
      return;
  }

  File auxFile = fs.open(auxPath, FILE_WRITE);
  if(!mainFile){
      Serial.println("Failed to open file for reading");
      return;
  }

  char aux;

  while(mainFile.available()){
      aux = (char) mainFile.read();
      if(aux == '\n'){
        break;
      }
  }

  while(mainFile.available()){
      aux = (char) mainFile.read();
      auxFile.print(aux);
  }

  auxFile.close();
  mainFile.close();

  deleteFile(fs, path);
  renameFile(fs, auxPath, path);

  return;
}

void Log::testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
      len = file.size();
      size_t flen = len;
      start = millis();
      while(len){
          size_t toRead = len;
          if(toRead > 512){
              toRead = 512;
          }
          file.read(buf, toRead);
          len -= toRead;
      }
      end = millis() - start;
      Serial.printf("%u bytes read for %u ms\n", flen, end);
      file.close();
  } else {
      Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if(!file){
      Serial.println("Failed to open file for writing");
      return;
  }

  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
      file.write(buf, 512);
  }

  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();

  return;
}
