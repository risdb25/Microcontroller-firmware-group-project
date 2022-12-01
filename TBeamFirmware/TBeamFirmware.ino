#include <Wire.h> //bme
#include <Adafruit_Sensor.h> //bme
#include <Adafruit_BME280.h> //bme
#include <OneWire.h> //ds18
#include <DallasTemperature.h> //ds18
#include <SHT21.h> //sht21
#include <Adafruit_TSL2591.h> //tsl
#include <SPI.h> //LoRa
#include <LoRa.h> //LoRa
#include "SSD1306.h" //LoRa

#define SEALEVELPRESSURE_HPA (1013.25) //bme
#define SCK     5    //GPIO5  -- SX1278's SCK -- LoRa
#define MISO    19   //GPIO19 -- SX1278's MISnO -- LoRa
#define MOSI    27   //GPIO27 -- SX1278's MOSI -- LoRa
#define SS      18   //GPIO18 -- SX1278's CS -- LoRa
#define RST     14   // GPIO14 -- SX1278's RESET -- LoRa
#define DI0     26   //GPIO26 -- SX1278's IRQ(Interrupt Request) -- LoRa
#define BAND  868E6 //-- LoRa
#define millisecondsToSecondsFactor 1000000// sleep -- conversion factor for micro seconds to seconds
#define secondsToSleep  4 //sleep -- number of seconds to put T-Beam to sleep for

int i; //used throughout the program, this is used to count number of readings taken.







//BME280 variables
Adafruit_BME280 bme;
float airTempArray[5];
float pressureArray[5];
float altitudeArray[5];
float humidityArray[5];

float avgAirTemp;
float avgPressure;
float avgAltitude;
float avgHumidity;

//DS18B20 variables
const int pinUsedByDS18 = 23; //number assigned to this variable = pin on board that the sensor is connected to.
OneWire oneWire(pinUsedByDS18);
DallasTemperature dallasTemp(&oneWire);
float soilTempArray[5];
float avgSoilTemp;

//SEN0114 variables
const int pinUsedBySEN0114 = 4;
int soilMoistureArray[5];
int avgSoilMoisture;

//SHT21 variables
SHT21 sht;
float airTempArraySHT[5];
float humidityArraySHT[5];
float avgAirTempSHT;
float avgHumiditySHT;

//TSL2591 variables
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
int luminosityArray[5];
int avgLuminosity;

//LoRa variables
SSD1306 display(0x3c, 21, 22);
String rssi = "RSSI --";
String packSize = "--";
String packet;








void setup()
{
  Serial.begin(9600); //used for serial monitor testing purposes
  bme.begin(0x76);
  dallasTemp.begin();
  Wire.begin();
  setUpTSL2591(); //function to set up TSL2591 sensor
  setUpLoRa(); //function to set up LoRa sender communication
}








/*
 *Next function is loop(), which runs continuously from top to bottom. There are three main parts to this function 
 *1 - over the course of ten minutes (controlled by for loop and delay), five readings are taken two minutes apart from each sensor. 
 *        After ten minutes, the for loop condition is not satisfied, thus the code beneath the for loop is executed.
 *2 - for each array where the last five readings are stored, an average reading is calculated. These average results are then stored.
 *3 - These results are then sent via LoRa communication to the database.
 *4 - process repeats from part 1.
 */
 
void loop() 
{
  for (i = 0; i < 5; i++) //when i = 5, loop will break and 10 minutes will have passed
  {
    retrieveBMEReadings();
//    retrieveDS18Readings(); --this method call causes a program freeze issue due to a conflict with LoRa, so is commented out until a fix can be applied.
    retrieveSEN0114Readings();
    retrieveSHT21Readings();    
    retrieveTSL2591Readings();
//    esp_sleep_enable_timer_wakeup(millisecondsToSecondsFactor * secondsToSleep); //will be a two minute delay
//    esp_light_sleep_start();
    delay(5000);
  }

  //Calculate and store average readings for each sensor
  avgAirTemp = calculateAverageReadings(airTempArray);
  avgPressure = calculateAverageReadings(pressureArray);
  avgAltitude = calculateAverageReadings(altitudeArray);
  avgHumidity = calculateAverageReadings(humidityArray);
  avgSoilTemp = calculateAverageReadings(soilTempArray);
  avgSoilMoisture = calculateAverageReadings(soilMoistureArray);
  avgAirTempSHT = calculateAverageReadings(airTempArraySHT);
  avgHumiditySHT = calculateAverageReadings(humidityArraySHT);
  avgLuminosity = calculateAverageReadings(luminosityArray);
  
  //Send average results using LoRa
  sendAverageResultsViaLoRa();

  //(loop method now restarts from first line)
}







/*Setup methods, this code has been separated from the setup function for the sake of
*the setup function's readability.
*/
void setUpTSL2591()
{
  tsl.setGain(TSL2591_GAIN_MED);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
}

void setUpLoRa()
{
  pinMode(16,OUTPUT);
  pinMode(2,OUTPUT);
  
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
  
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(868100000)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  
  display.init();
  display.flipScreenVertically();  
  display.setFont(ArialMT_Plain_10);
}








//Methods for retrieving readings, methods are split per sensor
void retrieveBMEReadings()
{
  airTempArray[i] = bme.readTemperature();
  pressureArray[i] = bme.readPressure() / 100.0F;
  altitudeArray[i] = bme.readAltitude(SEALEVELPRESSURE_HPA);
  humidityArray[i] = bme.readHumidity();
}

void retrieveDS18Readings() //(method not currently being called)
{
  dallasTemp.requestTemperatures();
  soilTempArray[i] = dallasTemp.getTempCByIndex(0);
}

void retrieveSEN0114Readings()
{
  soilMoistureArray[i] = analogRead(pinUsedBySEN0114);
}

void retrieveSHT21Readings()
{
  airTempArraySHT[i] = sht.getTemperature();
  humidityArraySHT[i] = sht.getHumidity();
}

void retrieveTSL2591Readings()
{
  luminosityArray[i] = tsl.getLuminosity(TSL2591_VISIBLE);
}








/*Methods for calculting average readings stored in each array.
 * This method has been overloaded because some sensors return integer readings
 * and some return float readings, therefore separate methods have been created for 
 * dealing with each data type.
 */
int calculateAverageReadings(int tempArr[])
{
  int sum = 0;
  int avg = 0;
  
  for (int i = 0; i < 5; i++)
  {
    sum += tempArr[i];
  }

  avg = sum / 5;
  return avg;
}

float calculateAverageReadings(float tempArr[])
{
  float sum = 0;
  float avg = 0;

  for (int i = 0; i < 5; i++)
  {
    sum += tempArr[i];
  }

  avg = sum / 5;
  return avg;
}







/*Method for the sending of average readings via LoRa.
 * There are currently 4 readings being sent. Code for sending the rest of the readings
 * will be written in the next sprint.
 */
void sendAverageResultsViaLoRa()
{
  //send packets
  LoRa.beginPacket();
  LoRa.print("\nAir temperature: ");
  LoRa.print(avgAirTemp);
  LoRa.print("\n");
  
  LoRa.print("Pressure: ");
  LoRa.print(avgPressure);
  LoRa.print("\n");
  
  LoRa.print("Altitude: ");
  LoRa.print(avgAltitude);
  LoRa.print("\n");
  
  LoRa.print("Humidity: ");
  LoRa.print(avgHumidity);
  LoRa.print("\n");

//  LoRa.print("Soil temperature: ");
//  LoRa.print(avgSoilTemp);
//  LoRa.print("\n");

  LoRa.print("Soil moisture: ");
  LoRa.print(avgSoilMoisture);
  LoRa.print("\n");

  LoRa.print("Air temperature (SHT): ");
  LoRa.print(avgAirTempSHT);
  LoRa.print("\n");

  LoRa.print("Humidity (SHT): ");
  LoRa.print(avgHumiditySHT);
  LoRa.print("\n");
  
  LoRa.print("Luminosity: ");
  LoRa.print(avgLuminosity);
  LoRa.print("\n\n");
  LoRa.endPacket();
}
