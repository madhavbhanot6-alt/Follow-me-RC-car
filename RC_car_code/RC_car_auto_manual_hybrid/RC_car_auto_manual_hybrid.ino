#include <Servo.h>

// ---------------- MOTOR PINS ----------------
int IN1 = 2;   // Left motor
int IN2 = 3;
int IN3 = 4;   // Right motor
int IN4 = 5;

int ENA = 6;   // PWM Left motor
int ENB = 11;  // PWM Right motor

// ---------------- SERVO + ULTRASONIC ----------------
#define TRIG 9
#define ECHO 10
#define SERVO_PIN 12

Servo head;

// ---------------- SPEED VALUES ----------------
int manualSpeed = 255;
int followSpeed = 130;
int turnSpeed   = 120;

// ---------------- FOLLOW DISTANCE RANGE ----------------
int minDist = 20;   // too close → back up
int maxDist = 70;   // too far → stop following

// ---------------- MODES ----------------
bool manualMode = true;
char command;

// ---------------- IDLE SCANNING ----------------
unsigned long lastSeenTime = 0;


// =========================================================
//                     ULTRASONIC
// =========================================================
long getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 20000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}


// =========================================================
//                     MOTOR FUNCTIONS
// =========================================================
void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward(int spd) {
  analogWrite(ENA, spd);
  analogWrite(ENB, spd);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward(int spd) {
  analogWrite(ENA, spd);
  analogWrite(ENB, spd);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void leftTurn(int spd) {
  analogWrite(ENA, 0);
  analogWrite(ENB, spd);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void rightTurn(int spd) {
  analogWrite(ENA, spd);
  analogWrite(ENB, 0);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}


// =========================================================
//                     IDLE LOOK AROUND
// =========================================================
void idleScan() {
  head.write(50); delay(250);
  head.write(90); delay(250);
  head.write(130); delay(250);
  head.write(90); delay(250);
}


// =========================================================
//                         SETUP
// =========================================================
void setup() {
  Serial.begin(9600);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  head.attach(SERVO_PIN);
  head.write(90);
  delay(300);

  stopMotors();
}


// =========================================================
//                         LOOP
// =========================================================
void loop() {

  // ---------------- BLUETOOTH COMMANDS ----------------
  if (Serial.available()) {
    command = toupper(Serial.read());

    if (command == 'M') {
      manualMode = true;
      stopMotors();
      Serial.println("MODE: MANUAL");
    }
    else if (command == 'A') {
      manualMode = false;
      stopMotors();
      Serial.println("MODE: AUTO FOLLOW-ME");
    }

    // ---------------- MANUAL MODE ----------------
    if (manualMode) {
      switch (command) {
        case 'F': forward(manualSpeed); break;
        case 'B': backward(manualSpeed); break;
        case 'L': leftTurn(manualSpeed); break;
        case 'R': rightTurn(manualSpeed); break;
        case 'S': stopMotors(); break;
      }
      return;  // Skip auto mode section
    }
  }


  // ==================================================
  //                AUTO FOLLOW-ME MODE
  // ==================================================
  int angles[3] = {60, 90, 120};
  long dist[3];

  for (int i = 0; i < 3; i++) {
    head.write(angles[i]);
    delay(150);
    dist[i] = getDistance();
  }

  // Find direction with shortest distance (closest target)
  int best = 0;
  if (dist[1] < dist[best]) best = 1;
  if (dist[2] < dist[best]) best = 2;

  long D = dist[best];

  // ---------------- PERSON NOT FOUND ----------------
  if (D > maxDist) {
    stopMotors();
    idleScan();
    return;
  }

  // ---------------- PERSON FOUND ----------------
  lastSeenTime = millis();

  // ---------------- TOO CLOSE → BACK UP ----------------
  if (D < minDist) {
    backward(followSpeed);
    delay(150);
    stopMotors();
    return;
  }

  // ---------------- TURN TOWARD PERSON ----------------
  if (best == 0) {
    leftTurn(turnSpeed);
    delay(120);
  }
  else if (best == 2) {
    rightTurn(turnSpeed);
    delay(120);
  }

  // ---------------- MOVE FORWARD ----------------
  forward(followSpeed);
}