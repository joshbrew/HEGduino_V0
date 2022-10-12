#include "MAX30105.h"
#include <Wire.h>

MAX30105 PPG1;
MAX30105 PPG2;

#define SDA 23
#define SCL 22

#define SDA1 33
#define SCL1 32

//not working on the sparkfun lib
byte interruptPin1 = 21;
byte interruptPin2 = 15;

//ESP32 has Wire and Wire1 by default, for other arduinos replace Wire and Wire1 references with this:
//TwoWire I2C1 = TwoWire(0);
//TwoWire I2C2 = TwoWire(1);

long long currentMillis = 0;

int maxR2 = 0;
int maxIR2 = 0;
byte redct = 0;
byte irct = 0;
int smpct = 0;

byte irctmax = 32; //number of samples before the IR oscillates back to zero (due to desynchronized MAX30102 sensors with one taking ambient measurements trying to catch the LEDs from the other MAX30102)

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
  byte sAvg = 8;
  int sRate = 1600;
  int ledMode = 2;
  int pulseWidth = 69; //411
  int adcRange = 16384;

  PPG1.softReset();
  PPG2.softReset();

  PPG1.shutDown();
  PPG2.shutDown();

  //user the same settings except PPG1 flashes the LEDs
  PPG1.setup(LEDpwr, 1, ledMode, 800, 118, adcRange);

  //delayMicros(100) ///??? How to sync devices
  PPG2.setup(0x00, sAvg, ledMode, sRate, pulseWidth, adcRange);

  PPG1.wakeUp();
  PPG2.wakeUp();
  
  //PPG1.setPulseAmplitudeIR(255); //only use IR
  //PPG1.setPulseAmplitudeRed(0);
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

int R2_0 = 0;
int R2_1 = 0;
int R2_2 = 0;
int R2_3 = 0;
int R2_4 = 0;


int IR2_0 = 0;
int IR2_1 = 0;
int IR2_2 = 0;
int IR2_3 = 0;
int IR2_4 = 0;

//bool cycled = false;

//trigger on interrupt
void readPulseOx() {
  PPG1.check();
  PPG2.check();

  while(PPG2.available()) {

   int R1 = PPG1.getFIFORed();
   int R2 = PPG2.getFIFORed();
   int IR1 = PPG1.getFIFOIR();
   int IR2 = PPG2.getFIFOIR();


    R2_4 = R2_3;
    R2_3 = R2_2;
    R2_2 = R2_1;
    R2_1 = R2_0;
    R2_0 = R2;

    IR2_4 = IR2_3;
    IR2_3 = IR2_2;
    IR2_2 = IR2_1;
    IR2_1 = IR2_0;
    IR2_0 = IR2;
      
    if(R2_2 > IR2_2) {
      if(maxR2 < R2_2 && R2_2 > R2_0 && R2_2 > R2_1 && R2_2 > R2_3 && R2_2 > R2_4) maxR2 = R2_2;
    }
    if(IR2_2 > R2_2) {
      if(maxIR2 < IR2_2 && IR2_2 > IR2_0 && IR2_2 > IR2_1 && IR2_2 > IR2_3 && IR2_2 > IR2_4) maxIR2 = IR2_2;
    }
//   Serial.print("R2: ");
//   Serial.print(R2);
//   Serial.print("\t IR2:");
//   Serial.println(IR2);

    if( R2_4 > 0 && IR2_4 > 0 && (IR2 > R2 && IR2_4 < R2_4) && maxR2 > 0 && maxIR2 > 0) { //switching
          //the lower of the two is the Red LED pulse
          if(maxR2 > maxIR2) {
            int temp = maxR2;
            maxR2 = maxIR2;
            maxIR2 = temp;  
          }
      
//        Serial.print("Time ");
//        Serial.print(millis());
//        Serial.print("R2Max: ");
//        Serial.print(maxR2);
//        Serial.print("\t IR2Max: ");
//        Serial.println(maxIR2);
//        Serial.print("\t Ratio ");
//        Serial.println(float(maxR2)/float(maxIR2));
//        Serial.print("\t Ambient ");
//        Serial.print(0);
//        Serial.print("\t Die Temp ");
//        Serial.println(PPG2.readTemperature());

        char outputArr[100];


        sprintf(outputArr,"%ul|%d|%d|%0.3f",
          micros(),maxR2, maxIR2, float(maxR2)/float(maxIR2)
        );
        
        Serial.println(outputArr);
        
        R2_4 = 0;
        R2_3 = 0;
        R2_2 = 0;
        R2_1 = 0;
        R2_0 = 0;

        IR2_4 = 0;
        IR2_3 = 0;
        IR2_2 = 0;
        IR2_1 = 0;
        IR2_0 = 0;
        
        maxR2 = 0;
        maxIR2 = 0;
        //irct = 0;
        //redct = 0;
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
