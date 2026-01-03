/*
  Rat Operated Vehicle code for Elegoo Smart Robot Car v4
  the original Rat Car: https://www.instructables.com/Rat-Operated-Vehicle/ - hardware is all this
  used elegoo code for talking to pins
  inspiration from https://github.com/LElizabethCrawford/RatCarCode/blob/master/Rat_Car_Code.ino
  lighting stolen from Adafruit's strandtest

  hacked up by mwl@mwl.io, who claims zero credit, but full blame for running the blender
  also over-commented by mwl because he is easily confused and C++ is different enough from Perl that he has zero business here
*/

//undef when this thing works
int DEBUG = 1;
int DEBUG_TOUCH = 1;
int DEBUG_MOVESTATE = 1;
int DEBUG_TARGET = 1;
int DEBUG_SPEED = 1;
int DEBUG_STEER = 1;

//configure the motors/
#define PIN_Motor_STBY 3
#define PIN_Motor_SpeedR 5
#define PIN_Motor_SpeedL 6
#define PIN_Motor_DirectionR 7
#define PIN_Motor_DirectionL 8

// Define the movement states
#define STOP 0
#define FORWARD 1
#define VEER_LEFT 2
#define PIVOT_LEFT 3
#define VEER_RIGHT 4
#define PIVOT_RIGHT 5

#define direction_forward true
#define direction_back false

//configure the touch sensors
const byte forwardInputPin = A0;     // the forward touch sensor - black
const byte leftInputPin = A1;        // the left touch sensor - white
const byte rightInputPin = A2;       // the right touch sensor - red

int forwardInputVal = 0;
int leftInputVal = 0;
int rightInputVal = 0;

int threshold = 500; //minimum ADC reading for touch sensor

//values for movement

int MAX_LEFT = 150; // Use an even number. -- base speed
int MAX_RIGHT = 150; // Use an even number.

int REVERSE_MAX_LEFT = -100; // maximum negative speed, for rotation
int REVERSE_MAX_RIGHT = -100; // maximum negative speed, for rotation

int CURRENT_LEFT = 0; // speed set the last time we went this way
int CURRENT_RIGHT = 0;

int TARGET_LEFT = 0; // speed we're setting this time
int TARGET_RIGHT = 0;

int TURN_OFFSET = 50;  // Adjust how sharp it turns.
// 50 makes for just a slight turn.  Much more, and the differences between left
// and right motors makes one turn direction sharper than the other.

int ACCELERATION = 10;  // 1 feels laggy, 50 feels lurchy.

//do we want to set the wheels to go forward?
bool LEFT_FORWARD = true;
bool RIGHT_FORWARD = true;

int moveState = STOP; // 0:stop, 1: forward, 2: veer left, 3: rotate left, 4: veer right, 5: rotate right.
int oldMoveState = STOP;

void setup()
{
  // put your setup code here, to run once:

  Serial.begin(9600);

  pinMode(PIN_Motor_SpeedR, OUTPUT);
  pinMode(PIN_Motor_SpeedL, OUTPUT);
  pinMode(PIN_Motor_DirectionR, OUTPUT);
  pinMode(PIN_Motor_DirectionL, OUTPUT);
  pinMode(PIN_Motor_STBY, OUTPUT);

}

//ratcar functions

//read the touch sensors
int checkInputs()
{
  leftInputVal = analogRead(leftInputPin);
  delay(10);
  leftInputVal = analogRead(leftInputPin);

  forwardInputVal = analogRead(forwardInputPin);
  delay(10);
  forwardInputVal = analogRead(forwardInputPin);

  rightInputVal = analogRead(rightInputPin);
  delay(10);
  rightInputVal = analogRead(rightInputPin);

  if ( DEBUG_TOUCH ) {
    if (forwardInputVal) {
      Serial.print("forward: ");
      Serial.println(forwardInputVal);
    }
    if (leftInputVal) {
      Serial.print("left: ");
      Serial.println(leftInputVal);
    }
    if (rightInputVal) {
      Serial.print("right: ");
      Serial.println(rightInputVal);
    }
  }

  delay(500);

  //use touch sensor results to set movement direction
  // moveState 0:stop, 1: forward, 2: veer left, 3: rotate left, 4: veer right, 5: rotate right.
  if (leftInputVal > threshold && forwardInputVal > threshold) { //press left and forward simultaneously
    moveState = VEER_LEFT;
  } else if (rightInputVal > threshold && forwardInputVal > threshold) { //press right and forward simultaneously
    moveState = VEER_RIGHT;
  } else if (leftInputVal > threshold) { //press left only
    moveState = PIVOT_LEFT;
  } else if (rightInputVal > threshold) { //press right only
    moveState = PIVOT_RIGHT;
  } else if (forwardInputVal > threshold) { //press forward only
    moveState = FORWARD;
  } else {
    moveState = STOP;  // Touching none is stop.
  }

  if ( DEBUG_MOVESTATE) {
    if (moveState != oldMoveState ) {
      Serial.print("moveState: ");
      Serial.println(moveState);
    }
  }
  return (moveState);
}

void setTarget(int) // Set target speed for the wheels.
{
  // 0:stop, 1: forward, 2: veer left, 3: rotate left, 4: veer right, 5: rotate right.

  switch (moveState) {
    case STOP:
      TARGET_LEFT = 0;
      TARGET_RIGHT = 0;
      if (DEBUG_TARGET) Serial.println ("Stop");
      break;

    case FORWARD:
      TARGET_LEFT = MAX_LEFT;
      TARGET_RIGHT = MAX_RIGHT;
      if (DEBUG_TARGET) Serial.println ("Forward");
      break;

    case VEER_LEFT:
      TARGET_LEFT = MAX_LEFT - TURN_OFFSET;
      TARGET_RIGHT = MAX_RIGHT;
      if (DEBUG_TARGET) Serial.println ("Veer Left");
      break;

    case PIVOT_LEFT:
      TARGET_LEFT = REVERSE_MAX_LEFT;
      TARGET_RIGHT = MAX_RIGHT;
      if (DEBUG_TARGET) Serial.println ("Pivot Left");
      break;

    case VEER_RIGHT:
      TARGET_LEFT = MAX_LEFT;
      TARGET_RIGHT = MAX_RIGHT - TURN_OFFSET;
      if (DEBUG_TARGET) Serial.println ("Veer Right");
      break;

    case PIVOT_RIGHT:
      TARGET_LEFT = MAX_LEFT;
      TARGET_RIGHT = REVERSE_MAX_RIGHT;
      if (DEBUG_TARGET) Serial.println ("Pivot Right");
      break;
  }

  if (DEBUG_TARGET) {
    Serial.print ("target_left: ");
    Serial.print (TARGET_LEFT);
    Serial.print (" target_right: ");
    Serial.println (TARGET_RIGHT);
  }
}

void stop()
{
  moveState = STOP; 
}

void figureWheelSpeed() // calculate next wheel speed, and call steer()
{
  //gradually shift, don't shake the rat

  //right wheel slower than desired
  if (CURRENT_RIGHT < TARGET_RIGHT) {
    CURRENT_RIGHT += ACCELERATION;
    if (DEBUG_SPEED) {
      Serial.print ("TARGET_RIGHT: ");
      Serial.print (TARGET_RIGHT);
      Serial.print (" CURRENT_RIGHT: ");
      Serial.print (CURRENT_RIGHT);
    }
  }

  //right wheel faster than desired
  if (CURRENT_RIGHT > TARGET_RIGHT) {
    CURRENT_RIGHT -= ACCELERATION;
    if (DEBUG_SPEED) {
      Serial.print ("TARGET_RIGHT: ");
      Serial.print (TARGET_RIGHT);
      Serial.print (" CURRENT_RIGHT: ");
      Serial.print (CURRENT_RIGHT);
    }
  }

  //left wheel slower than desired
  if (CURRENT_LEFT < TARGET_LEFT) {
    CURRENT_LEFT += ACCELERATION;
    if (DEBUG_SPEED) {
      Serial.print (" TARGET_LEFT: ");
      Serial.print (TARGET_LEFT);
      Serial.print (" CURRENT_LEFT: ");
      Serial.print (CURRENT_LEFT);
    }
  }

  //left wheel faster than desired
  if (CURRENT_LEFT > TARGET_LEFT) {
    CURRENT_LEFT -= ACCELERATION;
    if (DEBUG_SPEED) {
      Serial.print (" TARGET_LEFT: ");
      Serial.print (TARGET_LEFT);
      Serial.print (" CURRENT_LEFT: ");
      Serial.print (CURRENT_LEFT);
    }
  }

  if (DEBUG_SPEED) {
    if ((CURRENT_LEFT) | (CURRENT_RIGHT)) {
      Serial.println (" ");
    }
  }
  // If necessary, switch the left wheels direction.
  /*  if (CURRENT_LEFT < 0 && LEFT_FORWARD) {
      LEFT_FORWARD = false; // set left wheels backwards
      if (DEBUG_SPEED) Serial.println ("Reversing Left Wheels");
    }
    if (CURRENT_LEFT > 0 && !LEFT_FORWARD) {
      LEFT_FORWARD = true; // set left wheels forwards
      if (DEBUG_SPEED) Serial.println ("Forwarding Left Wheels");
    }

    // If necessary, switch the right wheels direction.
    if (CURRENT_RIGHT < 0 && RIGHT_FORWARD) {
      RIGHT_FORWARD = false; //set right wheels backwards
      if (DEBUG_SPEED) Serial.println ("Reversing Right Wheels");
    }
    if (CURRENT_RIGHT > 0 && !RIGHT_FORWARD) {
      RIGHT_FORWARD = true; // set right wheels forward
      if (DEBUG_SPEED) Serial.println ("Forwarding Right Wheels");
    } */

  //  steer (RIGHT_FORWARD, CURRENT_RIGHT, LEFT_FORWARD, CURRENT_LEFT);
  steer (CURRENT_RIGHT, CURRENT_LEFT);

  delay(2);  // enough delay from sensors.
}



//STEERING
//raw motor control
/*void steer(boolean direction_Right_Motor, uint8_t speedR, //RIGHT motor
           boolean direction_Left_Motor, uint8_t speedL //LEFT motor
          )*/
void steer(int speedR, //RIGHT motor
           int speedL //LEFT motor
          )
{
  {
    digitalWrite(PIN_Motor_STBY, HIGH);

    { //Right Motor
      if (DEBUG_SPEED) {
        Serial.print ("Set R to ");
        Serial.print (speedR);
      }
      if (speedR > 0) {
        digitalWrite(PIN_Motor_DirectionR, HIGH);
        analogWrite(PIN_Motor_SpeedR, speedR);
      }
      if (speedR < 0) {
        speedR = abs(speedR);
        digitalWrite(PIN_Motor_DirectionR, LOW);
        analogWrite(PIN_Motor_SpeedR, speedR);
      }
      if (speedR == 0) {
        analogWrite(PIN_Motor_SpeedR, 0);
        digitalWrite(PIN_Motor_STBY, LOW);
      }
    }

    { //Left Motor
      {
        if (DEBUG_SPEED) {
          Serial.print (" Set L to ");
          Serial.println (speedL);
        if (speedL > 0) {
          digitalWrite(PIN_Motor_DirectionL, HIGH);
          analogWrite(PIN_Motor_SpeedL, speedL);
        }

        if (speedL < 0) {
          speedL = abs(speedL);
          digitalWrite(PIN_Motor_DirectionL, LOW);
          analogWrite(PIN_Motor_SpeedL, speedL);
        }
        if (speedL == 0) {
          analogWrite(PIN_Motor_SpeedL, 0);
          digitalWrite(PIN_Motor_STBY, LOW);
        }
        }
      }
    }
  }
}


int idx = 0;

// after all this, the main program doesn't do much...
void loop()
{
  Serial.print("LOOP: ");
  Serial.println(idx);
  if (idx > 2) {
    stop();
    while (true) {}
  }
  //put your main code here, to run repeatedly :

  //right direction, speed, left direction, speed
  //  steer(0, 100, 1, 100);

  moveState = checkInputs();
  if (oldMoveState != moveState)
    setTarget(moveState);

  figureWheelSpeed();

  oldMoveState = moveState;
  idx++;
  Serial.println();
}

