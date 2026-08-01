#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
// 16x2 LCD on the usual I2C backpack address
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ADC pins for the 4 cell taps (voltage dividers in front of these!)
#define CELL1_PIN 34
#define CELL2_PIN 35
#define CELL3_PIN 32
#define CELL4_PIN 33
#define RED_LED 27
#define GREEN_LED 25 
#define YELLOW_LED 26
#define Relay 12
#define Buzzer 14
#define I2C_SDA 21
#define I2C_SCL 22

float Voltage;
float averageVoltage;
float v1,v2,v3,v4;
int p1,p2,p3,p4;
float packVoltage;
float imbalance;
String health;
int maxCell;
int minCell;
int rs;

float readVoltage(int pin) {
  int rawValue = analogRead(pin);
  return (rawValue / 4095.0) * 3.3;
}

int getPercentage(float v) {
  return constrain(map(v * 100,0,330, 0, 100), 0, 100);
}
String Status;

void batterytask();
void Screentask();
void lcdtask();
void serialtask();
void buzzertask();
void relaytask();
void ledtask(); 
void Runtimetask();
void runtimeADCCheck_SensorCheck();
void runtimeInvalidReadingCheck();
void runtimeRelayCheck();
void runtimeWatchdogCheck();
void runtimeModeManager();
// void runtimeRecoveryManager();
// void runtimeLogger();

unsigned long batteryTimer = 0;
unsigned long lcdTimer = 0;
unsigned long serialTimer = 0;
unsigned long buzzerTimer = 0;
unsigned long relayTimer = 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
unsigned long ledTimer = 0;
unsigned long ledpatternTimer = 0 ;
unsigned long buzzerpatternTimer = 0 ;
unsigned long voltageSpikeTimer = 0;
unsigned long recoveryTimer = 0 ;
unsigned long relaydelayTimer = 0;
unsigned long ScreenTImer = 0;
//runtime
unsigned long sensorFreezeTimer1 = 0;
unsigned long sensorFreezeTimer2 = 0;
unsigned long sensorFreezeTimer3 = 0;
unsigned long sensorFreezeTimer4 = 0;
unsigned long adcfreezeTimer = 0;
unsigned long runtimeTimer = 0;

//Watchdog 
unsigned long batteryheartbeat = 0;
unsigned long runtimeheartbeat = 0;
unsigned long screenheartbeat = 0;
unsigned long lcdheartbeat = 0;
unsigned long serialheartbeat = 0;
unsigned long buzzerheartbeat = 0;
unsigned long relayheartbeat = 0;
unsigned long ledheartbeat = 0;



const unsigned long batteryInterval = 100;                                
const unsigned long ScreenInterval=3000; 
const unsigned long lcdInterval = 500;
const unsigned long serialInterval = 1000;
const unsigned long buzzerInterval = 50;
const unsigned long relayInterval = 50;
const unsigned long ledInterval= 50;
const unsigned long voltageSpikeHoldTime = 2000;
const unsigned long recoverydelay = 3000;
const unsigned long relaytrip = 100;
const unsigned long relayrecover = 1000;
const float sensorChangeThreshold = 0.01;
const unsigned long sensorFreezeTime = 5000;
const float adcChangeThreshold = 0.01;
const unsigned long adcFreezeTime = 5000;
const unsigned long runtimeInterval = 100;
const unsigned long watchdogTimeout = 6000; 


// faults
bool weakCellFault = false;
bool overVoltageFault = false;
bool sensorFault = false;
bool voltageSpikeFault = false;
 

//voltage flucatuation detection
float oldV1 = 0;
float oldV2 = 0;
float oldV3 = 0;
float oldV4 = 0;

//runtime task 
float lastStableV1 = 0;
float lastStableV2 = 0;
float lastStableV3 = 0;
float lastStableV4 = 0;

float lastADCV1 = 0;
float lastADCV2 = 0;
float lastADCV3 = 0;
float lastADCV4 = 0;

bool buzzerstate = false;
bool ledstate = false;
bool recoverystate = false ;
bool relaystate = false ;
bool relaydelayrun = false ;
bool sensordisFault = false;
bool sensor1disFault = false;
bool sensor2disFault = false;
bool sensor3disFault = false;
bool sensor4disFault = false;
bool invalidreadingfault = false;
bool adcfrozen = false ;
bool relaymismatchfault = false;

enum WatchdogSource
{
  WD_NONE,
  WD_BATTERY,
  WD_RUNTIME,
  WD_SCREEN,
  WD_LCD,
  WD_SERIAL,
  WD_RELAY,
  WD_LED,
  WD_BUZZER
};

bool watchdogfault = false;
WatchdogSource watchdogSource = WD_NONE;

enum batterystate
{
  healthy,
  warning,
  critical,
  failure

};

 batterystate  currentState = healthy;

 enum Runtimemode
 {
  Normal,
  Degraded,
  Failsafe,
  Shutdown
 };

 Runtimemode currentRuntimemode = Normal ;

 enum Screen
 {
  Cell_Screen,
  Pack_Screen,
  Analytics_Screen,
  Protection_Screen,
  Diagonistic_Screen,
  Fault_Screen
 };

 Screen currentScreen = Cell_Screen;
 Screen previousScreen = Cell_Screen;
 
void setup() {
  pinMode(RED_LED , OUTPUT);
  pinMode(GREEN_LED, OUTPUT) ;
  pinMode(YELLOW_LED , OUTPUT);
  pinMode(Relay, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  Wire.begin(21, 22);   // SDA = 21, SCL = 22
    lcd.init();
    lcd.backlight();

 Serial.begin(115200);
  Serial.println("Voltage Demo Sim...");
}

void loop() {

  batterytask();
  Runtimetask();
  Screentask();
  lcdtask();
  serialtask();
  buzzertask();
  relaytask();
  ledtask(); } 
   void Runtimetask()
{
  if (millis() - runtimeTimer >= runtimeInterval)
    {
      runtimeTimer = millis();

    runtimeADCCheck_SensorCheck();
    runtimeInvalidReadingCheck();
    runtimeRelayCheck();
    runtimeWatchdogCheck();
    runtimeModeManager();
    // runtimeRecoveryManager();
    // runtimeLogger();
    runtimeheartbeat = millis();
    }}

void batterytask()
{
   if (millis() - batteryTimer >= batteryInterval)
    {
        batteryTimer = millis(); 

  v1 = readVoltage(CELL1_PIN);
  v2 = readVoltage(CELL2_PIN);
  v3 = readVoltage(CELL3_PIN);
  v4 = readVoltage(CELL4_PIN);


  p1 = getPercentage(v1);
  p2 = getPercentage(v2);   
  p3 = getPercentage(v3);
  p4 = getPercentage(v4);
// Task 1 start
  if (p1< 30|| p2 < 30 || p3 < 30 || p4 < 30) {
    Status = "Low";
  } 
  else if (p1 > 70 || p2 > 70 || p3 > 70 || p4 > 70) {
    Status = "High";
  } 
  else {
    Status = "Normal";
  }

  float maxVoltage=v1;
     maxCell=1;

if(v2>maxVoltage){
    maxVoltage=v2;
    maxCell=2;
}

if(v3>maxVoltage){
    maxVoltage=v3;
    maxCell=3;
}

if(v4>maxVoltage){
    maxVoltage=v4;
    maxCell=4;
}

float minVoltage=v1;
   minCell=1;
if(v2<minVoltage){
    minVoltage=v2;
    minCell=2;
}
if(v3<minVoltage){
        minVoltage=v3;
        minCell=3;
      }
if(v4<minVoltage){
        minVoltage=v4;
        minCell=4;
    }
    
    packVoltage=v1+v2+v3+v4; //pack voltage calculation
 
    averageVoltage=packVoltage/4.0; //average voltage calculation

    imbalance=((maxVoltage-minVoltage)/averageVoltage)*100;


if (v1>2.9 || v2>2.9 || v3>2.9 || v4>2.9)  {
   health = "Pack failure";
} 
else if(imbalance>20){
  health = "Critical Imbalance";
}
else if (imbalance>10){
  health = "Minor Imbalance";
}
else {
  health = "Healthy"; 
}

// Task 1 end and  Task 2 start

// WEAK CELL 

weakCellFault = false;
if(v1 < 0.7) weakCellFault = true;
if(v2 < 0.7) weakCellFault = true;
if(v3 < 0.7) weakCellFault = true;
if(v4 < 0.7) weakCellFault = true;

// over voltage fault

overVoltageFault = false;
if(v1 > 2.8) overVoltageFault = true;
if(v2 > 2.8) overVoltageFault = true;
if(v3 > 2.8) overVoltageFault = true;
if(v4 > 2.8) overVoltageFault = true;

// sesnor fault
sensorFault = false;

if(v1 < 0.05 || v1 >= 3.1)
    sensorFault = true;

if (v2 < 0.05 || v2 >= 3.1)
  sensorFault = true ;

if(v3 < 0.05 || v3 >= 3.1)
    sensorFault = true;

if (v4 < 0.05 || v4 >= 3.1)
  sensorFault = true ;


// voltage spike fault
bool spikedetected = false;

if(abs(v1-oldV1)>0.5)
   spikedetected = true;

oldV1 = v1;
if (abs(v2-oldV2)>0.5)
    spikedetected = true;

oldV2 = v2;
if (abs(v3-oldV3)>0.5)  
    spikedetected = true;   

oldV3 = v3;
if (abs(v4-oldV4)>0.5)
    spikedetected = true;

oldV4 = v4;

if (spikedetected)
{
    voltageSpikeFault = true;
    voltageSpikeTimer = millis();
}

// Clear warning only after 2 seconds
if (voltageSpikeFault &&
    millis() - voltageSpikeTimer >= voltageSpikeHoldTime)
{
    voltageSpikeFault = false;
}
                                                               // Faults Above 

//Logic to set the current state based on the faults detected

if (sensorFault){
  currentState = failure;

}
else if (overVoltageFault){
  currentState = critical;
}
else if (weakCellFault){
  currentState = critical;
}
else if (voltageSpikeFault){
  currentState = warning;
}

bool anyfault = weakCellFault || overVoltageFault|| voltageSpikeFault || sensorFault ;
if (anyfault){
  recoverystate = false;
}
else {
  if (!recoverystate){
     recoverystate = true  ;
     recoveryTimer = millis();
     rs = 0;
  }
  if (millis() - recoveryTimer >= recoverydelay){
    currentState = healthy;
    rs = 1 ;
  }
}
 batteryheartbeat = millis();
}
}
 
void runtimeModeManager()
{
  if(watchdogfault)
   {
    currentRuntimemode = Shutdown;
   }
  else if (adcfrozen || relaymismatchfault)
   {
    currentRuntimemode = Failsafe;
   }
  else if ( invalidreadingfault)
   {
    currentRuntimemode = Degraded;
   }
  else
   {
    currentRuntimemode = Normal;
   }
}

//Runtime task code from here 
void runtimeADCCheck_SensorCheck(){
    // ADC Frozen fault 
  bool adcChanged = false;

  if(abs(v1-lastADCV1) > adcChangeThreshold)
    adcChanged = true;

  if(abs(v2-lastADCV2) > adcChangeThreshold)
    adcChanged = true;

  if(abs(v3-lastADCV3) > adcChangeThreshold)
    adcChanged = true;

  if(abs(v4-lastADCV4) > adcChangeThreshold)
    adcChanged = true;
      
    if(adcChanged)
  {
    lastADCV1 = v1;
    lastADCV2 = v2;
    lastADCV3 = v3;
    lastADCV4 = v4;

    adcfreezeTimer = millis();

    adcfrozen = false;
  }
  else
{
    if(millis() - adcfreezeTimer >= adcFreezeTime)
    {
        adcfrozen = true;
    }
}
if (adcfrozen){
  sensor1disFault = false;
  sensor2disFault = false;
  sensor3disFault = false;
  sensor4disFault = false;
  sensordisFault = false ;
}

else
{
//1st sensor   sensor fault

    if (abs(v1 - lastStableV1) > sensorChangeThreshold)
{
    lastStableV1 = v1;
    sensorFreezeTimer1 = millis();
    sensor1disFault = false;
    
}
else
{
    if (millis() - sensorFreezeTimer1 >= sensorFreezeTime)
    {
        sensor1disFault = true;
    }
}

//2nd sensor

  if (abs(v2 - lastStableV2) > sensorChangeThreshold)
{
    lastStableV2 = v2;
    sensorFreezeTimer2 = millis();
    sensor2disFault = false;
}
else
{
    if (millis() - sensorFreezeTimer2 >= sensorFreezeTime)
    {
        sensor2disFault = true;
    }
}

//3rd sensor

  if (abs(v3 - lastStableV3) > sensorChangeThreshold)
{
    lastStableV3 = v3;
    sensorFreezeTimer3 = millis();
    sensor3disFault = false;
}
else
{
    if (millis() - sensorFreezeTimer3 >= sensorFreezeTime)
    {
        sensor3disFault = true;
    }
}

//4th sensor

  if (abs(v4 - lastStableV4) > sensorChangeThreshold)
{
    lastStableV4 = v4;
    sensorFreezeTimer4 = millis();
    sensor4disFault = false;
}
else
{
    if (millis() - sensorFreezeTimer4 >= sensorFreezeTime)
    {
        sensor4disFault = true;
    }
}
 
 sensordisFault = sensor1disFault || sensor2disFault || sensor3disFault || sensor4disFault;
} }  

void runtimeInvalidReadingCheck() {
// invalid reading fault  used 2.9 or 0.01 as because wokwi have no more value like ral bettery 
 invalidreadingfault =
  v1 <= 0.01 || v1 >=2.9 || isnan(v1)|| isinf(v1) || 
  v2 <= 0.01 || v2 >=2.9 || isnan(v2)|| isinf(v2) || 
  v3 <= 0.01 || v3 >=2.9 || isnan(v3)|| isinf(v3) || 
  v4 <= 0.01 || v4 >=2.9 || isnan(v4)|| isinf(v4) ;
    
}

void runtimeRelayCheck() {
  //Relay mismatch fault detection
   bool relayfeedbackstate;
   bool desiredrelaystate;

   desiredrelaystate = relaystate;

   // Wokwi Testing 
   relayfeedbackstate = relaystate; 

   // Assuming  have a pin to read the relay state for real hardware 
   //relayfeedbackstate = digitalRead(Relay_pin); 

   relaymismatchfault = ( desiredrelaystate != relayfeedbackstate );
} 

void runtimeWatchdogCheck()
{
    watchdogfault = false;
    watchdogSource = WD_NONE;

    if (millis() - batteryheartbeat > watchdogTimeout)
    {
        watchdogfault = true;
        watchdogSource = WD_BATTERY;
    }

    else if (millis() - runtimeheartbeat > watchdogTimeout)
    {
        watchdogfault = true;
        watchdogSource = WD_RUNTIME;
    }

    else if (millis() - screenheartbeat > watchdogTimeout)
    {
        watchdogfault = true;
        watchdogSource = WD_SCREEN;
    }

    else if (millis() - lcdheartbeat > watchdogTimeout)
    {
        watchdogfault = true;
        watchdogSource = WD_LCD;
    }

    else if (millis() - serialheartbeat > watchdogTimeout)
    {
        watchdogfault = true;
        watchdogSource = WD_SERIAL;
    }

    else if (millis() - relayheartbeat > watchdogTimeout)
    {
        watchdogfault = true;
        watchdogSource = WD_RELAY;
    }

    else if (millis() - ledheartbeat > watchdogTimeout)
    {
        watchdogfault = true;
        watchdogSource = WD_LED;
    }

    else if (millis() - buzzerheartbeat > watchdogTimeout)
    {
        watchdogfault = true;
        watchdogSource = WD_BUZZER;
    }
}

// Buzzer Task starts for Task 2 
void buzzertask() {
  
 if (millis() - buzzerTimer >= buzzerInterval) {
    buzzerTimer = millis();

  digitalWrite(Buzzer, 0);

switch(currentState)
{
case healthy:
   digitalWrite(Buzzer, 0);
    break;

case warning:
 if (millis() - buzzerpatternTimer >=500) {
    buzzerpatternTimer = millis();
    buzzerstate = !buzzerstate;
    {
    digitalWrite(Buzzer, buzzerstate);
 }
}
    break;

case critical:
   if (millis() - buzzerpatternTimer >=150) {
    buzzerpatternTimer = millis();
    buzzerstate = !buzzerstate;
    {
    digitalWrite(Buzzer, buzzerstate);
 }
}
    break;
   
case failure:
    digitalWrite(Buzzer, 1);
    break;
}
buzzerheartbeat = millis();

}
  }

//LED Task code from here
void ledtask() {
  
 if (millis() - ledTimer >= ledInterval) {
    ledTimer = millis();

  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);

switch(currentState)
{
case healthy:
   digitalWrite(GREEN_LED, 1);
    break;

case warning:
 if (millis() - ledpatternTimer >=250) {
    ledpatternTimer = millis();
    ledstate = !ledstate;
    {
 digitalWrite(YELLOW_LED, ledstate);
 }
}
   break;

case critical:
   if (millis() - ledpatternTimer >=150) {
    ledpatternTimer = millis();
    ledstate = !ledstate;
    {
    digitalWrite(RED_LED, ledstate);
 }
}
    break;
   
case failure:
    digitalWrite(RED_LED, 1);
    break;
}
ledheartbeat = millis();

}
  }

//Relay Task code from here For Task 2 Only for Faults and relay chatter 
void relaytask()
{
    if(millis() - relayTimer >= relayInterval)
    {
        relayTimer = millis();

  bool desiredRelayState;

    if(currentState == healthy)
      desiredRelayState = false;
    else
      desiredRelayState = true;

if(desiredRelayState != relaystate)
{
    if(!relaydelayrun)
    {
        relaydelayrun = true;
        relaydelayTimer = millis();
    }

    unsigned long delayTime;

    if(desiredRelayState)
        delayTime = relaytrip;
    else
        delayTime = relayrecover;

    if(millis() - relaydelayTimer >= delayTime)
    {
        relaystate = desiredRelayState;
        digitalWrite(Relay, relaystate);
        relaydelayrun = false;
    }
        }
    else
   {
        relaydelayrun = false;
    }
    relayheartbeat = millis();  
    }
}

// Serial Task code from here for Task 1 and Task 2
void serialtask()
{
  if (millis() - serialTimer >= serialInterval) {
    serialTimer = millis(); 

                                            // Task 1 
  Serial.println("-------------------------");
  Serial.println("BMS By Aman");
  Serial.println("-------------------------");
  Serial.print("C1 : ");
  Serial.println(v1);
  Serial.print("C2 : ");
  Serial.println(v2);
  Serial.print("C3 : ");
  Serial.println(v3);
  Serial.print("C4 : ");
  Serial.println(v4);

  Serial.print("Strongest Cell: C");
  Serial.println(maxCell);
  Serial.print("Weakest Cell: C");
  Serial.println(minCell);

  Serial.print("Pack Voltage: ");
  Serial.println(packVoltage);

  Serial.print("Average Voltage: ");
  Serial.println(averageVoltage);

  Serial.print("Imbalance: ");
  Serial.print(imbalance, 2);
  Serial.println("%");

  Serial.print("Health: ");
  Serial.println(health); 

  Serial.print("Status: ");
  Serial.println(Status);

                                             //Task 2

  if (weakCellFault) {
    Serial.println("Weak Cell Fault Detected!");
  }

  if (overVoltageFault) {
    Serial.println("Over Voltage Fault Detected!");
  }
  if (sensorFault) {
    Serial.println("Sensor Fault Detected!");
  }
  if (voltageSpikeFault) {
    Serial.println("Voltage Spike Fault Detected!");
  }

                                   //State Print
                                   
Serial.print("State : ");

switch(currentState)
 {
    case healthy:
        Serial.println("HEALTHY");
        break;

    case warning:
        Serial.println("WARNING");
        break;

    case critical:
        Serial.println("CRITICAL");
        break;

    case failure:
        Serial.println("FAILURE");
        break;
 }
 if (adcfrozen)
   Serial.println("ADC Frozen");

 if (sensor1disFault)
   Serial.println("Sensor 1 disconnected");

 if (sensor2disFault)
   Serial.println("Sensor 2 disconnected");

if (sensor3disFault)
   Serial.println("Sensor 3 disconnected");

 if (sensor4disFault)
   Serial.println("Sensor 4 disconnected");

 if (invalidreadingfault)
  Serial.println("invalid reading");

  if (relaymismatchfault)
  Serial.println("Relay Mismatch Fault");
  else 
  Serial.println("Relay Feedback Normal");
  
  switch (watchdogSource){
    case WD_BATTERY:
      Serial.println("WD Battery Fault");
      break;

    case WD_RUNTIME:
      Serial.println("WD Runtime Fault");
      break;

    case WD_SCREEN:
      Serial.println("WD Screen Fault");
      break;

    case WD_LCD:
      Serial.println("WD LCD Fault");
      break;

    case WD_SERIAL:
      Serial.println("WD Serial Fault");
      break;

    case WD_BUZZER:
      Serial.println("WD Buzzer Fault");
      break;

    case WD_RELAY:
      Serial.println("WD Relay Fault");
      break;

    case WD_LED:
      Serial.println("WD LED Fault");
      break;

    case WD_NONE:
    default:
      Serial.println("Watchdog Normal");
      break;
  }

  Serial.print("Runtime Mode: ");
  switch (currentRuntimemode)
  {
    case Normal:
      Serial.println("Normal");
      break;

    case Degraded:
      Serial.println("Degraded");
      break;

    case Failsafe:
      Serial.println("Failsafe");
      break;

    case Shutdown:
      Serial.println("Shutdown");
      break;
  }
  

  serialheartbeat = millis();
}}
 

// Screen Task code from here for Task 2
void Screentask() {
    screenheartbeat = millis();

   bool anyFault = weakCellFault || overVoltageFault || sensorFault || voltageSpikeFault;

      if(anyFault) 
      {
       if(currentScreen != Fault_Screen)
         {
        currentScreen = Fault_Screen;
        }
          return;
      }

      if(currentScreen == Fault_Screen)
      {
          currentScreen = Cell_Screen;
      }

  if (millis() - ScreenTImer >= ScreenInterval) {
    ScreenTImer = millis();  


  switch (currentScreen)
      {
      case Cell_Screen:
        currentScreen = Pack_Screen ;
        break;

        case Pack_Screen :
        currentScreen = Analytics_Screen;
        break;

        case Analytics_Screen :
        currentScreen = Protection_Screen;
        break;

        case Protection_Screen:
        currentScreen = Diagonistic_Screen;
        break;
      
        case Diagonistic_Screen :
        currentScreen = Cell_Screen;
        break;
      }
    
}}

// LCD Task code from here for Task 2
void lcdtask() {
  

  if (millis() - lcdTimer >= lcdInterval) {
    lcdTimer = millis();  

    if(currentScreen != previousScreen)
   {
    lcd.clear();
    previousScreen = currentScreen;
   }
  
   
   switch (currentScreen)
    {

   case Fault_Screen:

      lcd.setCursor(0,0);
      lcd.print("FAULT:");

      lcd.setCursor(0,1);
      if(sensorFault)
          lcd.print("Sensor Fault");

      else if(overVoltageFault)
          lcd.print("Over Voltage");

      else if(weakCellFault)
          lcd.print("Weak Cell");

      else if(voltageSpikeFault)
          lcd.print("Voltage Spike");

      break;

   case Cell_Screen:

    lcd.setCursor(0,0);
    lcd.print("C1:");
    lcd.print(v1,1);
    lcd.print(" V");

    lcd.print(" C2:");
    lcd.print(v2,1);
    lcd.print(" V");

    lcd.setCursor(0,1);

    lcd.print("C3:");
    lcd.print(v3,1);
    lcd.print(" V");

    lcd.print(" C4:");
    lcd.print(v4,1);
    lcd.print(" V");
    break;

   case Pack_Screen:

    lcd.setCursor(0,0);
    lcd.print("Pack:");
    lcd.print(packVoltage,1);
    lcd.print(" V");

    lcd.setCursor(0,1);
    lcd.print("Avg:");
    lcd.print(averageVoltage,2);
    lcd.print(" V");

    break;

   case Analytics_Screen:
   
   lcd.setCursor(0,0);
   lcd.print("Imbalance :");
   lcd.print(imbalance,1);

   lcd.setCursor(0,1);
   lcd.print("Health :");
   lcd.print(health);

    break;

   case Protection_Screen:
   
   lcd.setCursor(0,0);
   lcd.print("State :");

      switch(currentState)
      {
          case healthy:
              lcd.print("Healthy");
              break;

          case warning:
              lcd.print("Warning");
              break;

          case critical:
              lcd.print("Critical");
              break;

          case failure:
              lcd.print("Failure");
              break;
      }

   lcd.setCursor(0,1);
   lcd.print("Relay :");

      if(relaystate)
        lcd.print("Open");
      else 
        lcd.print("Close");
      break;

  
   case Diagonistic_Screen:
   
   lcd.setCursor(0,0);
   lcd.print("Weak Cell : C");
   lcd.print(minCell);

   lcd.setCursor(0,1);
   lcd.print("Strong Cell : C");
   lcd.print(maxCell);

   break;

   }
   lcdheartbeat = millis();
}
}



