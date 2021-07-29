#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DS3231.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <SPI.h>
#include <SD.h>
#include <DHT.h>

RTClib clock;

// Set the LCD address to 0x27 for a 20 chars and 4 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

OneWire sensorAWire(7);
OneWire sensorBWire(6);
//OneWire sensorCWire(5);
//OneWire sensorDWire(4);

DallasTemperature sensorA(&sensorAWire);
DallasTemperature sensorB(&sensorBWire);
//DallasTemperature sensorC(&sensorCWire);
//DallasTemperature sensorD(&sensorDWire);

DHT dhtInsideBox(3, DHT22);
DHT dhtOutsideBox(2, DHT22);

#define valveRelayPin (int)9
#define troubleRelayPin (int)8
#define minSoilMoist (float)65.0
#define maxSoilTemp (float)30.0
#define minHourToWater (int)5
#define maxHourToWater (int)10
#define maxBoxTemp (float)40
#define measuringDelay (long)600000
#define wateringDelay (long)600000
#define sysNormal (String)"   System Normal   "
#define clearLine (String)"                    "
#define sysWatering (String)"      Watering      "
#define sysIdle (String)"    System Idle    "
#define sysTempWarn (String)"  OVERHEAT WARNING  "
#define sysLog (String)" Environment Logged "
#define sysLogSuccess (String)"   Log Successful   "
#define sysLogWarn (String)"     LOG ERROR     "
#define sysWait (String) "Waiting for readings"
#define sdCardFail (String)"  SD CARD FAILURE  "

float moistureSensorAPercent = 0;
float moistureSensorBPercent = 0;
//int moistureSensorCPercent = 0;
//int moistureSensorDPercent = 0;
float avgSoilMoisture = 0;

float sensorATempC = 0;
float sensorBTempC = 0;
//float sensorCTempC = 0;
//float sensorDTempC = 0;
float avgSoilTemp = 0;
  
float boxTemp = 0;
float boxHumid = 0;
float airTemp = 0;
float airHumid = 0;

//bool century = false;
//bool h12Flag;
//bool pmFlag;
bool wateredLastHour = false;
int cycleCount = 0;

void setup()
{

   Wire.begin();
   SPI.begin();
   Serial.begin(9600);
   
   
  sensorA.begin();
  sensorB.begin();
  //sensorC.begin();
  //sensorD.begin();

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
}

void GetTempAndMoist()
{
  moistureSensorAPercent = map(analogRead(A0), 583, 247, 0, 100);
  moistureSensorBPercent = map(analogRead(A1), 598, 262, 0, 100);
  //moistureSensorCPercent = map(analogRead(A2), 521, 218, 0, 100);
  //moistureSensorDPercent = map(analogRead(A3), 527, 218, 0, 100);
  avgSoilMoisture = ((moistureSensorAPercent + moistureSensorBPercent)/2);

  sensorA.requestTemperatures();
  sensorB.requestTemperatures();
  //sensorC.requestTemperatures();
  //sensorD.requestTemperatures();
  
  // Get the temperatures
  sensorATempC = sensorA.getTempCByIndex(0);
  sensorBTempC = sensorB.getTempCByIndex(0);
  //sensorCTempC = sensorC.getTempCByIndex(0);
  //sensorDTempC = sensorD.getTempCByIndex(0);
  avgSoilTemp = ((sensorATempC + sensorBTempC)/2);

  boxHumid = dhtInsideBox.readHumidity();
  boxTemp = dhtInsideBox.readTemperature();
  airHumid = dhtOutsideBox.readHumidity(); 
  airTemp = dhtOutsideBox.readTemperature();
}

void clearLCDLine(int line)
{               
        lcd.setCursor(0,line);
        for(int n = 0; n < 20; n++) // 20 indicates symbols in line. For 2x16 LCD write - 16
        {
                lcd.print(" ");
        }
}

void UpdateDisplay(String sysStatus, String msg1)
{
  clearLCDLine(0);
  clearLCDLine(1);
  lcd.setCursor(0, 0);
  DateTime now = clock.now();
  lcd.print("-----" + String(now.year()) + "-" + String(now.month()) + "-" + String(now.day())+"-----");
  lcd.setCursor(0,1);
  lcd.print(sysStatus);
  if(msg1 != "")
  {
    clearLCDLine(2);
    lcd.setCursor(0,2);
    lcd.print(msg1);
  }
    clearLCDLine(2);
    clearLCDLine(3);
    lcd.setCursor(0,2);
    lcd.print("Last Data: "  + String(now.hour()) + ":" + String(now.minute()));
    lcd.setCursor(0,3);
    lcd.print("SM:"  + String(avgSoilMoisture) + "%,ST:" + String(avgSoilTemp)+"C");
}

void LogEnvironment(String wateredWithinLastHour)
{
  File dataLog = SD.open("log.txt", FILE_WRITE);
  if(dataLog)
  {
    DateTime now = clock.now();
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
    dataLog.print(avgSoilTemp);
    dataLog.print(",");
    dataLog.print(moistureSensorAPercent);
    dataLog.print(","); 
    dataLog.print(moistureSensorBPercent);
    dataLog.print(","); 
    dataLog.print(avgSoilMoisture);
    dataLog.print(","); 
    dataLog.print(boxHumid);
    dataLog.print(","); 
    dataLog.print(boxTemp);
    dataLog.print(","); 
    dataLog.print(airHumid);
    dataLog.print(","); 
    dataLog.print(airTemp);
    dataLog.print(","); 
    dataLog.println(wateredWithinLastHour);
    dataLog.close();
    digitalWrite(troubleRelayPin, LOW);
  }
  else 
  {
    UpdateDisplay(sysIdle, sysLogWarn);
    digitalWrite(troubleRelayPin, HIGH);
    digitalWrite(valveRelayPin, LOW);
  }
}

void loop()
{
  // Check the temperature inside the housing
  if(boxTemp > maxBoxTemp)
  {
    UpdateDisplay(sysTempWarn, "");
    digitalWrite(troubleRelayPin, HIGH);
  }
  
  bool watering = false;
  GetTempAndMoist();
  Serial.println("CC " + String(cycleCount));
  
  DateTime now = clock.now();
  Serial.println(String(avgSoilMoisture));

  // Check to see if the conditions are met for watering, either dry soil in the morning or
  // if the soil is hot and dry
  if(((now.hour()) > minHourToWater && (now.hour() < maxHourToWater) && (avgSoilMoisture < minSoilMoist))||((avgSoilTemp > maxSoilTemp) && (avgSoilMoisture < minSoilMoist)))
  {

      UpdateDisplay(sysWatering, "");
      digitalWrite(valveRelayPin, HIGH);
      watering = true;
      delay(wateringDelay);
      GetTempAndMoist();
      UpdateDisplay(sysWait, ""); 
      digitalWrite(valveRelayPin, LOW);
  }
  else
  {
      UpdateDisplay(sysIdle, ""); 
  }
  
  
  cycleCount++;
  if(watering == true)
  {
    wateredLastHour = true;
    delay(measuringDelay);
    // Increment the count here since its waited for 10 minutes, 8 in the watering part, and two to measure
    switch(cycleCount) 
    {
    case 3:
      GetTempAndMoist();
      LogEnvironment(String(wateredLastHour));
      UpdateDisplay(sysLog, "");
      wateredLastHour = false;
      Serial.println("logged");
      break;
    }
  }
  else
  {
    delay(measuringDelay + wateringDelay);
    // Increment the count here since its waited for 10 minutes
    now = clock.now();
    switch(cycleCount) 
    {
      case 3:
        GetTempAndMoist();
        LogEnvironment("False");
        UpdateDisplay(sysLog, "");
        Serial.println("logged");
        break;
      }
  }
 
  switch(cycleCount) 
  {
    case 6:
    cycleCount = 0;
    break;
  }
  }
