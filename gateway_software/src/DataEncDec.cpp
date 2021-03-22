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
* PURPOSE     : Code to compress and decompress data using bit-packing
*****************************************************************************/

#include "DataEncDec.h"

DataEncDec::DataEncDec(uint8_t size) : maxsize(size){
    buffer = (char*) malloc(size);
    cursor = 0;
}

DataEncDec::~DataEncDec(void){
    free(buffer);
}

void DataEncDec::reset(void){
    cursor = 0;
}

uint8_t DataEncDec::getSize(void){
    return cursor;
}

char* DataEncDec::getBuffer(void){
//    uint8_t[cursor] result;
//    memcpy(result, buffer, cursor);
//    return result;
    return buffer;
}

uint8_t DataEncDec::copy(uint8_t* dst){
    memcpy(dst, buffer, cursor);
    return cursor;
}

float DataEncDec::newPrecision(float n, int i){ 
    return round(pow(10,i)*n)/pow(10,i); 
}

uint8_t DataEncDec::addHeader(uint8_t from, uint8_t to){
    if ((cursor + 1) > maxsize){
        return 0;
    } 
    uint8_t ack = 0;
    uint8_t settings = 0;
                    // Recipient    Sender       Acknowledgement
    buffer[cursor++] = (to << 6) | (from << 4) | (settings << 1) | ack;

    return cursor;
}

uint8_t DataEncDec::addHeader(uint8_t from, uint8_t to, uint8_t ack){
    if ((cursor + 1) > maxsize){
        return 0;
    } 
    uint8_t settings = 0;
                    // Recipient    Sender       Acknowledgement
    buffer[cursor++] = (to << 6) | (from << 4) | (settings << 1) | ack;

    return cursor;
}

uint8_t DataEncDec::addHeader(uint8_t from, uint8_t to, uint8_t ack, uint8_t settings){
    if ((cursor + 1) > maxsize){
        return 0;
    } 
                    // Recipient    Sender        Settings         Acknowledgement
    buffer[cursor++] = (to << 6) | (from << 4) | (settings << 1) | ack;

    return cursor;
}

uint8_t DataEncDec::addDate(long value){
    if ((cursor + 4) > maxsize){
        return 0;
    } 

    uint32_t data = value;

    buffer[cursor++] = data >> 24;
    buffer[cursor++] = data >> 16;
    buffer[cursor++] = data >> 8;
    buffer[cursor++] = data;
    return cursor;
}

uint8_t DataEncDec::addTemp(float value){
    if ((cursor + 2) > maxsize){
        return 0;
    } 

    // -40 to 125 C precission 0.1
    uint16_t data = roundf((value/0.1) + 400);
    buffer[cursor++] = data >> 8;
    buffer[cursor++] = data;

    return cursor;
}

uint8_t DataEncDec::addHumi(int value){
    if ((cursor + 1) > maxsize){
        return 0;
    }

    // 0 to 100 % precission 1
    buffer[cursor++] = value; 

    return cursor;
}

uint8_t DataEncDec::addIrrad(float value){
    if ((cursor + 2) > maxsize){
        return 0;
    } 

    // 0 to 6553.5 W/m2 precission 0.1
    uint16_t data = round(value/0.1);
    buffer[cursor++] = data >> 8;
    buffer[cursor++] = data;

    return cursor;
}

uint8_t DataEncDec::addWindSpeed(float value){
    if ((cursor + 1) > maxsize){
        return 0;
    } 

    // 0 to 255 Km/h precission 1
    uint8_t data = roundf(value);
    buffer[cursor++] = data;

    return cursor;
}

uint8_t DataEncDec::addWindDirection(int value){
    if ((cursor + 1) > maxsize){
        return 0;
    } 

    uint8_t data = 0; //N

    if (value == 45) data = 1;     //NE
    if (value == 90) data = 2;     //E
    if (value == 135) data = 3;    //SE 
    if (value == 180) data = 4;    //S
    if (value == 225) data = 5;    //SO
    if (value == 270) data = 6;    //O
    if (value == 315) data = 7;    //NO

    buffer[cursor++] = data;

    return cursor;
}

uint8_t DataEncDec::addRain(float value){
    if ((cursor + 1) > maxsize){
        return 0;
    } 

    // 0 to 63.75 mm precission 0.25
    uint8_t data = value/0.25;
    buffer[cursor++] = data;

    return cursor;
}

uint8_t DataEncDec::addVoltage(float value){
    // 0 to 6553.5 V precission 0.1
    return addIrrad(value);
}

uint8_t DataEncDec::addCurrent(float value){
    if ((cursor + 1) > maxsize){
        return 0;
    }

    // 0 to 25.5 A precission 0.1
    uint8_t data = roundf(value/0.1);
    buffer[cursor++] = data;

    return cursor;
}

uint8_t DataEncDec::addPower(float value){
    if ((cursor + 3) > maxsize){
        return 0;
    } 

    // -9000 to 9000 W precission 0.1
    uint32_t data = round((value/0.1)+90000);
    buffer[cursor++] = data >> 16;
    buffer[cursor++] = data >> 8;
    buffer[cursor++] = data;
    return cursor;
}

uint8_t DataEncDec::getTo(char header){
    uint8_t to = (header>>6) & 3;
    return to;
} 

uint8_t DataEncDec::getFrom(char header){
    uint8_t from = (header>>4) & 3;
    return from;
}

uint8_t DataEncDec::getACK(char header){
    uint8_t ack = (header & 1);
    return ack;
}

uint8_t DataEncDec::getSettings(char header){
    uint8_t settings = (header>>1) & 1;
    return settings;
}

long DataEncDec::getDate(char byte_h, char byte_hm, char byte_lm, char byte_l){
    uint32_t val = (byte_h << 24) | (byte_hm << 16) | (byte_lm << 8) | byte_l;
    long data = val;

    return data;
}

float DataEncDec::getTemp(char byte_h, char byte_l){
    uint16_t val = (byte_h << 8) | byte_l;
    float data = val;
    data = (data-400.0)*0.1;

    return data;
}

int DataEncDec::getHumi(char byte){
    int data = byte;

    return data;
}

float DataEncDec::getIrrad(char byte_h, char byte_l){
    uint16_t val = (byte_h << 8) | byte_l;
    float data = val;
    data = data*0.1;

    return data;
}

float DataEncDec::getWindSpeed(char byte){
    float data = byte;

    return data;
}

int DataEncDec::getWindDirection(char byte){
    int value = byte;
    int data = 0;
    if (value == 1) data = 45;     //NE
    if (value == 2) data = 90;      //E
    if (value == 3) data = 135;    //SE 
    if (value == 4) data = 180;    //S
    if (value == 5) data = 225;    //SO
    if (value == 6) data = 270;    //O
    if (value == 7) data = 315;    //NO

    return data;
}

float DataEncDec::getRain(char byte){
    float data = byte;
    data = data*0.25;

    return data;
}

float DataEncDec::getVoltage(char byte_h, char byte_l){
    return getIrrad(byte_h, byte_l);
}

float DataEncDec::getCurrent(char byte){
    float data = byte;
    data = data*0.1;

    return data;
}

float DataEncDec::getPower(char byte_h, char byte_m, char byte_l){
    uint32_t val = (byte_h << 16) | (byte_m << 8) | byte_l;
    float data = val;
    data = (data-90000.0)*0.1;

    return data;
}