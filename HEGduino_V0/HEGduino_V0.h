#ifndef HEGDUINO_V0_H
#define HEGDUINO_V0_H
//MAX86141 HEG SCRIPT

//#include "MAX86141.h"
#include "MAX30105.h"
#include <Wire.h>
#include "spo2_algorithm.h"
#include <CircularBuffer.h>
#include <esp_timer.h>
#include "IIRfilter.h"


//
// Pin Definitions.
//

#define SDA 23
#define SCL 22

#define SDA1 33
#define SCL1 32

//not working on the sparkfun lib
byte interruptPin1 = 21;
byte interruptPin2 = 15;

float sps = 10; //~avg output at default setting

MAX30105 PPG1;
MAX30105 PPG2;

byte LEDpwr = 255;
byte sAvg = 4;
int sRate = 1600;
int ledMode = 2;
int pulseWidth = 69;
int adcRange = 16384;

byte redct = 0;
byte irct = 0;

byte irctmax = 10; //number of samples before the IR oscillates back to zero (due to desynchronized MAX30102 sensors with one taking ambient measurements trying to catch the LEDs from the other MAX30102)

bool USE_USB = true;

char outputarr[64];
bool newOutputFlag = false;
bool newEvent = false; //WiFi event task

bool RED_ON = false;
bool IR_ON = false;

//from max86141 settings
bool AMBIENT = true;
char * MODE = "DEFAULT"; //SPO2, DEBUG, FAST, TEMP, EXT_LED (raw ambient mode with GPIO timer based external leds)
char * LEDPA = "FULL"; //FULL, HALF
char * EXPMODE = "DEFAULT"; //Exposure modes, DEFAULT, FAST, SLOW
char * LEDMODE = "DEFAULT"; //DEFAULT, 2IR, REDISAMB, 2IRAMB
bool USE_FILTERS = false;
bool USE_DC_FILTER = false;

bool coreProgramEnabled = false;

float RED_AVG, IR_AVG, AMBIENT_AVG, RATIO_AVG;
float v1, drdt, lastRatio;

//pulse Ox library variables
CircularBuffer<uint32_t, 200> redBuffer;
CircularBuffer<uint32_t, 200> irBuffer;
int32_t bufCap = 200;

int32_t spo2;
int8_t validSPO2;
int32_t lastValidSPO2;
int32_t heartRate;
int8_t validHeartRate;
int32_t lastValidHeartRate;
////

unsigned long currentMicros = 0;
unsigned long lastSampleMicros = 0;
unsigned long coreNotEnabledMicros = 0;
unsigned long lastSPO2Micros = 0;

unsigned long SPO2Freq = 25000; //SPO2/HR calculation frequency

bool DEEP_SLEEP_EN = true;
unsigned long sleepTimeout = 600000000; //10 minutes till sleep if no activity on sensor.

//Manual LED sampling controls.
unsigned long LEDMicros = 0;
unsigned long LEDFrequency = 7500; //Time (in Microseconds) between each change in LED state (Red, IR, Ambient);

void setupHEG() {
    
  // put your setup code here, to run once:
//  pinMode(interruptPin1, INPUT);
//  pinMode(interruptPin2, INPUT);

  Wire.setPins(SDA,SCL);
  Wire1.setPins(SDA1,SCL1);

  Serial.begin(115200);
  Serial.println("Sensors starting");
  
  bool p1 = PPG1.begin(Wire,  I2C_SPEED_FAST);
  bool p2 = PPG2.begin(Wire1, I2C_SPEED_FAST);

  if(!p1) Serial.println("Pulse Ox 1 not found");
  if(!p2) Serial.println("Pulse Ox 2 not found");

  PPG1.softReset();
  PPG2.softReset();

  PPG1.shutDown();
  PPG2.shutDown();

  //user the same settings except PPG1 flashes the LEDs
  PPG1.setup(LEDpwr, sAvg, ledMode, sRate, pulseWidth, adcRange);
  PPG2.setup(0x00, sAvg, ledMode, sRate, pulseWidth, adcRange);

  PPG1.wakeUp();
  PPG2.wakeUp();
//  //enable interrupts
//  PPG1.enableAFULL();
//  PPG2.enableAFULL();

//  PPG1.writeRegister8(PPG1._i2caddr,0x02,0b1000000);
//  PPG2.writeRegister8(PPG2._i2caddr,0x02,0b1000000);
//  
//  PPG1.setFIFOAlmostFull(1);
//  PPG2.setFIFOAlmostFull(1);

  //disable rollover
//  PPG1.disableFIFORollover();
//  PPG2.disableFIFORollover();

//  Serial.print("PPG1 INT: ");
//  Serial.println(PPG1.getINT1());
//  Serial.print("PPG1 INT GPIO: ");
//  Serial.println(digitalRead(interruptPin1));
//  Serial.print("PPG2 INT: ");
//  Serial.println(PPG2.getINT1());
//  Serial.print("PPG2 INT GPIO: ");
//  Serial.println(digitalRead(interruptPin2));


  //attachInterrupt(interruptPin2, readPulseOx, FALLING);

  currentMicros = esp_timer_get_time();
  coreNotEnabledMicros = currentMicros;
  coreProgramEnabled = true;

}

void outputSerial(){
    if (USE_USB == true)
    {
        //Serial.flush();
        Serial.print(outputarr);
    }
}

void sampleHEG(){
    PPG1.check();
    PPG2.check();

    while(PPG2.available()) {

      //Serial.println(HEG1.read_reg(REG_FIFO_DATA_COUNT));
          
       //int R1 = PPG1.getFIFORed();
       //int IR1 = PPG1.getFIFOIR();
       
       int R2 = PPG2.getFIFORed();
       int IR2 = PPG2.getFIFOIR();
    
        if(RED_AVG < R2) RED_AVG = float(R2);
        if(IR_AVG < IR2) IR_AVG = float(IR2);
        if(IR2 > 0) irct++;

      if(IR2 == 0 && irct > irctmax) {
        RATIO_AVG = RED_AVG/IR_AVG;

  
        if(MODE == "SPO2"){
          redBuffer.push(RED_AVG);
          irBuffer.push(IR_AVG);
  
          if(currentMicros - lastSPO2Micros > SPO2Freq){ //
            if(redBuffer.isFull()){
              uint32_t rBufTemp[bufCap];
              uint32_t irBufTemp[bufCap];
              for(int i=0; i<bufCap; i++){
                rBufTemp[i] = redBuffer[i];
                irBufTemp[i] = irBuffer[i];
              }
  
              maxim_heart_rate_and_oxygen_saturation(rBufTemp, bufCap, irBufTemp, &spo2, &validSPO2, &heartRate, &validHeartRate);
  
              if(validSPO2 == 1){ lastValidSPO2 = spo2;}
              if(validHeartRate == 1) { lastValidHeartRate = heartRate; }
            }
            lastSPO2Micros = currentMicros;
          }
            sprintf(outputarr, "%lu|%0.0f|%0.0f|%0.4f|%d|%d\r\n",
              currentMicros, RED_AVG, IR_AVG, RATIO_AVG, lastValidHeartRate, lastValidSPO2);
        }
        else if (MODE == "TEMP") {
          uint8_t t = PPG2.readTemperature();
          sprintf(outputarr, "%lu|%0.0f|%0.0f|%0.4f|%0.0f|%0.4f\r\n",
              currentMicros, RED_AVG, IR_AVG, RATIO_AVG, AMBIENT_AVG, t);
        }
        else if (MODE == "DEBUG"){
            sprintf(outputarr, "RED: %0.0f \t IR: %0.0f \t RATIO: %0.4f \r\n",
                RED_AVG, IR_AVG, RATIO_AVG);
          }
        else if (MODE == "FAST"){
            sprintf(outputarr, "%0.0f|%0.0f|%0.4f\r\n",
                RED_AVG, IR_AVG, RATIO_AVG);
        }
        else { //Default
            sprintf(outputarr, "%lu|%0.1f|%0.1f|%0.4f\r\n",
              currentMicros, RED_AVG, IR_AVG, RATIO_AVG);
        }
        //Serial.print("Red: ");
        //Serial.print(RED_AVG);
        //Serial.print("\t");
        //Serial.print("IR: ");
        //Serial.print(IR_AVG);
        //Serial.print("\t");
  
        //Serial.println(outputarr);
  
        newOutputFlag = true; //Flags for output functions
        newEvent = true;
        lastSampleMicros = currentMicros;
        lastRatio=RATIO_AVG;

        RED_AVG = 0;
        IR_AVG = 0;
        irct = 0;
      }
      
      PPG1.nextSample();
      PPG2.nextSample();
    }
}


// the loop function runs over and over again until power down or reset
void HEG_core_loop() {

  if(currentMicros - coreNotEnabledMicros < sleepTimeout){ //Enter sleep mode after 10 min of inactivity (in microseconds).
    if(coreProgramEnabled == true){
      sampleHEG();
      coreNotEnabledMicros = currentMicros; // Core is enabled, sleep timer resets;
    }
  }
  else if (DEEP_SLEEP_EN == true){
    Serial.println("HEG going to sleep now... Reset the power to turn device back on!");
    delay(1000);
    esp_deep_sleep_start(); //Ends the loop() until device is reset.
  }
}

#endif
