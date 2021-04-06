#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>

DS3231 rtcA;
RTClib rtc;

// Set the LCD address to 0x27 for a 20 chars and 4 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

OneWire sensorAWire(7);
OneWire sensorBWire(6);
OneWire sensorCWire(5);
OneWire sensorDWire(4);

DallasTemperature sensorA(&sensorAWire);
DallasTemperature sensorB(&sensorBWire);
DallasTemperature sensorC(&sensorCWire);
DallasTemperature sensorD(&sensorDWire);

DHT dhtInsideBox(3, DHT22);
DHT dhtOutsideBox(2, DHT22);

#define valveRelayPin (int)9
#define troubleRelayPin (int)8
#define measuringDelay (long)120000
#define wateringDelay (long)480000
#define sysNormal (String)"   System Normal   "
#define clearLine (String)"                    "
#define sysWatering (String)"      Watering      "
#define sysIdle (String)"    System Idle    "
#define sysTempWarn (String)"  OVERHEAT WARNING  "
#define sysLogWarn (String)"     LOG ERROR     "
#define sdCardFail (String)"  SD CARD FAILURE  "

int moistureSensorAPercent = 0;
int moistureSensorBPercent = 0;
int moistureSensorCPercent = 0;
int moistureSensorDPercent = 0;
int avgSoilMoisture = 0;

float sensorATempC = 0;
float sensorBTempC = 0;
float sensorCTempC = 0;
float sensorDTempC = 0;
float avgSoilTemp = 0;
  
void setup()
{
   Wire.begin();
   SPI.begin();
   
  sensorA.begin();
  sensorB.begin();
  sensorC.begin();
  sensorD.begin();

  dhtInsideBox.begin();
  dhtOutsideBox.begin();
  
  // Set relay signal pins
  pinMode(valveRelayPin, OUTPUT);
  pinMode(troubleRelayPin, OUTPUT);

  // initialize the LCD
  lcd.init();
  lcd.clear();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  pinMode(10, OUTPUT);
  if(!SD.begin(10))
  {
    lcd.setCursor(0, 2);
    lcd.print(sdCardFail);
    digitalWrite(troubleRelayPin, HIGH);
  }
  else
  {
    lcd.setCursor(0, 2);
    lcd.print("   sd card sucess   ");
    digitalWrite(troubleRelayPin, LOW);
  }
  
  // Uncomment and set these properties to set the time
//  rtcA.setYear(21);
//  rtcA.setMonth(3);
//  rtcA.setDate(27);
//  rtcA.setDoW(6);
//  rtcA.setHour(7);
//  rtcA.setMinute(47);
//  rtcA.setSecond(50);
}

void LogEnvironment(String waterStatus)
{
  DateTime now  = rtc.now();
  File dataLog = SD.open("log.txt", FILE_WRITE);
  if(dataLog)
  {
    dataLog.print(now.year());
    dataLog.print("-");
    dataLog.print(now.month());
    dataLog.print("-");
    dataLog.print(now.day());  
    dataLog.print(",");
    dataLog.print(now.hour());
    dataLog.print(":");
    dataLog.print(now.minute());
    dataLog.print(","); 
    dataLog.print(sensorATempC);
    dataLog.print(","); 
    dataLog.print(sensorBTempC);
    dataLog.print(","); 
    dataLog.print(sensorCTempC);
    dataLog.print(","); 
    dataLog.print(sensorDTempC);
    dataLog.print(","); 
    dataLog.print(avgSoilTemp);
    dataLog.print(","); 
    dataLog.print(avgSoilMoisture);
    dataLog.print(","); 
    dataLog.print(moistureSensorAPercent);
    dataLog.print(","); 
    dataLog.print(moistureSensorBPercent);
    dataLog.print(","); 
    dataLog.print(moistureSensorCPercent);
    dataLog.print(","); 
    dataLog.print(moistureSensorDPercent);
    dataLog.print(",");     
    dataLog.print(avgSoilMoisture);
    dataLog.print(","); 
    dataLog.print(dhtInsideBox.readHumidity());
    dataLog.print(","); 
    dataLog.print(dhtInsideBox.readTemperature());
    dataLog.print(","); 
    dataLog.print(dhtOutsideBox.readHumidity());
    dataLog.print(","); 
    dataLog.print(dhtOutsideBox.readTemperature());
    dataLog.print(","); 
    dataLog.println(waterStatus);
    dataLog.close();
    lcd.setCursor(0,2);
    lcd.print("Last Log: "  + String(now.hour()) + ":" + String(now.minute()));
    digitalWrite(troubleRelayPin, LOW);
  }
  else 
  {
    lcd.setCursor(0,3);
    lcd.print(sysLogWarn);
    digitalWrite(troubleRelayPin, HIGH);
    digitalWrite(valveRelayPin, LOW);
  }
}

void GetTempAndMoist()
{
  moistureSensorAPercent = map(analogRead(A0), 580, 240, 0, 100);
  moistureSensorBPercent = map(analogRead(A1), 580, 240, 0, 100);
  moistureSensorCPercent = map(analogRead(A2), 580, 240, 0, 100);
  moistureSensorDPercent = map(analogRead(A3), 580, 240, 0, 100);
  avgSoilMoisture = ((moistureSensorAPercent + moistureSensorBPercent + moistureSensorCPercent + moistureSensorDPercent)/4);

  sensorA.requestTemperatures();
  sensorB.requestTemperatures();
  sensorC.requestTemperatures();
  sensorD.requestTemperatures();
  
  // Get the temperatures
  sensorATempC = sensorA.getTempCByIndex(0);
  sensorBTempC = sensorB.getTempCByIndex(0);
  sensorCTempC = sensorC.getTempCByIndex(0);
  sensorDTempC = sensorD.getTempCByIndex(0);
  avgSoilTemp = ((sensorATempC + sensorCTempC + sensorDTempC)/3);
}

void UpdateDisplay()
{
  lcd.clear();
  DateTime now  = rtc.now();
  lcd.setCursor(0, 0);
  lcd.print("-----" + String(now.year()) + "-" + String(now.month()) + "-" + String(now.day())+"-----");
}

void loop()
{
  bool watering = false;
  DateTime now  = rtc.now();
  GetTempAndMoist();
  UpdateDisplay(); 
  lcd.setCursor(0,1);
  lcd.print(sysIdle);
  
  if(now.hour() > 5 && now.hour() < 10 && avgSoilMoisture < 25 == true)
  {

      lcd.setCursor(0,1);
      lcd.print(sysWatering);
      digitalWrite(valveRelayPin, HIGH);
      watering = true;
      delay(wateringDelay);
      GetTempAndMoist();
      UpdateDisplay(); 
      lcd.setCursor(0,1);
      lcd.print(sysIdle);
      digitalWrite(valveRelayPin, LOW);
  }
  
  if(dhtInsideBox.readTemperature()>50)
  {
    lcd.setCursor(0,3);
    lcd.print(sysTempWarn);
    digitalWrite(troubleRelayPin, HIGH);
  }
  
  if(watering == true)
  {
      GetTempAndMoist();
      LogEnvironment("OPEN");
      delay(measuringDelay);
  }
  else
  {
    GetTempAndMoist();
    LogEnvironment("OFF");
    delay(measuringDelay + wateringDelay);
  }
}
