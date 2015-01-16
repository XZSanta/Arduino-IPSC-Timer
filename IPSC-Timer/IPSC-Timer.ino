#include <SPI.h>
#include <Wire.h>
#include <Button.h> // Jack Christensens Button library https://github.com/JChristensen/Button
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Nano SDA = A4
// Nano SCL = A5

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// For buttons
#define PULLUP true
#define INVERT true
#define DEBOUNCE_MS 20
#define LONG_PRESS 1000

Button StartButton(3, PULLUP, INVERT, DEBOUNCE_MS);
Button UpButton(5, PULLUP, INVERT, DEBOUNCE_MS);
Button DownButton(4, PULLUP, INVERT, DEBOUNCE_MS);

// this constant won't change:
const int DetectorPin = 2;
const int BuzzerPin = 8;

// Variables will change:
int ShotCounter = 0;
int DetectorState = 0;
int LastDetectorState = 1;
int TimerState = 0;
int XPos = 0;
int DelayedStart = false;
int DelayedStartTime = 3000;
int State=0;

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // The last time a shot was detected
long debounceDelay = 10;    //"Deaf-time" in ms after registered shot
long StartTime = 0;
float LatestShotTime = 0;
float FirstShotTime = 0;
float PrevShotTime = 0;
float SplitShotTime = 0;
float BestSplitShotTime = 0;

void setup() {
  // initialize the fetector pin as a input:
  pinMode(DetectorPin, INPUT);
  pinMode(BuzzerPin, OUTPUT);

  // initialize serial communication:
  //Serial.begin(9600);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  UpdateDisplay ();
}

void loop() {
  DetectorState = digitalRead(DetectorPin);
  
  StartButton.read();
  UpButton.read();
  DownButton.read();
 
  switch (State) {
    case 0:  //Ready state   
      if (StartButton.wasReleased())
        State = 4;
      else if (StartButton.pressedFor(LONG_PRESS))
        State = 1;
      break;
      
    case 1: //intermediate state in order to detect button release after switching state 
      Serial.println("To Setup");
      if (StartButton.wasReleased())
        State = 2;
      break;
      
    case 2: //Setup
      if (StartButton.wasReleased())
        Serial.println("Setup Function");
      else if (StartButton.pressedFor(LONG_PRESS))
        State = 3;
        break;
      
    case 3: //intermediate state in order to detect button release after switching state 
      Serial.println("To Ready");
      if (StartButton.wasReleased())
        State = 0;
      break;
      
    case 4:
      State = 5;
      break;
      
     case 5: //Timer running!
      if (StartButton.wasReleased())
        Serial.println("Running");
      else if (StartButton.pressedFor(LONG_PRESS))
        State = 3;
      break;
    } //End of switch-machine 

if (State == 0){
}
if (State == 1){
  UpdateDisplaySetup();
}
if (State == 2){
}
if (State == 3){
  ResetTimer();
  UpdateDisplay ();
}
if (State == 4){
  StartTimer();
}
if (State == 5){
  DetectShots();
  //UpdateDisplay ();
}

LastDetectorState = DetectorState;
}

void DetectShots(){
  
    if (DetectorState != LastDetectorState){
     if (millis() > lastDebounceTime) {
    if (DetectorState == HIGH) {
      lastDebounceTime = (millis()+debounceDelay);
      
      LatestShotTime = (float((millis()-StartTime))/1000);
      ShotCounter++;
      
      if (ShotCounter == 1) { //Only on first shot
        FirstShotTime = LatestShotTime;
      }
      
      
      
      if (ShotCounter > 1){
        SplitShotTime = (LatestShotTime - PrevShotTime);
        if (ShotCounter == 2){
           BestSplitShotTime = SplitShotTime;       
        }
        if ((SplitShotTime) < (BestSplitShotTime)) {
          BestSplitShotTime = SplitShotTime;
        }
      }
      
      UpdateDisplay();
      PrevShotTime = LatestShotTime;
    }
   }
  }

  
}

void StartTimer() {
  if (DelayedStart == true){
    delay (DelayedStartTime);
  }
  StartTime = millis();
  Beep();
}

void ResetTimer() {
  ShotCounter = 0;
  LatestShotTime = 0;
  FirstShotTime = 0;
  SplitShotTime = 0;
  BestSplitShotTime = 0;
}

void UpdateDisplaySetup() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(35,0);
  display.println("SETUP");
  display.display();
}

void UpdateDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0,0);
  display.println((FirstShotTime),2);
  
  display.setTextColor(WHITE);
  if (ShotCounter < 10){
    XPos = 58;
  }
  else {
    XPos = 51;
  } 
  display.setCursor(XPos,0);
  display.println(ShotCounter);
  
  display.setTextColor(BLACK, WHITE);
  display.setCursor(80,0);
  display.println((BestSplitShotTime),2);
  
  display.setTextSize(3);
  display.setTextColor(WHITE);
    if (LatestShotTime < 10){
      XPos = 28;
    }
    else if (LatestShotTime < 100){
      XPos = 18;
    }
    else {
      XPos = 8;
    }
  display.setCursor(XPos,35);
  display.println((LatestShotTime),2);
  display.display();
}

void Beep() {
  digitalWrite(BuzzerPin, HIGH);
  delay(700);
  digitalWrite(BuzzerPin, LOW);
}
