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

// this constant won't change:
const int DetectorPin = 2;
//const int BuzzerPin = 8; //
const int BuzzerPin = 13; //Quiet night-mode for testing while the family is asleep

// Variables will change:
int ShotCounter = 0;
int DetectorState = 0;
int LastDetectorState = 1;
int TimerState = 0;
int XPos = 0;
int DelayedStart = EEPROM.read(1);
int DelayedStartTime = 3000;

int interval = 700;
int intervalState = 1;

int State = 0;

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // The last time a shot was detected
long debounceDelay = 10;    //"Deaf-time" in ms after registered shot
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
  ms.set_root_menu(&mm);

  lcd.begin();
  lcd.clear();
  DisplayTimer() ;
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
        if (intervalState == LOW){
          intervalState = HIGH;
          lcd.clear();
        }
        else {
          intervalState = LOW;
          DisplayReady();
        }
      }
      if (StartButton.wasReleased()) {
        State = 5;
        StartTimer();
      }
      else if (StartButton.pressedFor(LONG_PRESS)) {
        State = 1;
        lcd.clear();
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
        DisplayReady();
      }
      break;

    case 3: //intermediate state before Ready in order to detect button release after switching state
      if (StartButton.wasReleased())
        State = 0;
      break;

      //    case 4: //intermediate state before Running in order to detect button release after switching state
      //      StartTimer();
      //      State = 5;
      //      break;

    case 5: //Timer running!
      DetectShots();
      if (StartButton.pressedFor(LONG_PRESS)) {
        State = 3;
        lcd.clear();
        ResetTimer();
        DisplayReady();
      }
      break;
    case 6: //Calibrate
      DetectCalibrateShots();
      break;

  } //End of switch-machine

  LastDetectorState = DetectorState;
}

void DetectShots() {
  if (DetectorState != LastDetectorState) {
    if (millis() > lastDebounceTime) {
      if (DetectorState == HIGH) {
        lastDebounceTime = (millis() + debounceDelay);

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

void DetectCalibrateShots() {
  //  if (DetectorState != LastDetectorState) {
  //    if (DetectorState == HIGH) {
  //        lastDebounceTime = (millis();
  //    }
  //    if (DetectorState == LOW) {
  //        debounceDelay = ((millis()-lastDebounceTime)+10);
  //    }
  //  }
  //
}

void StartTimer() {
  if (DelayedStart == true) {
    lcd.clear();
    DisplayStandby();
    delay (DelayedStartTime);
  }
  StartTime = millis();
  Beep();
  lcd.clear();
  DisplayTimer();
}

void ResetTimer() {
  ShotCounter = 0;
  LatestShotTime = 0;
  FirstShotTime = 0;
  SplitShotTime = 0;
  BestSplitShotTime = 0;
}


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
  //lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.setCursor(XPos, 4);
  lcd.clearLine (4);
  lcd.println((LatestShotTime), 2);
  //  lcd.printLong((LatestShotTime)* 100); //Test of number only font
}

void ClearEEPROM() {
  for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);
}

void Beep() {
  digitalWrite(BuzzerPin, HIGH);
  delay(700);
  digitalWrite(BuzzerPin, LOW);
}

void DisplayReady() {
  lcd.setCursor(34, 3);
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.print("READY...");
}

void DisplayStandby() {
  lcd.setCursor(10, 3);
  lcd.setFontSize(FONT_SIZE_MEDIUM);
  lcd.print("Stand By...");
}

void displayMenu() {
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

// Menu callback function
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

}

void on_item9_selected(MenuItem * p_menu_item) {
  ms.back();
  displayMenu();
}
