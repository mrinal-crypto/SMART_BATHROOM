#include <Servo.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo myServo;

#define WATER_LEVEL_FOR_USE 8 //in cm

#define TRIG_PIN 9
#define ECHO_PIN 10
#define BUZ_ALERT 11
#define SERVO 6
#define DOOR_OPEN_SWITCH 7
#define DOOR_CLOSE_SWITCH 8

int screenInterval = 1000;
int messageIndex = 0;

long duration = 0;
unsigned int distance = 0;
unsigned int afterDoorClosedWaterDistance = 0;

unsigned long DistancePrevMillis = 0;
unsigned long msgPrevMillis = 0;
const unsigned int distanceMeasureInterval = 100;
int percentage = 0;
String percentageString = "";
bool isDoorClosed = false;
bool hasRunOnce = false;

void setup() {
  Serial.begin(9600);
  pinMode(DOOR_OPEN_SWITCH, INPUT);
  pinMode(DOOR_CLOSE_SWITCH, INPUT);
  pinMode(BUZ_ALERT, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  myServo.attach(SERVO);
  rotateServoSmoothly(90, 20); //rotate servo to home position

  lcd.init();
  lcd.backlight();

  welcomeMsg();
}
////////////////////////////////////////////////////////////////////
void welcomeMsg() {
  lcd.clear();
  printMsg(2, 0, "P P G I T I");
  printMsg(1, 1, "Smart Bathroom");
  delay(5000);
}

/////////////////////////////////////////////////////////////////////
void printMsg(uint8_t x, uint8_t y, String msg) {
  lcd.setCursor(x, y);
  lcd.print(msg);
}
/////////////////////////////////////////////////////////////////////
void buzzerAlert() {
  digitalWrite(BUZ_ALERT, HIGH);
  delay(200);
  digitalWrite(BUZ_ALERT, LOW);
  delay(50);
  digitalWrite(BUZ_ALERT, HIGH);
  delay(200);
  digitalWrite(BUZ_ALERT, LOW);
}
////////////////////////////////////////////////////////////////////////
void rotateServoSmoothly(int targetAngle, int stepDelay) {
  int currentAngle = myServo.read();
  int increment = (targetAngle > currentAngle) ? 1 : -1;

  while (currentAngle != targetAngle) {
    currentAngle += increment;
    myServo.write(currentAngle);
    delay(stepDelay);
  }
}
///////////////////////////////////////////////////////////////////////
void waterUsedPercentage(int value) {
  if (value >= afterDoorClosedWaterDistance) {
    percentage = (100 / WATER_LEVEL_FOR_USE) * (value - afterDoorClosedWaterDistance);
    //    percentage = map(value, afterDoorClosedWaterDistance, (afterDoorClosedWaterDistance + WATER_LEVEL_FOR_USE), 0, 100);
    percentageString = String(percentage) + "%";
  }
}
////////////////////////////////////////////////////////////////////////
void printDistance(int value) {
  Serial.print("Distance: ");
  Serial.print(value);
  Serial.println(" cm");
}
/////////////////////////////////////////////////////////////////////
void doorControl(unsigned int value) {

  if (digitalRead(DOOR_CLOSE_SWITCH) == HIGH) {
    rotateServoSmoothly(0, 20);
    isDoorClosed = true;
  }

  if (value <= (afterDoorClosedWaterDistance + WATER_LEVEL_FOR_USE) && digitalRead(DOOR_OPEN_SWITCH) == HIGH && isDoorClosed == true) {
    isDoorClosed = true;
    Serial.println("not enough water is given, please use more water");
    rotateServoSmoothly(0, 20);
    buzzerAlert();
    delay(1000);
  }

  if (value > (afterDoorClosedWaterDistance + WATER_LEVEL_FOR_USE) && digitalRead(DOOR_OPEN_SWITCH) == HIGH) {
    isDoorClosed = false;
    hasRunOnce = false;
    afterDoorClosedWaterDistance = 0;
    rotateServoSmoothly(90, 20);
    Serial.println("servo roate zero degree");
    Serial.println("ok! you give water properly. your door is opened now");
    delay(1000);
  }

  if (digitalRead(DOOR_OPEN_SWITCH) == HIGH && digitalRead(DOOR_CLOSE_SWITCH) == HIGH) {
    isDoorClosed = false;
    hasRunOnce = false;
    rotateServoSmoothly(90, 20);
    Serial.println("emergency exit");
    printMsg(0, 0, "                ");
    printMsg(0, 1, "                ");
    printMsg(0, 0, "EMERGENCY EXIT");
    delay(8000);
  }
}
///////////////////////////////////////////////////////////////////////
void displayMsg() {
  if (isDoorClosed == false) {
    unsigned long currentMillis = millis();
    if (currentMillis - msgPrevMillis >= screenInterval) {
      msgPrevMillis = currentMillis;
      messageIndex++;
      if (messageIndex == 1) {
        printMsg(0, 0, "                ");
        printMsg(0, 1, "                ");
        printMsg(0, 0, "KEEP TOILETS ");
        printMsg(0, 1, "CLEAN & FLUSH..");
      }
      if (messageIndex == 2) {
        printMsg(0, 0, "                ");
        printMsg(0, 1, "                ");
        printMsg(0, 0, "AFTER USE");
        messageIndex = 0;
      }
    }
  }
  
  if (isDoorClosed == true && percentage < 100 && digitalRead(DOOR_OPEN_SWITCH) == LOW) {
    unsigned long currentMillis = millis();
    if (currentMillis - msgPrevMillis >= screenInterval) {
      msgPrevMillis = currentMillis;
      messageIndex++;
      if (messageIndex == 1) {
        printMsg(0, 0, "                ");
        printMsg(0, 0, "DOOR CLOSED");
      }
      if (messageIndex == 2) {
        printMsg(0, 0, "                ");
        printMsg(0, 0, "WATERED PROPERLY");
        messageIndex = 0;
      }
    }
    printMsg(0, 1, "WATER USED ");
    printMsg(11, 1, "     ");
    printMsg(11, 1, percentageString);
  }

  if (isDoorClosed == true && percentage < 100 && digitalRead(DOOR_OPEN_SWITCH) == HIGH) {

    printMsg(0, 0, "                ");
    printMsg(0, 0, "USE MORE WATER");
    printMsg(0, 1, "WATER USED ");
    printMsg(11, 1, "     ");
    printMsg(11, 1, percentageString);
  }

  if (isDoorClosed == true && percentage >= 100) {
    unsigned long currentMillis = millis();
    if (currentMillis - msgPrevMillis >= screenInterval) {
      msgPrevMillis = currentMillis;
      messageIndex++;
      if (messageIndex == 1) {
        printMsg(0, 0, "                ");
        printMsg(0, 0, "YOU MAY GO NOW");
      }
      if (messageIndex == 2) {
        printMsg(0, 0, "                ");
        printMsg(0, 0, "THANK YOU");
        messageIndex = 0;
      }
    }
    printMsg(0, 1, "WATER USED ");
    printMsg(11, 1, "     ");
    printMsg(11, 1, percentageString);
  }
}
/////////////////////////////////////////////////////////////////////
void measureWaterLevelEverytime() {

  uint8_t totalSamples = 10;
  unsigned int sumOfTotalSamples = 0;
  for (uint8_t samples = 1; samples <= totalSamples; samples++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    duration = pulseIn(ECHO_PIN, HIGH);
    int dis = duration * 0.034 / 2;
    sumOfTotalSamples += dis;
  }

  distance = sumOfTotalSamples / totalSamples;

  //    digitalWrite(TRIG_PIN, LOW);
  //    delayMicroseconds(2);
  //    digitalWrite(TRIG_PIN, HIGH);
  //    delayMicroseconds(10);
  //    digitalWrite(TRIG_PIN, LOW);
  //
  //    duration = pulseIn(ECHO_PIN, HIGH);
  //    distance = duration * 0.034 / 2;  // Calculate distance in centimeters

  printDistance(distance);

}
////////////////////////////////////////////////////////////////////////
void afterDoorClosedWaterLevel() {

  if (!hasRunOnce) {
    uint8_t totalSamples = 20;
    unsigned int sumOfTotalSamples = 0;

    for (uint8_t samples = 1; samples <= totalSamples; samples++) {
      digitalWrite(TRIG_PIN, LOW);
      delayMicroseconds(2);
      digitalWrite(TRIG_PIN, HIGH);
      delayMicroseconds(10);
      digitalWrite(TRIG_PIN, LOW);

      duration = pulseIn(ECHO_PIN, HIGH);
      distance = duration * 0.034 / 2;
      sumOfTotalSamples += distance;
    }

    afterDoorClosedWaterDistance = sumOfTotalSamples / totalSamples;
    Serial.print("now the water level is (cm) = ");
    Serial.println(afterDoorClosedWaterDistance);
    hasRunOnce = true;
  }
}
////////////////////////////////////////////////////////////////////////
void actionAfterDoorClosed() {
  if (isDoorClosed == true) {
    afterDoorClosedWaterLevel();
    measureWaterLevelEverytime();
    waterUsedPercentage(distance);
  }
}
////////////////////////////////////////////////////////////////////////
void loop() {
  displayMsg();
  actionAfterDoorClosed();
  doorControl(distance);
}
