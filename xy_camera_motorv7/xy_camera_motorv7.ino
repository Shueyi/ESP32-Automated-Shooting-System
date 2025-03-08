#include <ESP32Servo.h>
#include <WiFi.h>

// Servo Objects
Servo xServo;
Servo yServo;
Servo triggerServo;

// Pin definitions
const int xServoPin = 18;
const int yServoPin = 19;
const int triggerServoPin = 21;
const int pirPin = 26;

// Servo control values
const int CW_SPEED = 100;
const int CCW_SPEED = 80;
const int STOP = 90;

// Motor specifications
const float UNITS_PER_SECOND_CW = 360.0 / 5.0;
const float UNITS_PER_SECOND_CCW = 360.0 / 6.0;
const float MS_PER_UNIT_CW = (1000.0 / UNITS_PER_SECOND_CW) + 4.85;
const float MS_PER_UNIT_CCW = (1000.0 / UNITS_PER_SECOND_CCW) - 2.5;

// UART pins for ESP32-CAM
#define TXD_PIN 17
#define RXD_PIN 16

// Target coordinates
const int targetX = 32;
const int targetY = 32;

// Y and X scanning angles
const int yAngles[] = {20,40};
const int xAngles[] = {90, 120, 105, 75, 60};

// Servo position trackers
int currentXUnit = 0;
int currentYUnit = 0;

// Function prototypes
void moveToXUnit(int targetUnit);
void moveToYAngle(int yValue);
void triggerMotor();
void moveServo(Servo& servo, int speed, int duration);
int calculateRotationTime(int units, bool clockwise);
void handlePIRMotion();
void scanYAndXAxis();
void clearSerialBuffer();

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Initializing ESP32...");

  Serial1.begin(9600, SERIAL_8N1, RXD_PIN, TXD_PIN);

  xServo.attach(xServoPin);
  yServo.attach(yServoPin);
  triggerServo.attach(triggerServoPin);

  xServo.write(STOP);
  yServo.write(0);
  triggerServo.write(0);

  pinMode(pirPin, INPUT);

  delay(1000);
  Serial.println("Setup complete.");
}

void loop() {
  if (digitalRead(pirPin) == HIGH) {
    Serial.println("PIR Sensor Triggered: Motion detected!");
    clearSerialBuffer();  // Clear any existing data in Serial1
    handlePIRMotion();
    scanYAndXAxis();  // Start scanning for Y-axis and X-axis angles
  } else {
    Serial.println("PIR Sensor Idle: No motion detected.");
  }
  delay(100);
}

// Function to scan Y-axis and X-axis for object detection
void scanYAndXAxis() {
  bool objectDetected = false;

  for (int i = 0; i < sizeof(yAngles) / sizeof(yAngles[0]); i++) {
    moveToYAngle(yAngles[i]);
    delay(3000);

    for (int j = 0; j < sizeof(xAngles) / sizeof(xAngles[0]); j++) {
      moveToXUnit(xAngles[j]);
      Serial.printf("[INFO] Scanning at Y-axis: %d degrees, X-axis: %d degrees\n", yAngles[i], xAngles[j]);
      delay(5000);

      if (Serial1.available()) {
        String receivedData = Serial1.readStringUntil('\n');
        Serial.printf("[INFO] Received raw data: %s\n", receivedData.c_str());

        int x = 0, y = 0;
        if (sscanf(receivedData.c_str(), "%d,%d", &x, &y) == 2) {
          Serial.printf("[INFO] Object detected at -> X: %d, Y: %d\n", x, y);
          objectDetected = true;

          while (x != targetX || y != targetY) {
            if (x != targetX) {
              if (x < targetX) {
                moveToXUnit(currentXUnit + 1);
                Serial.println("[INFO] Adjusting X-axis: Moving right");
              } else if (x > targetX) {
                moveToXUnit(currentXUnit - 1);
                Serial.println("[INFO] Adjusting X-axis: Moving left");
              }
              delay(500);  // Allow time for position update
              if (Serial1.available()) {
                receivedData = Serial1.readStringUntil('\n');
                if (sscanf(receivedData.c_str(), "%d,%d", &x, &y) == 2) {
                  Serial.printf("[INFO] X: %d, Y: %d (After X adjustment)\n", x, y);
                }
              }
            }

            if (x == targetX && y != targetY) {
              if (y < targetY) {
                moveToYAngle(currentYUnit - 1);
                Serial.println("[INFO] Adjusting Y-axis: Moving up");
              } else if (y > targetY) {
                moveToYAngle(currentYUnit + 1);
                Serial.println("[INFO] Adjusting Y-axis: Moving down");
              }
              delay(500);  // Allow time for position update
              if (Serial1.available()) {
                receivedData = Serial1.readStringUntil('\n');
                if (sscanf(receivedData.c_str(), "%d,%d", &x, &y) == 2) {
                  Serial.printf("[INFO] X: %d, Y: %d (After Y adjustment)\n", x, y);
                }
              }
            }
          }

          if (x == targetX && y == targetY) {
            Serial.println("[SUCCESS] Exact target coordinates reached!");
            delay(200);
            triggerMotor();
            delay(2000);
            moveToYAngle(0);
            delay(500);
            moveToXUnit(0);
            return;  // Exit the scanning function
          }
        }
      }
    }

    if (objectDetected) break;
  }

  if (!objectDetected) {
    Serial.println("[INFO] No object detected after scanning all angles.");
  }

  moveToYAngle(0);
  moveToXUnit(0);
}


void clearSerialBuffer() {
  while (Serial1.available()) {
    Serial1.read();
  }
  Serial.println("[INFO] Serial1 buffer cleared.");
}

void handlePIRMotion() {
  Serial.println("[INFO] Handling PIR-triggered motion...");
  moveToXUnit(100);
  delay(2000);
  moveToXUnit(currentXUnit);
  Serial.println("[INFO] PIR-triggered motion handled.");
}

void moveToXUnit(int targetUnit) {
  if (targetUnit < 0 || targetUnit > 360) {
    Serial.println("[ERROR] X-axis target out of range (0-360).");
    return;
  }

  int unitsToMove = abs(targetUnit - currentXUnit);
  bool clockwise = (targetUnit > currentXUnit);
  int rotationTime = calculateRotationTime(unitsToMove, clockwise);

  Serial.printf("[INFO] Moving X-axis from %d to %d units.\n", currentXUnit, targetUnit);
  moveServo(xServo, clockwise ? CW_SPEED : CCW_SPEED, rotationTime);

  xServo.write(STOP);
  currentXUnit = targetUnit;
  Serial.println("[INFO] X-axis movement complete.");
}

void moveToYAngle(int yValue) {
  if ( yValue > 60) {
    Serial.println("[ERROR] Y-axis target out of range (0-50).");
    return;
  }

  Serial.printf("[INFO] Moving Y-axis to %d degrees.\n", yValue);
  yServo.write(yValue);
  currentYUnit = yValue;
  delay(500);
  Serial.println("[INFO] Y-axis movement complete.");
}

void triggerMotor() {
  const int maxAngle = 18;
  const int delayAfterMove = 600;

  Serial.println("[INFO] Triggering motor...");
  triggerServo.write(maxAngle);
  delay(delayAfterMove);

  triggerServo.write(0);
  delay(delayAfterMove);
  Serial.println("[INFO] Motor reset to initial position.");
}

void moveServo(Servo& servo, int speed, int duration) {
  servo.write(speed);
  delay(duration);
  servo.write(STOP);
}

int calculateRotationTime(int units, bool clockwise) {
  return clockwise ? (int)(units * MS_PER_UNIT_CW) : (int)(units * MS_PER_UNIT_CCW);
}