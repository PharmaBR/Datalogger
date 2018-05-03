#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <RTClib.h>
#include "user_interface.h"
RTC_DS1307 RTC;
#include <BlynkSimpleEsp8266.h>

//Include ATH code to Blynk

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[]= "YourAuthToken";
// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Cherin";
char pass[] = "A2609c41989";

os_timer_t mTimer;


//Declare Variables
bool Timeout = false;
DateTime DateTimeNow;
double TempNow = 0;
double PHNow = 0;
double CO2Now = 0;

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

// Data wire is plugged into pin 6 on the Arduino
#define ONE_WIRE_BUS 2
 
// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

void tCallback(void *tCall){
   Timeout = true;
}

void startThermometer(){
  sensors.begin();  
}

void startClock(){
      RTC.begin(); 
       //Start up the RTC
  if (! RTC.isrunning()) {
    Serial.println("RTC not working");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  } else {
    Serial.println("RTC started");
  }
}

void startSerial(){
  Serial.begin(9600);
  Serial.println("Serial started");
}


void startWiFi(){
 // WiFi.begin("Cherin", "A2609c41989"); Login para banco que já está ok.
 Blynk.begin(auth, ssid, pass); //tentando usar o blynk em cima do projeto
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 8442);
  //Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8442);
  Serial.println("WiFi started");
  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion
    delay(500);
    Serial.println("Waiting for connection");
  }  
}

void startTimer(){
  os_timer_setfn(&mTimer, tCallback, NULL);
  os_timer_arm(&mTimer, 30000, true);
}

void initialize(){
  startSerial();
  startThermometer();
  startWiFi();
  startClock();
  startTimer();
}

double getTemperature(){
  Serial.println("Reading temperature");
  sensors.requestTemperatures();
  int temperature = sensors.getTempCByIndex(0);
  Serial.println("The temperature is: ");
  Serial.print(temperature);
  Serial.print(" ºC");
  return temperature;  
}

DateTime getNow(){
  Serial.println("Reading clock");
  DateTime now = RTC.now();
  char strdatetime [25] = "";
  Serial.println("The clock is: ");
  Serial.print(nowToStr(now));
  return now;
}

char* nowToStr(DateTime now){
  char strdatetime [25] = "";
  sprintf(strdatetime, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  return strdatetime;
}

void readData(){
  DateTimeNow = getNow();
  TempNow = getTemperature();  
}

void sendDataToServer(){
  Serial.println("Sending data");
  StaticJsonBuffer<350> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& data = root.createNestedArray("data");
  JsonObject& logger = data.createNestedObject();
  logger["temperature"] = sensors.getTempCByIndex(0);
  logger["ph"] = PHNow;
  logger["co2"] = CO2Now;
  logger["date_time"] = nowToStr(DateTimeNow);
  root.printTo(Serial);
  char JSONmessageBuffer[350];
  root.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  HTTPClient http;    //Declare object of class HTTPClient
  http.begin("http://kombuchatea.life/kombucha/datalogger/save");      //Specify request destination
  http.addHeader("Content-Type", "application/json");  //Specify content-type header
  int httpCode = http.POST(JSONmessageBuffer);   //Send the request
  String payload = http.getString();                                        //Get the response payload
  if (httpCode==200){
    Serial.println("Success");
  } else {
    Serial.println("Error ");
    Serial.print(httpCode);   //Print HTTP return code
    Serial.println(payload);    //Print request response payload
  }
}
 
void setup(void)
{
  initialize();
}
 
 
void loop(void)
{
  Blynk.run();
  // You can inject your own code or combine it with other sketches.
  // Check other examples on how to communicate with Blynk. Remember
  // to avoid delay() function!
  if (Timeout){
    readData();
    sendDataToServer();
    Timeout = false;
  }
}

