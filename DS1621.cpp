#include "DS1621.h"

// based on code from McPhalen published at http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1198065647



DS1621::DS1621(byte i2c_addr) {
  addr=i2c_addr;
  Wire.begin();
}

// device ID and address
#define DEV_TYPE   0x90 >> 1                    // shift required by wire.h
#define DEV_ADDR   0x00                         // DS1621 address is 0
#define SLAVE_ID   DEV_TYPE | DEV_ADDR

// DS1621 Registers & Commands

#define RD_TEMP    0xAA                         // read temperature register
#define ACCESS_TH  0xA1                         // access high temperature register
#define ACCESS_TL  0xA2                         // access low temperature register
#define ACCESS_CFG 0xAC                         // access configuration register
#define RD_CNTR    0xA8                         // read counter register
#define RD_SLOPE   0xA9                         // read slope register
#define START_CNV  0xEE                         // start temperature conversion
#define STOP_CNV   0X22                         // stop temperature conversion

// DS1621 configuration bits

#define DONE       B10000000                    // conversion is done
#define THF        B01000000                    // high temp flag
#define TLF        B00100000                    // low temp flag
#define NVB        B00010000                    // non-volatile memory is busy
#define POL        B00000010                    // output polarity (1 = high, 0 = low)
#define ONE_SHOT   B00000001                    // 1 = one conversion; 0 = continuous conversion

// Set configuration register

void DS1621::setConfig(byte cfg)
{
 Wire.beginTransmission(SLAVE_ID);
 Wire.send(ACCESS_CFG);
 Wire.send(cfg);
 Wire.endTransmission();
 delay(15);                                    // allow EE write time to finish
}


// Read a DS1621 register

byte DS1621::getReg(byte reg)
{
 Wire.beginTransmission(SLAVE_ID);
 Wire.send(reg);                               // set register to read
 Wire.endTransmission();
 Wire.requestFrom(SLAVE_ID, 1);
 byte regVal = Wire.receive();
 return regVal;
}


// Sets temperature threshold
// -- whole degrees C only
// -- works only with ACCESS_TL and ACCESS_TH

void DS1621::setThresh(byte reg, int tC)
{
 if (reg == ACCESS_TL || reg == ACCESS_TH) {
   Wire.beginTransmission(addr);
   Wire.send(reg);                             // select temperature reg
   Wire.send(byte(tC));                        // set threshold
   Wire.send(0);                               // clear fractional bit
   Wire.endTransmission();
   delay(15);
 }
}


// Start/Stop DS1621 temperature conversion

void DS1621::startConversion(boolean start)
{
 Wire.beginTransmission(addr);
 if (start == true)
   Wire.send(START_CNV);
 else
   Wire.send(STOP_CNV);
 Wire.endTransmission();
}


// Reads temperature or threshold
// -- whole degrees C only
// -- works only with RD_TEMP, ACCESS_TL, and ACCESS_TH

int DS1621::getTemp(byte reg)
{
 int tC;

 if (reg == RD_TEMP || reg == ACCESS_TL || reg == ACCESS_TH) {
   byte tVal = getReg(reg);
   if (tVal >= B10000000) {                    // negative?
     tC = 0xFF00 | tVal;                       // extend sign bits
   }
   else {
     tC = tVal;
   }
   return tC;                                  // return threshold
 }
 return 0;                                     // bad reg, return 0
}


// Read high resolution temperature
// -- returns temperature in 1/100ths degrees
// -- DS1620 must be in 1-shot mode

int DS1621::getHrTemp()
{
 startConversion(true);                        // initiate conversion
 byte cfg = 0;
 while (!(cfg & DONE)) {                          // let it finish
   cfg = getReg(ACCESS_CFG);
 }

 int tHR = getTemp(RD_TEMP);                   // get whole degrees reading
 byte cRem = getReg(RD_CNTR);                  // get counts remaining
 byte slope = getReg(RD_SLOPE);                // get counts per degree

 if (tHR >= 0)
   tHR = (tHR * 100 - 25) + ((slope - cRem) * 100 / slope);
 else {
   tHR = -tHR;
   tHR = (25 - tHR * 100) + ((slope - cRem) * 100 / slope);
 }
 return tHR;
}
