/*
  Rat Operated Vehicle code for Elegoo Smart Robot Car v4
  the original Rat Car: https://www.instructables.com/Rat-Operated-Vehicle/ - hardware is all this
  used elegoo code for talking to pins
  inspiration from https://github.com/LElizabethCrawford/RatCarCode/blob/master/Rat_Car_Code.ino
  lighting stolen from Adafruit's strandtest

  hacked up by mwl@mwl.io, who claims zero credit, but full blame for running the blender
  also over-commented by mwl because he is easily confused and C++ is different enough from Perl that he has zero business here
*/

const bool DEBUG = true;
#define DBG(x) do { if (DEBUG) { x; } } while (0)

//configure the motors/
#define PIN_Motor_STBY 3
#define PIN_Motor_SpeedR 5
#define PIN_Motor_SpeedL 6
#define PIN_Motor_DirectionR 7
#define PIN_Motor_DirectionL 8

// Define the movement states
enum MoveState {
  STOP,
  FORWARD,
  VEER_LEFT,
  PIVOT_LEFT,
  VEER_RIGHT,
  PIVOT_RIGHT
};

#define direction_forward true
#define direction_back false

// Constants

const byte forwardInputPin = A0;     // the forward touch sensor - black
const byte leftInputPin = A1;        // the left touch sensor - white
const byte rightInputPin = A2;       // the right touch sensor - red

const int ADC_THRESHOLD = 200; // minimum ADC reading for touch sensor (experimentally seeing > 1000)

const int MAX_LEFT = 100;  // Use an even number. -- base speed
const int MAX_RIGHT = 100; // Use an even number.

const int REVERSE_MAX_LEFT = -50;  // maximum negative speed, for rotation
const int REVERSE_MAX_RIGHT = -50; // maximum negative speed, for rotation

const int TURN_OFFSET = 50;  // Adjust how sharp it turns.
// 50 makes for just a slight turn.  Much more, and the differences between left
// and right motors makes one turn direction sharper than the other.

const int ACCELERATION = 10;  // 1 feels laggy, 50 feels lurchy.

// Loop-local variable initialization

// Initialize ADC readings
int forwardInputVal = 0;
int leftInputVal = 0;
int rightInputVal = 0;

// "current" speed
int CURRENT_LEFT = 0; 
int CURRENT_RIGHT = 0;

// "target" speed after acceleration
int TARGET_LEFT = 0;
int TARGET_RIGHT = 0;

// Initial state
MoveState moveState = STOP;
MoveState oldMoveState = STOP;

// Setup code runs once
void setup() {

  Serial.begin(9600);

  pinMode(PIN_Motor_SpeedR, OUTPUT);
  pinMode(PIN_Motor_SpeedL, OUTPUT);
  pinMode(PIN_Motor_DirectionR, OUTPUT);
  pinMode(PIN_Motor_DirectionL, OUTPUT);
  pinMode(PIN_Motor_STBY, OUTPUT);

}

// debug helpers

void labeled(char str[], int val) {
  Serial.print(str);
  Serial.println(val);
}

void printMoveState(int ms) {
  switch(ms) {
    case STOP: 
      Serial.println("STOP");
      break;
    case FORWARD: 
      Serial.println("FORWARD");
      break;
    case VEER_LEFT: 
      Serial.println("VEER LEFT");
      break;
    case PIVOT_LEFT: 
      Serial.println("PIVOT LEFT");
      break;
    case VEER_RIGHT: 
      Serial.println("VEER RIGHT");
      break;
    case PIVOT_RIGHT: 
      Serial.println("PIVOT RIGHT");
      break;
  }
}

// read an analog sensor (throw away first value; 
//   may be unstable due to ADC mux switching)
int readTouch(byte pin) {
  int v = analogRead(pin);
  delay(10);
  return analogRead(pin);
}

// read the touch sensors
int checkInputs() {
  leftInputVal    = readTouch(leftInputPin);
  forwardInputVal = readTouch(forwardInputPin);
  rightInputVal   = readTouch(rightInputPin);

  DBG({
    labeled("LEFT INPUT", leftInputVal);
    labeled("RIGHT INPUT", rightInputVal);
    labeled("FORWARD INPUT", forwardInputVal);
  });

  delay(500);

  //use touch sensor results to set movement direction
  bool left = leftInputVal > ADC_THRESHOLD;
  bool right = rightInputVal > ADC_THRESHOLD;
  bool forward = forwardInputVal > ADC_THRESHOLD;
  if (forward && left) moveState = VEER_LEFT;
  else if (forward && right) moveState = VEER_RIGHT;
  else if (left) moveState = PIVOT_LEFT;
  else if (right) moveState = PIVOT_RIGHT;
  else if (forward) moveState = FORWARD;
  else moveState = STOP;
  
  DBG(printMoveState(moveState));
  return (moveState);
}

void setTarget() // Set target speed for the wheels.
{

  DBG({
    Serial.print("New state:");
    printMoveState(moveState);
  });

  switch (moveState) {
    case STOP:
      TARGET_LEFT = 0;
      TARGET_RIGHT = 0;
      break;

    case FORWARD:
      TARGET_LEFT = MAX_LEFT;
      TARGET_RIGHT = MAX_RIGHT;
      break;

    case VEER_LEFT:
      TARGET_LEFT = MAX_LEFT - TURN_OFFSET;
      TARGET_RIGHT = MAX_RIGHT;
      break;

    case PIVOT_LEFT:
      TARGET_LEFT = REVERSE_MAX_LEFT;
      TARGET_RIGHT = MAX_RIGHT;
      break;

    case VEER_RIGHT:
      TARGET_LEFT = MAX_LEFT;
      TARGET_RIGHT = MAX_RIGHT - TURN_OFFSET;
      break;

    case PIVOT_RIGHT:
      TARGET_LEFT = MAX_LEFT;
      TARGET_RIGHT = REVERSE_MAX_RIGHT;
      break;
  }

  DBG({
    labeled("target left:", TARGET_LEFT);
    labeled("target right:", TARGET_RIGHT);
  });
}

void stop()
{
  moveState = STOP; 
}

int accelerate(int curr, int target) {
  if (curr < target) return min(curr + ACCELERATION, target);
  if (curr > target) return max(curr - ACCELERATION, target);
  return curr;
}

void figureWheelSpeed() // calculate next wheel speed, and call steer()
{
  // gradually shift, don't shake the rat

  // right wheel slower than desired
  /*
  DBG({
    labeled("TARGET_RIGHT:", TARGET_RIGHT);
    labeled("CURRENT_RIGHT:", CURRENT_RIGHT);
  });
  */
  CURRENT_RIGHT = accelerate(CURRENT_RIGHT, TARGET_RIGHT);
  // left wheel slower than desired
  /*
  DBG({
    labeled("TARGET_LEFT:", TARGET_LEFT);
    labeled("CURRENT_LEFT:", CURRENT_LEFT);
  });
  */
  CURRENT_LEFT = accelerate(CURRENT_LEFT, TARGET_LEFT);

  steer (CURRENT_RIGHT, CURRENT_LEFT);

  delay(10);  // delay for sensors
}

void driveMotor(int speed, int pinSpeed, int pinDir) {
  if (speed == 0) {
    analogWrite(pinSpeed, 0);
    digitalWrite(PIN_Motor_STBY, LOW);
    return;
  }

  digitalWrite(pinDir, speed > 0 ? HIGH : LOW);
  analogWrite(pinSpeed, abs(speed));

}

// Update motor speeds
void steer(int speedR, int speedL) {
  digitalWrite(PIN_Motor_STBY, HIGH);
  driveMotor(speedR, PIN_Motor_SpeedR, PIN_Motor_DirectionR);
  driveMotor(speedL, PIN_Motor_SpeedL, PIN_Motor_DirectionL);
}

// after all this, the main program doesn't do much...
// iteratively update state

int idx = 0;
void loop()
{

  Serial.print("LOOP ");
  Serial.println(idx);
  //if (idx > 3) {
  //  while (true) {}
  //}

  moveState = checkInputs();
  if (oldMoveState != moveState)
    setTarget();

  figureWheelSpeed();

  oldMoveState = moveState;
  DBG(Serial.println());
  delay(1000);
  idx++;
}

