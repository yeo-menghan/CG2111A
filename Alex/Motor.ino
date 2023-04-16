/*
   Alex's motor drivers.

*/

// Set up Alex's motors. Right now this is empty, but
// later you will replace it with code to set up the PWMs
// to drive the motors.
void setupMotors()
{
  /* Our motor set up is:
        A1IN - Pin 5, PD5, OC0B
        A2IN - Pin 6, PD6, OC0A
        B1IN - Pin 10, PB2, OC1B
        B2In - pIN 11, PB3, OC2A
  */
}

// Start the PWM for Alex's motors.
// We will implement this later. For now it is
// blank.
void startMotors()
{

}

// Convert percentages to PWM values
int pwmVal(float speed)
{
  if (speed < 0.0)
    speed = 0;

  if (speed > 100.0)
    speed = 100.0;

  return (int) ((speed / 100.0) * 255.0);
}

void forward(float dist, float speed)
{
  if (dist == 0)
    deltaDist = 99999;
  else
    deltaDist = dist;

  newDist = forwardDist + deltaDist;

  dir = FORWARD;
  int val = pwmVal(speed);
    analogWrite(LF, val*0.95);
    analogWrite(RF, val);
    analogWrite(LR, 0);
    analogWrite(RR, 0);
  
}

void reverse(float dist, float speed)
{

  if (dist == 0)
    deltaDist = 99999;
  else
    deltaDist = dist;

  newDist = reverseDist + deltaDist;


  dir = BACKWARD;
  int val = pwmVal(speed);

  analogWrite(LR, val*0.95);
  analogWrite(RR, val);
  analogWrite(LF, 0);
  analogWrite(RF, 0);
}

unsigned long computeDeltaTicks(float ang, int counts)
{

  unsigned long ticks = (unsigned long) ((ang * AlexCirc * counts) / (360.0 * WHEEL_CIRC));
  return ticks;
}


void left(float ang, float speed)
{
  dir = LEFT;
  int val = pwmVal(speed);
  if (ang == 0) {
    deltaTicks = 9999999;
  }
  else {
    deltaTicks = computeDeltaTicks(ang, COUNTS_PER_REV);
  }
  targetTicks = leftReverseTicksTurns + deltaTicks;

  analogWrite(LR, val);
  analogWrite(RF, val);
  analogWrite(LF, 0);
  analogWrite(RR, 0);
}

void right(float ang, float speed)
{
  dir = RIGHT;
  int val = pwmVal(speed);
  if (ang == 0) {
    deltaTicks = 9999999;
  }
  else {
    deltaTicks = computeDeltaTicks(ang, COUNTS_PER_REV);
  }
  targetTicks = rightReverseTicksTurns + deltaTicks;
  
  analogWrite(RR, val);
  analogWrite(LF, val);
  analogWrite(LR, 0);
  analogWrite(RF, 0);
}

// Stop Alex. To replace with bare-metal code later.
void stop()
{
  dir = STOP;
  analogWrite(LF, 0);
  analogWrite(LR, 0);
  analogWrite(RF, 0);
  analogWrite(RR, 0);
}
