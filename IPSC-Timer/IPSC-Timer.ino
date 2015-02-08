#include <Wire.h>
#include <Button.h> // Jack Christensens Button library https://github.com/JChristensen/Button
#include <MenuSystem.h>
#include <MicroLCD.h>
#include <EEPROM.h>

// Nano SDA = A4
// Nano SCL = A5

LCD_SSD1306 lcd; /* for SSD1306 OLED module */

// For buttons
#define PULLUP true
#define INVERT true
#define DEBOUNCE_MS 20
#define LONG_PRESS 1000

Button StartButton(3, PULLUP, INVERT, DEBOUNCE_MS);
Button UpButton(5, PULLUP, INVERT, DEBOUNCE_MS);
Button DownButton(4, PULLUP, INVERT, DEBOUNCE_MS);

// Menu variables
MenuSystem ms;
Menu mm("Settings");
Menu mu1("Start Delay");
MenuItem mu1_mi1("On (3 sec.)");
MenuItem mu1_mi2("Off");
MenuItem mu1_mi3("..");
Menu mu2("Echo-Cancel");
MenuItem mu2_mi1("+10 ms");
MenuItem mu2_mi2("-10 ms");
MenuItem mu2_mi3("Calibrate");
MenuItem mu2_mi4("..");
Menu mu3("Buzzer");
MenuItem mu3_mi1("On");
MenuItem mu3_mi2("Off");
MenuItem mu3_mi3("..");
Menu mu4("Default Settings");
MenuItem mu4_mi1("Reset device");
MenuItem mu4_mi2("..");

// this constant won't change:
const int DetectorPin = 2;
const int BuzzerPin = 8;

// Variables will change:
int ShotCounter = 0;
int DetectorState = 0;
int LastDetectorState = 1;
int TimerState = 0;
int XPos = 0;
int DelayedStart = EEPROM.read(1);
int BuzzerEnabled = EEPROM.read(2);
int DebounceDelay = EEPROM.read(3);    //"Deaf-time" in ms after registered shot
int DelayedStartTime = 3000;

int interval = 700;
int intervalState = 1;

int State = 0;

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // The last time a shot was detected
long StartTime = 0;

long currentMillis;
long previousMillis;

float LatestShotTime = 0;
float FirstShotTime = 0;
float PrevShotTime = 0;
float SplitShotTime = 0;
float BestSplitShotTime = 0;

void setup() {
  // initialize the detector pin as a input:
  pinMode(DetectorPin, INPUT);
  pinMode(BuzzerPin, OUTPUT);

  // initialize serial communication:
  //Serial.begin(9600);

  // Menu setup
  mm.add_menu(&mu1);
  mu1.add_item(&mu1_mi1, &on_item3_selected);
  mu1.add_item(&mu1_mi2, &on_item4_selected);
  mu1.add_item(&mu1_mi3, &on_item5_selected); //..
  mm.add_menu(&mu2);
  mu2.add_item(&mu2_mi1, &on_item6_selected);
  mu2.add_item(&mu2_mi2, &on_item7_selected);
  mu2.add_item(&mu2_mi3, &on_item8_selected);
  mu2.add_item(&mu2_mi4, &on_item9_selected); //..
  mm.add_menu(&mu3);
  mu3.add_item(&mu3_mi1, &on_item10_selected);
  mu3.add_item(&mu3_mi2, &on_item11_selected);
  mu3.add_item(&mu3_mi3, &on_item5_selected); //..
  mm.add_menu(&mu4);
  mu4.add_item(&mu4_mi1, &on_item12_selected);
  mu4.add_item(&mu4_mi2, &on_item5_selected); //..
  ms.set_root_menu(&mm);

  lcd.begin();
  lcd.clear();

  DisplayReady1() ;
  //State=2;
  //delay(1000);
}

void loop() {
  DetectorState = digitalRead(DetectorPin);

  StartButton.read();
  UpButton.read();
  DownButton.read();

  switch (State) {
    case 0:  //Ready state
      currentMillis = millis();
      if (currentMillis - previousMillis > interval) {
        previousMillis = currentMillis;
        if (intervalState == LOW) {
          intervalState = HIGH;
          DisplayReady2();
        }
        else {
          intervalState = LOW;
          DisplayReady1();
        }
      }
      if (StartButton.wasReleased()) {
        State = 5;
        StartTimer();
      }
      else if (StartButton.pressedFor(LONG_PRESS)) {
        State = 1;
        displayMenu();
      }
      break;

    case 1: //intermediate state before Setup in order to detect button release after switching state
      if (StartButton.wasReleased())
        State = 2;
      break;

    case 2: //Setup
      if (UpButton.wasPressed()) {
        ms.prev();
        displayMenu();
      }
      if (DownButton.wasPressed()) {
        ms.next();
        displayMenu();
      }
      if (StartButton.wasReleased()) {
        ms.select();
        lcd.clear();
        displayMenu();
      }
      if (StartButton.pressedFor(LONG_PRESS)) {
        State = 3;
        lcd.clear();
        DisplayReady1();
      }
      break;

    case 3: //intermediate state before Ready in order to detect button release after switching state
      if (StartButton.wasReleased())
        State = 0;
      break;

    case 5: //Timer running!
      DetectShots();
      if (StartButton.pressedFor(LONG_PRESS)) {
        State = 6;
        lcd.clear();
//        ResetTimer();
        DisplayReview();
      }
      if (StartButton.wasReleased()) {
        //lcd.backlight();
        }
      break;
    
   case 6: //intermediate state before Review in order to detect button release after switching state
      if (StartButton.wasReleased())
        State = 7;
      break;
      
   case 7: //Review
     if (StartButton.pressedFor(LONG_PRESS)) {
        State = 3;
        lcd.clear();
        ResetTimer();
        DisplayReady1();
      }
     break;

  } //End of case switch-machine

  LastDetectorState = DetectorState;
}

void DetectShots() {
  if (DetectorState != LastDetectorState) {
    if (millis() > lastDebounceTime) {
      if (DetectorState == HIGH) {
        lastDebounceTime = (millis() + DebounceDelay);

        LatestShotTime = (float((millis() - StartTime)) / 1000);
        ShotCounter++;

        if (ShotCounter == 1) { //Only on first shot
          FirstShotTime = LatestShotTime;
        }

        if (ShotCounter > 1) {
          SplitShotTime = (LatestShotTime - PrevShotTime);
          if (ShotCounter == 2) {
            BestSplitShotTime = SplitShotTime;
          }
          if ((SplitShotTime) < (BestSplitShotTime)) {
            BestSplitShotTime = SplitShotTime;
          }
        }

        DisplayTimer();
        PrevShotTime = LatestShotTime;
      }
    }
  }
}

void DetectCalibrationShots() {
  if (DetectorState != LastDetectorState) {
    if (DetectorState == HIGH) {
      lastDebounceTime = (millis());
    }
    if (DetectorState == LOW) {
      DebounceDelay = ((millis() - lastDebounceTime) + 10);
      EEPROM.write(3, (DebounceDelay));
      ShotCounter++;
    }
  }

}

void StartTimer() {
  if (DelayedStart == true) {
    lcd.clear();
    DisplayStandby();
    delay (DelayedStartTime);
  }
  lcd.clear();
  DisplayTimer();
  lastDebounceTime = (millis()) + 750; //To make sure buzzer doesn't trigger at shot
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

void ClearEEPROM() {
  for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);
}

void SetDefaults() {
  EEPROM.write(1, 1);
  EEPROM.write(2, 1);
  EEPROM.write(3, 10);
}

void Beep() {
  if (BuzzerEnabled == true) {
    digitalWrite(BuzzerPin, HIGH);
    delay(700);
    digitalWrite(BuzzerPin, LOW);
  }
}

///////////////////////////////////////////////////// Display cunctions

void DisplayTimer() {
  lcd.clear();
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.setCursor(0, 0);
  lcd.println((FirstShotTime), 2);

  if (ShotCounter < 10) {
    XPos = 58;
  }
  else {
    XPos = 51;
  }
  lcd.setCursor(XPos, 0);
  lcd.println(ShotCounter);

  lcd.setCursor(85, 0);
  lcd.println((BestSplitShotTime), 2);

  if (LatestShotTime < 10) {
    XPos = 43;
  }
  else if (LatestShotTime < 100) {
    XPos = 37;
  }
  else {
    XPos = 34;
  }
  lcd.setCursor(XPos, 4);
  lcd.clearLine (4);
  lcd.println((LatestShotTime), 2);
}

void DisplayReview() {
  lcd.clear();
  lcd.setFontSize(FONT_SIZE_SMALL);
  lcd.setCursor(40, 0);
  lcd.println("Review");
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.setCursor(0, 1);
  lcd.println((FirstShotTime), 2);

  if (ShotCounter < 10) {
    XPos = 58;
  }
  else {
    XPos = 51;
  }
  lcd.setCursor(XPos, 1);
  lcd.println(ShotCounter);

  lcd.setCursor(85, 1);
  lcd.println((BestSplitShotTime), 2);

  if (LatestShotTime < 10) {
    XPos = 43;
  }
  else if (LatestShotTime < 100) {
    XPos = 37;
  }
  else {
    XPos = 34;
  }
  lcd.setCursor(XPos, 4);
  lcd.clearLine (4);
  lcd.println((LatestShotTime), 2);
}

void DisplayReady1() {
  lcd.setCursor(34, 3);
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.print("READY...");
}

void DisplayReady2() {
  lcd.setCursor(34, 3);
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.print("READY    ");
}

void DisplayStandby() {
  lcd.setCursor(10, 3);
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.print("Stand By...");
}

void DisplayCalibration() {
  lcd.setCursor(10, 3);
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.print("Fire a round");
}

void displayMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.setFontSize(FONT_SIZE_SMALL);
  Menu const* cp_menu = ms.get_current_menu();

  lcd.println(cp_menu->get_name());
  MenuComponent const* cp_menu_sel = cp_menu->get_selected();
  for (int i = 0; i < cp_menu->get_num_menu_components(); ++i)
  {
    MenuComponent const* cp_m_comp = cp_menu->get_menu_component(i);
    if (cp_menu_sel == cp_m_comp) {
      lcd.print("> ");
    }
    else {
      lcd.print("  ");
    }
    lcd.print(cp_m_comp->get_name());

    lcd.println("");
  }
}

///////////////////////////////////////////////////// Menu callback functions
void on_item3_selected(MenuItem * p_menu_item) {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Delay On");
  DelayedStart = true;
  EEPROM.write(1, (DelayedStart));
  delay(1500);
}

void on_item4_selected(MenuItem * p_menu_item) {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Delay Off");
  DelayedStart = false;
  EEPROM.write(1, (DelayedStart));
  delay(1500);
}

void on_item5_selected(MenuItem * p_menu_item) {
  ms.back();
  displayMenu();
}

void on_item6_selected(MenuItem * p_menu_item) {
  displayMenu();
}

void on_item7_selected(MenuItem * p_menu_item) {
  displayMenu();
}
void on_item8_selected(MenuItem * p_menu_item) {
  lcd.clear();
  DisplayCalibration();
  do {
    DetectorState = digitalRead(DetectorPin);
    DetectCalibrationShots();
    LastDetectorState = DetectorState;
  }
  while ((ShotCounter) < 2);
  ShotCounter = 0;
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print (DebounceDelay);
  lcd.print (" ms");
  delay(1000);
}

void on_item9_selected(MenuItem * p_menu_item) {
  ms.back();
  displayMenu();
}

void on_item10_selected(MenuItem * p_menu_item) {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Buzzer On");
  BuzzerEnabled = true;
  EEPROM.write(2, (BuzzerEnabled));
  delay(1500);
}

void on_item11_selected(MenuItem * p_menu_item) {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Buzzer Off");
  BuzzerEnabled = false;
  EEPROM.write(2, (BuzzerEnabled));
  delay(1500);
}

void on_item12_selected(MenuItem * p_menu_item) {
  ClearEEPROM();
  SetDefaults();
}
