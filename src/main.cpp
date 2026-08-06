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
void runtimeADCCheck();
void runtimeSensorCheck();
void runtimeInvalidReadingCheck();
void runtimeRelayCheck();
void runtimeWatchdogCheck();
void runtimeModeManager();
void runtimefaultisolation();
void faultmodulation();
void runtimeLogger();
// void runtimeRecoveryManager();


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
unsigned long adcFreezeTimer;
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
bool batterySensorFault = false;
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

int lastRaw1;
int lastRaw2;
int lastRaw3;
int lastRaw4;


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
bool invalidreadingcell1 = false;
bool invalidreadingcell2 = false;
bool invalidreadingcell3 = false;
bool invalidreadingcell4 = false;
bool adcfrozen = false ;
bool relaymismatchfault = false;


//Fault Isolation
bool cell1valid = true;
bool cell2valid = true; 
bool cell3valid = true;
bool cell4valid = true;

// fault modulation
bool relaytaskenable = true;
bool lcdtaskenable = true ;
bool screentaskenable = true;
bool serialtaskenable = true;
bool buzzertaskenable = true;
bool ledtaskenable = true;

bool relaycontrolenable = true;
bool lcdcontrolenable = true ;
bool ledcontrolenable = true ;
bool screencontrolenable = true ;
bool buzzercontrolenable = true ;
bool serialcontrolenable = true;



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
 //logger 
// Previous Runtime Fault States

bool lastSensor1Fault = false;
bool lastSensor2Fault = false;
bool lastSensor3Fault = false;
bool lastSensor4Fault = false;

bool lastInvalidCell1 = false;
bool lastInvalidCell2 = false;
bool lastInvalidCell3 = false;
bool lastInvalidCell4 = false;

bool lastADCFrozen = false;
bool lastRelayMismatch = false;
bool lastWatchdogFault = false;
bool  waitingForRecoveryCell1 = false;
bool  waitingForRecoveryCell2 = false;
bool  waitingForRecoveryCell3 = false;
bool  waitingForRecoveryCell4 = false;

// Previous Runtime Mode
Runtimemode lastRuntimeMode = Normal;

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

//Battery monitoring task
void batterytask()
{ 
   if (millis() - batteryTimer >= batteryInterval)
    {
        batteryTimer = millis(); 

  v1 = readVoltage(CELL1_PIN);
  v2 = readVoltage(CELL2_PIN);
  v3 = readVoltage(CELL3_PIN);
  v4 = readVoltage(CELL4_PIN);

  if (cell1valid)
  p1 = getPercentage(v1);
  else p1=-1;
  
  if (cell2valid)
  p2 = getPercentage(v2); 
  else p2=-1;
  
  if(cell3valid)  
  p3 = getPercentage(v3);
  else  p3=-1;

   if(cell4valid)
  p4 = getPercentage(v4);
  else p4=-1;
  
// Task 1 start
  if ((cell1valid && p1 < 30) || (cell2valid && p2 < 30) || (cell3valid && p3 < 30) || (cell4valid && p4 < 30)) {
    Status = "Low";
  } 
  else if ((cell1valid && p1 > 70) || (cell2valid && p2 > 70) || (cell3valid && p3 > 70) || (cell4valid && p4 > 70)) {
    Status = "High";
  } 
  else {
    Status = "Normal";
  }

  float maxVoltage=-1;
     maxCell=0;

if(cell1valid && v1>maxVoltage){
    maxVoltage=v1;
    maxCell=1;
}

if(cell2valid && v2>maxVoltage){
    maxVoltage=v2;
    maxCell=2;
}

if(cell3valid && v3>maxVoltage){
    maxVoltage=v3;
    maxCell=3;
}

if(cell4valid && v4>maxVoltage){
    maxVoltage=v4;
    maxCell=4;
}

float minVoltage=100;
   minCell=0;

if (cell1valid && v1<minVoltage){
    minVoltage=v1;
    minCell=1;
}
if(cell2valid && v2<minVoltage){
    minVoltage=v2;
    minCell=2;
}
if(cell3valid && v3<minVoltage){
        minVoltage=v3;
        minCell=3;
      }
if(cell4valid && v4<minVoltage){
        minVoltage=v4;
        minCell=4;
    }
    
    //pack voltage calculation
    packVoltage=0;
    if (cell1valid) packVoltage+=v1;
    if (cell2valid) packVoltage+=v2;
    if (cell3valid) packVoltage+=v3;
    if (cell4valid) packVoltage+=v4;

    //average voltage calculation
    int activecells = 0;
    if (cell1valid) activecells++;
    if (cell2valid) activecells++;
    if (cell3valid) activecells++;
    if (cell4valid) activecells++;

    if (activecells> 0)
     averageVoltage=packVoltage/activecells;
    else 
    averageVoltage=0;  
   
    //Imbalance
   if(activecells>2)
    imbalance=((maxVoltage-minVoltage)/averageVoltage)*100;
   else
   imbalance=0;

   //Pack health calculation based on the cell voltages and imbalance
if ((cell1valid && v1>2.9) || (cell2valid && v2>2.9) || (cell3valid && v3>2.9) || (cell4valid && v4>2.9))  {
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

if(cell1valid)
{
    if(v1 < 0.7)
        weakCellFault = true;
}

if(cell2valid)
{
    if(v2 < 0.7)
        weakCellFault = true;
}

if(cell3valid)
{
    if(v3 < 0.7)
        weakCellFault = true;
}

if(cell4valid)
{
    if(v4 < 0.7)
        weakCellFault = true;
}

// over voltage fault

overVoltageFault = false;
if(cell1valid && v1 > 2.8) overVoltageFault = true;
if(cell2valid && v2 > 2.8) overVoltageFault = true;
if(cell3valid && v3 > 2.8) overVoltageFault = true;
if(cell4valid && v4 > 2.8) overVoltageFault = true;

// sesnor fault
batterySensorFault = false;

if(cell1valid)
{
    if(v1 < 0.05 || v1 >= 3.1)
        batterySensorFault = true;
}

if(cell2valid)
{
    if(v2 < 0.05 || v2 >= 3.1)
        batterySensorFault = true;
}

if(cell3valid)
{
    if(v3 < 0.05 || v3 >= 3.1)
        batterySensorFault = true;
}

if(cell4valid)
{
    if(v4 < 0.05 || v4 >= 3.1)
        batterySensorFault = true;
}


// voltage spike fault
bool spikedetected = false;
if (cell1valid){
   if (abs(v1-oldV1)>0.5)
    spikedetected = true;
    oldV1 = v1;
  }
  if(cell2valid){
   if (abs(v2-oldV2)>0.5)
    spikedetected = true;
    oldV2 = v2;
  }
  if(cell3valid){
   if (abs(v3-oldV3)>0.5)
    spikedetected = true;
    oldV3 = v3;
  }
 if (cell4valid){
   if (abs(v4-oldV4)>0.5)
    spikedetected = true;
    oldV4 = v4;
 }

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

if (batterySensorFault){
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

bool anyfault = weakCellFault || overVoltageFault|| voltageSpikeFault || batterySensorFault ;
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
 
//Runtime task code from here 
void Runtimetask()
{
  if (millis() - runtimeTimer >= runtimeInterval)
    {
      runtimeTimer = millis();

    runtimeADCCheck();
    runtimeSensorCheck();
    runtimeInvalidReadingCheck();
    runtimeRelayCheck();
    runtimeWatchdogCheck();
    runtimeModeManager();
    runtimefaultisolation();
    faultmodulation();
     runtimeLogger();
    // runtimeRecoveryManager();
    
    runtimeheartbeat = millis();
    }}

void runtimeModeManager()
{
    // Highest Priority
    if (watchdogfault)
    {
        currentRuntimemode = Shutdown;
        return;
    }

    // Serious Hardware Faults
    if (adcfrozen || relaymismatchfault)
    {
        currentRuntimemode = Failsafe;
        return;
    }

    // Recoverable Faults
    if (sensordisFault ||
        invalidreadingcell1 ||
        invalidreadingcell2 ||
        invalidreadingcell3 ||
        invalidreadingcell4)
    {
        currentRuntimemode = Degraded;
        return;
    }

    // Healthy
    currentRuntimemode = Normal;
}

void runtimeADCCheck()
{
    int raw1 = analogRead(CELL1_PIN);
    int raw2 = analogRead(CELL2_PIN);
    int raw3 = analogRead(CELL3_PIN);
    int raw4 = analogRead(CELL4_PIN);

    bool adcChanged = false;

    if(raw1 != lastRaw1) adcChanged = true;
    if(raw2 != lastRaw2) adcChanged = true;
    if(raw3 != lastRaw3) adcChanged = true;
    if(raw4 != lastRaw4) adcChanged = true;

    if(adcChanged)
    {
        lastRaw1 = raw1;
        lastRaw2 = raw2;
        lastRaw3 = raw3;
        lastRaw4 = raw4;

        adcFreezeTimer = millis();

        adcfrozen = false;
    }
    else
    {
        if(millis() - adcFreezeTimer >= adcFreezeTime)
        {
            adcfrozen = true;
        }
    }
}

void runtimeSensorCheck()
{
    sensor1disFault = false;
    sensor2disFault = false;
    sensor3disFault = false;
    sensor4disFault = false;

    int raw1 = analogRead(CELL1_PIN);
    int raw2 = analogRead(CELL2_PIN);
    int raw3 = analogRead(CELL3_PIN);
    int raw4 = analogRead(CELL4_PIN);

    if(raw1 <= 5 || raw1 >= 4090)
        sensor1disFault = true;

    if(raw2 <= 5 || raw2 >= 4090)
        sensor2disFault = true;

    if(raw3 <= 5 || raw3 >= 4090)
        sensor3disFault = true;

    if(raw4 <= 5 || raw4 >= 4090)
        sensor4disFault = true;

 sensordisFault = sensor1disFault || sensor2disFault || sensor3disFault || sensor4disFault;
} 

void runtimeInvalidReadingCheck()
{
    // Cell 1
    if(sensor1disFault)
    {
        invalidreadingcell1 = false;
    }
    else
    {
        invalidreadingcell1 =
            (v1 <= 0.05) ||
            (v1 >= 2.9) ||
            isnan(v1) ||
            isinf(v1);
    }

    // Cell 2
    if(sensor2disFault)
    {
        invalidreadingcell2 = false;
    }
    else
    {
        invalidreadingcell2 =
            (v2 <= 0.05) ||
            (v2 >= 2.9) ||
            isnan(v2) ||
            isinf(v2);
    }

    // Cell 3
    if(sensor3disFault)
    {
        invalidreadingcell3 = false;
    }
    else
    {
        invalidreadingcell3 =
            (v3 <= 0.05) ||
            (v3 >= 2.9) ||
            isnan(v3) ||
            isinf(v3);
    }

    // Cell 4
    if(sensor4disFault)
    {
        invalidreadingcell4 = false;
    }
    else
    {
        invalidreadingcell4 =
            (v4 <= 0.05) ||
            (v4 >= 2.9) ||
            isnan(v4) ||
            isinf(v4);
    }
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

void runtimefaultisolation()
{
cell1valid = !sensor1disFault && !invalidreadingcell1;
cell2valid = !sensor2disFault && !invalidreadingcell2;
cell3valid = !sensor3disFault && !invalidreadingcell3 ;
cell4valid = !sensor4disFault && !invalidreadingcell4 ;
}

void faultmodulation()
{
    // Default
    relaycontrolenable = true;
    lcdcontrolenable = true;
    screencontrolenable = true;
    serialcontrolenable = true;
    ledcontrolenable = true;
    buzzercontrolenable = true;

    switch(currentRuntimemode)
    {
        case Normal:
            break;

        case Degraded:
            // Healthy modules continue
            break;

        case Failsafe:

            relaystate = true;   
             digitalWrite(Relay, HIGH);         // Open relay
            relaycontrolenable = false;

            break;

        case Shutdown:

            relaystate = true;

            relaycontrolenable = false;
            screencontrolenable = false;
            ledcontrolenable = false;
            buzzercontrolenable = false;

            break;

            
    }

    // Individual module failures override runtime mode

    if(watchdogSource == WD_LCD)
        lcdcontrolenable = false;

    if(watchdogSource == WD_SCREEN)
        screencontrolenable = false;

    if(watchdogSource == WD_SERIAL)
        serialcontrolenable = false;

    if(watchdogSource == WD_LED)
        ledcontrolenable = false;

    if(watchdogSource == WD_BUZZER)
        buzzercontrolenable = false;

    if(relaymismatchfault)
        relaycontrolenable = false;

        
}

void runtimeLogger()
{

   if(sensor1disFault != lastSensor1Fault)
{
    if(sensor1disFault)
    {
        Serial.print("[");
        Serial.print(millis());
        Serial.println(" ms] Sensor 1 Disconnected");
    }
    else
    {
        Serial.print("[");
        Serial.print(millis());
        Serial.println(" ms] Sensor 1 Recovered");
    }

    lastSensor1Fault = sensor1disFault;
}

if(sensor2disFault != lastSensor2Fault)
{
    Serial.print("[");
    Serial.print(millis());
    Serial.print(" ms] ");

    if(sensor2disFault)
        Serial.println("Sensor 2 Disconnected");
    else
        Serial.println("Sensor 2 Recovered");

    lastSensor2Fault = sensor2disFault;
}

if(sensor3disFault != lastSensor3Fault)
{
    Serial.print("[");
    Serial.print(millis());
    Serial.print(" ms] ");

    if(sensor3disFault)
        Serial.println("Sensor 3 Disconnected");
    else
        Serial.println("Sensor 3 Recovered");

    lastSensor3Fault = sensor3disFault;
}

if(sensor4disFault != lastSensor4Fault)
{
    Serial.print("[");
    Serial.print(millis());
    Serial.print(" ms] ");

    if(sensor4disFault)
        Serial.println("Sensor 4 Disconnected");
    else
        Serial.println("Sensor 4 Recovered");

    lastSensor4Fault = sensor4disFault;

}

// for invalid readings

if (invalidreadingcell1 && !lastInvalidCell1)
{
    Serial.print("[");
    Serial.print(millis());
    Serial.println(" ms] Cell 1 Invalid Reading");

    waitingForRecoveryCell1 = true;
}

// Reading Normal (only after complete recovery)
if (waitingForRecoveryCell1 &&!sensor1disFault &&!invalidreadingcell1 && lastInvalidCell1)
{
    Serial.print("[");
    Serial.print(millis());
    Serial.println(" ms] Cell 1 Reading Normal");

    waitingForRecoveryCell1 = false;
}

// Update previous state
lastInvalidCell1 = invalidreadingcell1;

if (invalidreadingcell2 && !lastInvalidCell2)
{
    Serial.print("[");
    Serial.print(millis());
    Serial.println(" ms] Cell 2 Invalid Reading ");
     waitingForRecoveryCell2 = true;
}
  if(waitingForRecoveryCell2 && !sensor2disFault && !invalidreadingcell2 && lastInvalidCell2 )
  {
    Serial.print("[");
    Serial.print(millis());
    Serial.println(" ms] Cell 2 Reading Normal");
    waitingForRecoveryCell2 = false;
  }
    lastInvalidCell2 = invalidreadingcell2;


if (invalidreadingcell3 && !lastInvalidCell3)
{
    Serial.print("[");
    Serial.print(millis());
    Serial.println(" ms] Cell 3 Invalid Reading ");
     waitingForRecoveryCell3 = true;
}
  if(waitingForRecoveryCell3 && !sensor3disFault && !invalidreadingcell3 && lastInvalidCell3 )
  {
    Serial.print("[");
    Serial.print(millis());
    Serial.println(" ms] Cell 3 Reading Normal");
    waitingForRecoveryCell3 = false;
  }
    lastInvalidCell3 = invalidreadingcell3;

if (invalidreadingcell4 && !lastInvalidCell4)
{
    Serial.print("[");
    Serial.print(millis());
    Serial.println(" ms] Cell 4 Invalid Reading ");
     waitingForRecoveryCell4 = true;
}
  if(waitingForRecoveryCell4 && !sensor4disFault && !invalidreadingcell4 && lastInvalidCell4 )
  {
    Serial.print("[");
    Serial.print(millis());
    Serial.println(" ms] Cell 4 Reading Normal");
    waitingForRecoveryCell4 = false;
  }
    lastInvalidCell4 = invalidreadingcell4;

    // adc frozen 

if(adcfrozen != lastADCFrozen)
{
    Serial.print("[");
    Serial.print(millis());
    Serial.print(" ms] ");

    if(adcfrozen)
    {
        Serial.println("ADC Frozen");
    }
    else
    {
        Serial.println("ADC Recovered");
    }

    lastADCFrozen = adcfrozen;
}

}

// Buzzer Task starts for Task 2 
void buzzertask() {
  if (!buzzercontrolenable)
    return ;
  
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
  if (!ledcontrolenable)
    return;

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
  if (!relaycontrolenable)
    return ;
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
void serialtask(){
 if (!serialcontrolenable)
   return ;
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
  if (batterySensorFault) {
    Serial.println("Battery Sensor Fault Detected!");
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

 if (batterySensorFault)
   Serial.println("Battery Sensor Fault Detected!");

 if (sensor1disFault)
   Serial.println("Sensor 1 disconnected");

 if (sensor2disFault)
   Serial.println("Sensor 2 disconnected");

if (sensor3disFault)
   Serial.println("Sensor 3 disconnected");

 if (sensor4disFault)
   Serial.println("Sensor 4 disconnected");
   

 if (invalidreadingcell1)
   Serial.println("Invalid reading on Cell 1");
 if (invalidreadingcell2)
   Serial.println("Invalid reading on Cell 2");
 if (invalidreadingcell3)
   Serial.println("Invalid reading on Cell 3");
 if (invalidreadingcell4)
   Serial.println("Invalid reading on Cell 4");

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
  if(adcfrozen)
    lcd.print("ADC Frozen");

else if(relaymismatchfault)
    lcd.print("Relay Fault");

else if(batterySensorFault)
    lcd.print("Battery Sensor Fault");

else if(invalidreadingcell1 || invalidreadingcell2 ||
        invalidreadingcell3 || invalidreadingcell4)
    lcd.print("Invalid Read");

else if(watchdogfault)
    lcd.print("Watchdog");
  

  serialheartbeat = millis();
}}
 

// Screen Task code from here for Task 2
void Screentask() {

  screenheartbeat = millis();

  if(!screencontrolenable)
     return ;
   

   bool anyFault = weakCellFault || overVoltageFault || batterySensorFault || voltageSpikeFault;

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
   if(!lcdcontrolenable)
    return ;

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
      if(batterySensorFault)
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
