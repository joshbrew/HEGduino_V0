#ifndef BLE_API_H
#define BLE_API_H

//https://learn.sparkfun.com/tutorials/esp32-ota-updates-over-ble-from-a-react-web-application/all
//^^ Implement Bluetooth OTA ^^

/*
 *  pOtaCharacteristic = pService->createCharacteristic(
                     CHARACTERISTIC_UUID_FW,
                     BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE
                     );
    pOtaCharacteristic->addDescriptor(new BLE2902());
    pOtaCharacteristic->setCallbacks(new otaCallback(this));
        
    void otaCallback::onWrite(BLECharacteristic *pCharacteristic)
    {
      std::string rxData = pCharacteristic->getValue();
      if (!updateFlag) { //If it's the first packet of OTA since bootup, begin OTA
        Serial.println("BeginOTA");
        esp_ota_begin(esp_ota_get_next_update_partition(NULL), OTA_SIZE_UNKNOWN, &otaHandler);
        updateFlag = true;
      }
      if (_p_ble != NULL)
      {
        if (rxData.length() > 0)
        {
          esp_ota_write(otaHandler, rxData.c_str(), rxData.length());
          if (rxData.length() != FULL_PACKET)
          {
            esp_ota_end(otaHandler);
            Serial.println("EndOTA");
            if (ESP_OK == esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL))) {
              esp_restart();
            }
            else {
              Serial.println("Upload Error");
            }
          }
        }
      }

  uint8_t txData[5] = {1, 2, 3, 4, 5};
  //delay(1000);
  pCharacteristic->setValue((uint8_t*)txData, 5);
  pCharacteristic->notify();
} 
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "BluetoothSerial.h"

#include "HEGduino_V0.h"

BluetoothSerial SerialBT;

BLECharacteristic *pCharacteristic;

#define SERVICE_UUID                   "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX         "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX         "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

bool USE_BT = false;          
bool USE_BLE = false;         
bool BLE_ON, BLE_SETUP = false;
bool BLEdeviceConnected = false;

#define SOFTWARE_VERSION_MAJOR 0
#define SOFTWARE_VERSION_MINOR 1
#define SOFTWARE_VERSION_PATCH 0
#define HARDWARE_VERSION_MAJOR 1
#define HARDWARE_VERSION_MINOR 2

#define SERVICE_UUID_OTA               "6E400004-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_ID         "6E400005-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_FW         "6E400006-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_HW_VERSION "6E400007-B5A3-F393-E0A9-E50E24DCCA9E"

#define FULL_PACKET 512
#define CHARPOS_UPDATE_FLAG 5

#include "esp_ota_ops.h"
esp_ota_handle_t otaHandler = 0;
bool updateFlag = false;
bool readyFlag = false;
int bytesReceived = 0;
int timesWritten = 0;

class MyServerCallbacks : public BLEServerCallbacks
{

  void onConnect(BLEServer *pServer)
  {
    Serial.println("Device connected to BLE");
    BLEdeviceConnected = true;
  }

  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("Device disconnected from BLE");
    BLEdeviceConnected = false;
  }

};

class otaCallback: public BLECharacteristicCallbacks {
  public:
    otaCallback() {  
    }
    

    void onWrite(BLECharacteristic *pCharacteristic);
};

void otaCallback::onWrite(BLECharacteristic *pCharacteristic)
{
  //HEG1.write_reg(REG_MODE_CONFIG,0b00000011);
  coreProgramEnabled = false;
  
  std::string rxData = pCharacteristic->getValue();
  if (!updateFlag) { //If it's the first packet of OTA since bootup, begin OTA
    Serial.println("BeginOTA");
    esp_ota_begin(esp_ota_get_next_update_partition(NULL), OTA_SIZE_UNKNOWN, &otaHandler);
    updateFlag = true;
  }

  if (rxData.length() > 0)
  {
    esp_ota_write(otaHandler, rxData.c_str(), rxData.length());
    if (rxData.length() != FULL_PACKET)
    {
      esp_ota_end(otaHandler);
      Serial.println("EndOTA");
      if (ESP_OK == esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL))) {
        delay(2000);
        esp_restart();
      }
      else {
        Serial.println("Upload Error");
      }
    }
    
  }

  uint8_t txData[5] = {1, 2, 3, 4, 5};
  //delay(1000);
  pCharacteristic->setValue((uint8_t*)txData, 5);
  pCharacteristic->notify();
}

//Setup BLE callbacks here.
class MyCallbacks : public BLECharacteristicCallbacks //We need to set up the BLE callback commands here.
{

  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0)
    {

      Serial.println("HEG RECEIVE: ");

      for (int i = 0; i < rxValue.length(); i++)
      {
        Serial.print(rxValue[i]);
      }
       
      //commandESP32(rxValue.c_str());

      if (rxValue.find("t") != -1)
        { //Enable Sensor
            
          PPG1.wakeUp();
          PPG2.wakeUp();
          coreProgramEnabled = true;
        }
        if (rxValue.find("f") != -1)
        { //Disable sensor
            PPG1.shutDown();
            PPG2.shutDown();
            coreProgramEnabled = false;
        }
        if (rxValue.find("o") != -1) {
          if(MODE != "FAST") {
            MODE = "FAST";
          }
          else {
            MODE = "";
          }
        }
        if (rxValue.find("h") != -1) {
          if(MODE != "SPO2") {
            MODE = "SPO2";
          }
          else {
            MODE = "";
          }
        }
        if (rxValue.find("T") != -1) {
          if(MODE != "TEMP") {
            MODE = "TEMP";
          }
          else {
            MODE = "";
          }
        }
      if (rxValue.find("i") != -1){ //change intensity
        if(LEDPA == "FULL"){
          PPG1.setPulseAmplitudeRed(128);
          PPG1.setPulseAmplitudeIR(128); // 0 = 0mA, 255 = Max mA
          LEDPA = "HALF";
        }
        else {
          PPG1.setPulseAmplitudeRed(255); //will saturate ppg1 but make ppg2 more readable
          PPG1.setPulseAmplitudeIR(255);
          LEDPA = "FULL";
        }
      }
        if (rxValue.find("X") != -1){
          if(USE_DC_FILTER == true){
            USE_DC_FILTER = false;
          }
          else {
            USE_DC_FILTER = true;
          }
        }
        if (rxValue.find("L") != -1) {}
        if (rxValue.find("c") != -1) {}
        if (rxValue.find("r") != -1) {}
        if (rxValue.find("R") != -1) {
            //HEG1.write_reg(REG_MODE_CONFIG,0b00000011);
            coreProgramEnabled = false;
            if (USE_USB == true) {
            Serial.println("Restarting ESP32...");
            }
            delay(300);
            ESP.restart();
        }
        if (rxValue.find("S") != -1) 
        {
          //HEG1.write_reg(REG_MODE_CONFIG,0b00000011);
            coreProgramEnabled = false;
            Serial.println("HEG going to sleep now... Reset the power to turn device back on!");
            delay(1000);
            esp_deep_sleep_start(); //Ends the loop() until device is reset.
        }
        if (rxValue.find("u") != -1)
        { //USB Toggle
            EEPROM.begin(512);
            int ComMode = EEPROM.read(0);
            if(ComMode != 3){
            EEPROM.write(0,3);
            EEPROM.commit();
            EEPROM.end();
            delay(100);
            ESP.restart();
            }
            else //Default back to WiFi mode
            {
            EEPROM.write(0,0);
            EEPROM.commit();
            EEPROM.end();
            delay(100);
            ESP.restart();
            }
        }
        if (rxValue.find("B") != - 1)
        { //Bluetooth Serial Toggle
            EEPROM.begin(512);
            int ComMode = EEPROM.read(0);
            if (ComMode != 2)
            {
            EEPROM.write(0,2);
            EEPROM.commit();
            EEPROM.end();
            delay(100);
            ESP.restart();
            }
            else //Default back to WiFi mode
            {
            EEPROM.write(0,0);
            EEPROM.commit();
            EEPROM.end();
            delay(100);
            ESP.restart();
            }
        }
        if (rxValue.find("b") != -1)
        { //Bluetooth LE Toggle
            EEPROM.begin(512);
            if (EEPROM.read(0) != 1)
            {
            EEPROM.write(0,1);
            EEPROM.commit();
            EEPROM.end();
            delay(100);
            ESP.restart();
            }
            else //Default back to WiFi mode
            {
            EEPROM.write(0,0);
            EEPROM.commit();
            EEPROM.end();
            delay(100);
            ESP.restart();
            }
        }
        delay(100);
      }
    }
};

void setupBTSerial() {

  SerialBT.begin("HEGduino BT");
  Serial.println("HEG booted in Bluetooth Serial Mode! Pair it with your device and access the stream via standard serial monitor.");
  USE_BT = true;
  delay(100);

}

void outputBT(){
  if (USE_BT == true) //BTSerial
    {
      if (SerialBT.hasClient() == true)
      {
          SerialBT.flush();
          SerialBT.print(outputarr);
          delay(10); // 10ms delay min required for BT output only due to bottleneck in library.
      }
    }
  if (BLEdeviceConnected == true) // BLE
  {
      pCharacteristic->setValue(outputarr);
      pCharacteristic->notify();
      delay(10); // bluetooth stack will go into congestion if too many packets are sent
  }
}

void setupBLE()
  {
      // Create the BLE Device
      BLEDevice::init("HEG"); // Give it a name
      BLEDevice::setMTU(512);

      // Create the BLE Server
      BLEServer *pServer = BLEDevice::createServer();
      pServer->setCallbacks(new MyServerCallbacks());

      // Create the BLE Service
      BLEService *pService = pServer->createService(SERVICE_UUID);

      // Create a BLE Characteristic
      pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

      BLEService *pESPOTAService = pServer->createService(SERVICE_UUID_OTA);
      BLECharacteristic * pESPOTAIdCharacteristic = pESPOTAService->createCharacteristic(
                                       CHARACTERISTIC_UUID_ID,
                                       BLECharacteristic::PROPERTY_READ
                                     );
      BLECharacteristic * pVersionCharacteristic = pService->createCharacteristic(
                             CHARACTERISTIC_UUID_HW_VERSION,
                             BLECharacteristic::PROPERTY_READ
                           );
      BLECharacteristic * pOtaCharacteristic = pService->createCharacteristic(
                         CHARACTERISTIC_UUID_FW,
                         BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE
                       );

      pOtaCharacteristic->addDescriptor(new BLE2902());
      pOtaCharacteristic->setCallbacks(new otaCallback());

      BLE2902 *desc = new BLE2902();
      desc->setNotifications(true);
      pCharacteristic->addDescriptor(desc);

      BLECharacteristic *pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
      pCharacteristic->setReadProperty(true);
      pCharacteristic->setCallbacks(new MyCallbacks());

      // Start the service(s)
      pESPOTAService->start();
      pService->start();

      // Start advertising
      pServer->getAdvertising()->addServiceUUID(SERVICE_UUID_OTA);
      pServer->getAdvertising()->start();

      uint8_t hardwareVersion[5] = {HARDWARE_VERSION_MAJOR, HARDWARE_VERSION_MINOR, SOFTWARE_VERSION_MAJOR, SOFTWARE_VERSION_MINOR, SOFTWARE_VERSION_PATCH};
      pVersionCharacteristic->setValue((uint8_t*)hardwareVersion, 5);
      
      BLE_SETUP = true;
      BLE_ON = true;
      USE_BLE = true;
      Serial.println("BLE service started, scan for HEG.");
}


#endif
