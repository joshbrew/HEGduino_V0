#include "MAX30105.h"
#include <Wire.h>

MAX30105 PPG1;
MAX30105 PPG2;

#define SDA 23
#define SCL 22

#define SDA1 14
#define SCL1 32

//not working on the sparkfun lib
byte interruptPin1 = 33;
byte interruptPin2 = 15;

//ESP32 has Wire and Wire1 by default, for other arduinos replace Wire and Wire1 references with this:
//TwoWire I2C1 = TwoWire(0);
//TwoWire I2C2 = TwoWire(1);

long long currentMillis = 0;

int summedR2 = 0;
int summedIR2 = 0;
byte redct = 0;
byte irct = 0;

byte irctmax = 4; //number of samples before the IR oscillates back to zero (due to desynchronized MAX30102 sensors with one taking ambient measurements trying to catch the LEDs from the other MAX30102)

void setup() {

  
  // put your setup code here, to run once:
  pinMode(interruptPin1, INPUT);
  pinMode(interruptPin2, INPUT);

  Wire.setPins(SDA,SCL);
  Wire1.setPins(SDA1,SCL1);

  Serial.begin(115200);
  Serial.println("Sensors starting");
  
  bool p1 = PPG1.begin(Wire,  I2C_SPEED_FAST);
  bool p2 = PPG2.begin(Wire1, I2C_SPEED_FAST);

  if(!p1) Serial.println("Pulse Ox 1 not found");
  if(!p2) Serial.println("Pulse Ox 2 not found");

  byte LEDpwr = 255;
  byte sAvg = 4;
  int sRate = 1600;
  int ledMode = 2;
  int pulseWidth = 69;
  int adcRange = 16384;

  PPG1.softReset();
  PPG2.softReset();

  PPG1.shutDown();
  PPG2.shutDown();

  //user the same settings except PPG1 flashes the LEDs
  PPG1.setup(LEDpwr, sAvg, ledMode, sRate, pulseWidth, adcRange);
  //delayMicros(100) ///??? How to sync devices
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

}

//trigger on interrupt
void readPulseOx() {
  PPG1.check();
  PPG2.check();

  while(PPG2.available()) {

   int R1 = PPG1.getFIFORed();
   int R2 = PPG2.getFIFORed();
   int IR1 = PPG1.getFIFOIR();
   int IR2 = PPG2.getFIFOIR();

    if(summedR2 < R2) summedR2 = R2;
    if(summedIR2 < IR2) summedIR2 = IR2;
    if(IR2 > 0) irct++;

//    Serial.print("R2 ");
//    Serial.print(R2);
//    Serial.print("\t IR2");
//    Serial.println(IR2);
    
    if(IR2 == 0 && irct > irctmax) {
//        Serial.print("Time ");
//        Serial.print(millis());
//        Serial.print("\t R2 Max ");
//        Serial.print(summedR2);
//        Serial.print("\t IR2 Max ");
//        Serial.print(summedIR2);
//        Serial.print("\t Ratio ");
//        Serial.println(float(summedR2)/float(summedIR2));
//        Serial.print("\t Ambient ");
//        Serial.print(0);
//        Serial.print("\t Die Temp ");
//        Serial.println(PPG2.readTemperature());

        char outputArr[100];


        sprintf(outputArr,"%ul|%d|%d|%0.3f",
          micros(),summedR2, summedIR2, float(summedR2)/float(summedIR2)
        );
         Serial.println(outputArr);
        
        irct = 0;
        summedR2 = 0;
        summedIR2 = 0;
    }

    PPG1.nextSample();
    PPG2.nextSample();
  }

  //clear interrupts
//  PPG1.getINT1();
//  PPG2.getINT1();
}

void loop() {
  // put your main code here, to run repeatedly:
  //  if(digitalRead(interruptPin1) == LOW) { 
  //      
  //    //Serial.println("Samples read!");
  //  }
  //  Serial.println("TEST");
  //  delay(1000);
  
  readPulseOx();
}
