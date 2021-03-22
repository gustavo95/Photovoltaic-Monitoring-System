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
* PURPOSE     : Main code for LoRa gateway
*****************************************************************************/

// //==========================================================
// // ALL LIBS MUST BE INSTALLED TRHOUGH PLATFORMIO
// //==========================================================

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <SSD1306.h>
#include <DataEncDec.h>
#include "esp32-mqtt.h"
#include <RTClib.h>

// Pin definitions 
#define SCK 5   // GPIO5  SCK
#define MISO 19 // GPIO19 MISO
#define MOSI 27 // GPIO27 MOSI
#define SS 18   // GPIO18 CS
#define RST 14  // GPIO14 RESET
#define DI00 26 // GPIO26 IRQ(Interrupt Request)
 
#define BAND 915E6 //Frequência do radio - exemplo : 433E6, 868E6, 915E6
 
//Objects declaration
SSD1306 display(0x3c, 4, 15);
DataEncDec decoder(0);
hw_timer_t *timer = NULL;

//Variable declaration
float settings[6] = {2, 200, 2, 40, 50, 3600};
int settingsStation = 0;
int settingsDatalogger = 0;
long lastStationData = 0;
long lastDLData = 0;

//Menssage handler
void messageReceived(String &topic, String &payload) {
  Serial.println("\n\nIncoming: " + topic + " - " + payload + "\n\n");
  if(topic == "/devices/DL-Node-1/commands"){
    publishState("{\"LOG\": \"Recived command: " + payload +"\"}");
    delay(2000);

    //Reset
    if(payload[0] == 'R'){
      esp_restart();
    }
  }
  if(topic == "/devices/DL-Node-1/config"){
    // publishState("{\"LOG\": \"Recived config: " + payload +"\"}"); // bug??

    settings[0] = (float)(payload[0])*1000 + (float)(payload[1])*100 + (float)(payload[2])*10 + (float)(payload[3]) + (float)(payload[4])*0.1 - 53332.8;
    settings[1] = (float)(payload[6])*1000 + (float)(payload[7])*100 + (float)(payload[8])*10 + (float)(payload[9]) + (float)(payload[10])*0.1 - 53332.8;
    settings[2] = (float)(payload[12])*1000 + (float)(payload[13])*100 + (float)(payload[14])*10 + (float)(payload[15]) + (float)(payload[16])*0.1 - 53332.8;
    settings[3] = (float)(payload[18])*1000 + (float)(payload[19])*100 + (float)(payload[20])*10 + (float)(payload[21]) + (float)(payload[22])*0.1 - 53332.8;
    settings[4] = (float)(payload[24])*1000 + (float)(payload[25])*100 + (float)(payload[26])*10 + (float)(payload[27]) + (float)(payload[28])*0.1 - 53332.8;
    settings[5] = (float)(payload[30])*1000 + (float)(payload[31])*100 + (float)(payload[32])*10 + (float)(payload[33]) + (float)(payload[34])*0.1 - 53332.8;

    settingsStation = 1;
    settingsDatalogger = 1;
  }
}

void setupLoRa(){ 
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI00);
  digitalWrite(RST, LOW);
  digitalWrite(RST, HIGH);
 
  if (!LoRa.begin(BAND)){
    display.clear();
    display.drawString(0, 0, "Erro ao inicializar o LoRa!");
    display.display();
    Serial.println("Erro ao inicializar o LoRa!");
    delay(3000);
    esp_restart();
  }
 
  LoRa.enableCrc();
  // LoRa.receive();

  //US902_928 { 903900000, 125, 7, 10, 923300000, 500, 7, 12}
  //AU925_928 { 916800000, 125, 7, 10, 916800000, 125, 7, 12}

  LoRa.setSignalBandwidth(125E3); //7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, 500E3
  LoRa.setSpreadingFactor(7); //range: 6 to 12
  LoRa.setTxPower(10); //range: 2 to 20
  LoRa.setSyncWord(0x12); // default: 0x12
}

//Watchdog reset
void IRAM_ATTR resetModule(){
    Serial.println("Watchdog Reboot!\n\n");
    esp_restart();
}

void setup(){
    Serial.begin(115200);

    //Config watchdog 2 min and 30 sec
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &resetModule, true);
    timerAlarmWrite(timer, 150000000, true);
    timerAlarmEnable(timer);

    pinMode(25, OUTPUT); //LED pin configuration
    pinMode(16, OUTPUT); //OLED RST pin

    digitalWrite(16, LOW); //OLED Reset
    delay(50);
    digitalWrite(16, HIGH);

    display.init();
    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    display.clear();
    display.drawString(0, 0, "Slave esperando...");
    display.display();
    Serial.println("Slave esperando...");
    //Configuring the LoRa radio
    setupLoRa();

    setupCloudIoT();
    delay(1000);
    mqtt->loop();
    delay(10);  // <- fixes some issues with WiFi stability
    if (!mqttClient->connected()) {
      connect();
      delay(500);
    }
    Serial.println("CloudIoT initialized");
}

void sendACK(uint8_t to){
  int size = 5;
  if((to == STATION && settingsStation) || (to == DATALOGGER && settingsDatalogger)){
    size = 14;
  }
  DataEncDec encoder(size);
  encoder.reset();

  if((to == STATION && settingsStation) || (to == DATALOGGER && settingsDatalogger)){
    encoder.addHeader(GATEWAY, to, 1, 1);
  }
  else{
    encoder.addHeader(GATEWAY, to, 1, 0);
  }

  configTime(0, 0, ntp_primary, ntp_secondary);
  DateTime now = time(NULL)-10800;
  encoder.addDate(now.unixtime());

  if(to == STATION && settingsStation){
    encoder.addVoltage(settings[0]);
    encoder.addVoltage(settings[1]);
    encoder.addVoltage(0);
    encoder.addPower(0);
    settingsStation = 0;
  }
  if(to == DATALOGGER && settingsDatalogger){
    encoder.addVoltage(settings[2]);
    encoder.addVoltage(settings[3]);
    encoder.addVoltage(settings[4]);
    encoder.addPower(settings[5]);
    settingsDatalogger = 0;
  }

  char* buffer = encoder.getBuffer();
  LoRa.beginPacket();
  for(int i = 0; i < size; i++){
    LoRa.print(buffer[i]);
  }
  LoRa.endPacket();
}

void readStationData(char* received){
  DateTime now = decoder.getDate(received[1], received[2], received[3], received[4]);
  float temp = decoder.getTemp(received[5], received[6]);
  int humi = decoder.getHumi(received[7]);
  float irrad = decoder.getIrrad(received[8], received[9]);
  float windSpeed = decoder.getWindSpeed(received[10]);
  int windDirection = decoder.getWindDirection(received[11]);
  float rain = decoder.getRain(received[12]);
  float pvtemp = decoder.getTemp(received[13], received[14]);

  Serial.println(now.year());
  Serial.println(now.month());
  Serial.println(now.day());
  Serial.println(now.hour());
  Serial.println(now.minute());
  Serial.println(now.second());
  Serial.println(temp);
  Serial.println(humi);
  Serial.println(irrad);
  Serial.println(windSpeed);
  Serial.println(windDirection);
  Serial.println(rain);
  Serial.println(pvtemp);

  if(lastStationData != now.unixtime()){
    mqtt->loop();
    delay(10);  // <- fixes some issues with WiFi stability
    if (!mqttClient->connected()) {
      connect();
      delay(500);
    }

    bool sent = publishTelemetry("/station",
            "{\"DATE\": \""+String(now.year())+"-"+String(now.month())+"-"+String(now.day())+
            "\",\"TIME\": \""+String(now.hour())+":"+String(now.minute())+":"+String(now.second())+
            "\",\"AMB_TEMPERATURE\": "+String(temp)+
            ",\"PRESSURE\": 0"+
            ",\"HUMIDITY\": "+String(humi)+
            ",\"IRRADIANCE\": "+String(irrad)+
            ",\"WIND_SPEED\": "+String(windSpeed)+
            ",\"WIND_DIRECTION\": "+String(windDirection)+
            ",\"RAIN\": "+String(rain)+
            ",\"PV_TEMPERATURE\": "+String(pvtemp)+"}");

    display.clear();
    display.drawString(0, 0, "Station data received");
    display.drawString(0,20, String(now.year())+"-"+String(now.month())+"-"+String(now.day())+
                              " "+String(now.hour())+":"+String(now.minute())+":"+String(now.second()));
    display.display();

    //fixes some issues with LoRA stability
    if (sent){
      sendACK(STATION);
      lastStationData = now.unixtime();
    }
    else esp_restart();
  }
  else{
    setupLoRa();
    sendACK(STATION);
  }
}

void readDataLoggerData(char* received){
  DateTime now = decoder.getDate(received[1], received[2], received[3], received[4]);
  float current1 = decoder.getCurrent(received[5]);
  float current2 = decoder.getCurrent(received[6]);
  float voltage1 = decoder.getVoltage(received[7], received[8]);
  float voltage2 = decoder.getVoltage(received[9], received[10]);
  float power = decoder.getPower(received[11], received[12], received[13]);

  Serial.println(now.year());
  Serial.println(now.month());
  Serial.println(now.day());
  Serial.println(now.hour());
  Serial.println(now.minute());
  Serial.println(now.second());
  Serial.println(current1);
  Serial.println(current2);
  Serial.println(voltage1);
  Serial.println(voltage2);
  Serial.println(power);

  if(lastDLData != now.unixtime()){
    mqtt->loop();
    delay(10);  // <- fixes some issues with WiFi stability
    if (!mqttClient->connected()) {
      connect();
      delay(500);
    }

    bool sent = publishTelemetry("/datalogger",
        "{\"DATE\": \""+String(now.year())+"-"+String(now.month())+"-"+String(now.day())+
        "\",\"TIME\": \""+String(now.hour())+":"+String(now.minute())+":"+String(now.second())+
        "\",\"ADC00\": "+String(current1)+
        ",\"ADC01\": "+String(current2)+
        /*",\"ADC02\": "+data[2]+
        ",\"ADC03\": "+data[3]+
        ",\"ADC04\": "+data[4]+
        ",\"ADC05\": "+data[5]+
        ",\"ADC06\": "+data[6]+
        ",\"ADC07\": "+data[7]+
        ",\"ADC10\": "+data[8]+
        ",\"ADC11\": "+data[9]+
        ",\"ADC12\": "+data[10]+
        ",\"ADC13\": "+data[11]+
        ",\"ADC14\": "+data[12]+
        ",\"ADC15\": "+data[13]+
        ",\"ADC16\": "+data[14]+
        ",\"ADC17\": "+data[15]+
        ",\"ADC20\": "+data[16]+
        ",\"ADC21\": "+data[17]+
        ",\"ADC22\": "+data[18]+
        ",\"ADC23\": "+data[19]+*/
        ",\"ADC24\": "+String(voltage1)+
        ",\"ADC25\": "+String(voltage2)+
        ",\"ADC26\": 0"+
        /*",\"ADC27\": "+data[23]+
        ",\"ADC30\": "+data[24]+
        ",\"ADC31\": "+data[25]+
        ",\"ADC32\": "+data[26]+
        ",\"ADC33\": "+data[27]+
        ",\"ADC34\": "+data[28]+
        ",\"ADC35\": "+data[29]+
        ",\"ADC36\": "+data[30]+
        ",\"ADC37\": "+data[31]+
        ",\"ADC40\": "+data[32]+
        ",\"ADC41\": "+data[33]+
        ",\"ADC42\": "+data[34]+
        ",\"ADC43\": "+data[35]+
        ",\"ADC44\": "+data[36]+
        ",\"ADC45\": "+data[37]+
        ",\"ADC46\": "+data[38]+
        ",\"ADC47\": "+data[39]+*/
        ",\"ADC50\": "+String(power)+
        /*",\"ADC51\": "+data[41]+
        ",\"ADC52\": "+data[42]+
        ",\"ADC53\": "+data[43]+
        ",\"ADC54\": "+data[44]+
        ",\"ADC55\": "+data[45]+
        ",\"ADC56\": "+data[46]+
        ",\"ADC57\": "+data[47]+*/
        "}");

    display.clear();
    display.drawString(0, 0, "Data logger data received");
    display.drawString(0,20, String(now.year())+"-"+String(now.month())+"-"+String(now.day())+
                              " "+String(now.hour())+":"+String(now.minute())+":"+String(now.second()));
    display.display();

    //fixes some issues with LoRA stability
    if (sent){
      sendACK(DATALOGGER);
      lastDLData = now.unixtime();
    }
    else esp_restart();
  }
  else{
    setupLoRa();
    sendACK(STATION);
  }
}

void loop(){
  int packetSize = LoRa.parsePacket();
 
  if (packetSize > 0){
    char received[packetSize];
    int cursor = 0;
 
    while(LoRa.available()){
      received[cursor] = (char) LoRa.read();
      cursor++;
    }

    if(decoder.getTo(received[0]) == GATEWAY){
      timerWrite(timer, 0);
      if(decoder.getFrom(received[0]) == STATION){
        digitalWrite(25, HIGH);   // indicative LED

        Serial.println("Station data received");

        readStationData(received);
        digitalWrite(25, LOW);   // indicative LED
      }

      if(decoder.getFrom(received[0]) == DATALOGGER){
        digitalWrite(25, HIGH);   // indicative LED

        Serial.println("Data Logger data received");

        readDataLoggerData(received);
        digitalWrite(25, LOW);   // indicative LED
      }
    }
  }
}