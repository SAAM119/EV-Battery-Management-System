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


// faults
bool weakCellFault = false;
bool overVoltageFault = false;
bool sensorFault = false;
bool voltageSpikeFault = false;

//Runtime faults 
bool sensordisconnected = false ;
bool adcfrozen = false ;
bool relaymismatch = false ;
bool invalidreading = false ;
bool watchdog = false ;  

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

bool buzzerstate = false;
bool ledstate = false;
bool recoverystate = false ;
bool relaystate = false ;
bool relaydelayrun = false ;

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
  Screentask();
  lcdtask();
  serialtask();
  buzzertask();
  relaytask();
  ledtask();
  Runtimetask();
}

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
// else if (imbalance>20){
//   currentState = warning;
// }

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
}
}
 
  void Runtimetask(){
    bool sensordisfault = false ;

     if (abs(v1 - lastStableV1) > sensorChangeThreshold)
     {
      lastStableV1 = v1 ;
      sensorFreezeTimer1 = millis();
     }
     else 
     {
      if (millis() - sensorFreezeTimer1 > sensorFreezeTime){
        sensordisfault = true;
      }
     }

    sensorFault = sensordisfault;
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
    }
}

        // Serial Task code from here for Task 1 and Task 2
void serialtask()
{
  if (millis() - serialTimer >= serialInterval) {
    serialTimer = millis(); 

                                              // Task 1 

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
}
}

// Screen Task code from here for Task 2
void Screentask() {

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
}
}

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
}
}

