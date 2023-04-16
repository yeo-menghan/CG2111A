#include <serialize.h>
#include <stdarg.h>
#include "packet.h"
#include "constants.h"
#include <math.h>

typedef enum {
  STOP = 0,
  FORWARD = 1,
  BACKWARD = 2,
  LEFT = 3,
  RIGHT = 4
} 
TDirection;

volatile TDirection dir = STOP;

/*
   Alex's configuration constants
 */
#define PI 3.141592654
#define ALEX_LENGTH 19.5 // measure exact ltr
#define ALEX_BREADTH 12.5 // measure exact ltr

// measures Alex's diagonal and circumference of turn
float AlexDiagonal = 18.0; // TODO
float AlexCirc = 56.66; // TODO

// Number of ticks per revolution from the
// wheel encoder

#define COUNTS_PER_REV      180

// Wheel circumference in cm.
// We will use this to calculate forward/reverse distance traveled
// by taking revs * WHEEL_CIRC

#define WHEEL_CIRC          20.42

// Motor control pins. You need to adjust these till
// Alex moves in the correct direction
#define LF                  11   // Left forward pin
#define LR                  10   // Left reverse pin
#define RF                  5  // Right forward pin
#define RR                  6  // Right reverse pin

/* Alex's Ultrasonic Sensor */
#define echo A1
#define trigger A0
volatile unsigned long duration; // Variable to store time taken for the pulse to return to the ultrasonic
volatile unsigned long distance; // Variable to store distance after calculation

/* Alex's Colour sensor */
#define S0 7
#define S1 4                                      
#define S2 9
#define S3 12
#define colourOut 13
unsigned long redFreq;
unsigned long greenFreq;
unsigned long blueFreq;
#define colourSensorDelay 50 // ms
#define colourAverageDelay 20 // ms

/*
      Alex's State Variables
 */

// Store the ticks from Alex's left and
// right encoders.
volatile unsigned long leftForwardTicks;
volatile unsigned long rightForwardTicks;
volatile unsigned long leftReverseTicks;
volatile unsigned long rightReverseTicks;

// Left and right encoder ticks for turning
volatile unsigned long leftForwardTicksTurns;
volatile unsigned long rightForwardTicksTurns;
volatile unsigned long leftReverseTicksTurns;
volatile unsigned long rightReverseTicksTurns;

// Store the revolutions on Alex's left
// and right wheels
volatile unsigned long leftRevs;
volatile unsigned long rightRevs;

// Forward and reverse distance traveled
volatile unsigned long forwardDist;
volatile unsigned long reverseDist;


// variables to keep track of whether we have moved a commanded distancefor
unsigned long deltaDist;
unsigned long newDist;

// variables to keep track of our turning angle
unsigned long deltaTicks;
unsigned long targetTicks;


void setup() {
  // put your setup code here, to run once:
  AlexDiagonal = sqrt((ALEX_LENGTH * ALEX_LENGTH) + (ALEX_BREADTH * ALEX_BREADTH));
  AlexCirc = PI * AlexDiagonal;
  cli();
  setupEINT();
  setupSerial();
  startSerial();
  setupMotors();
  startMotors();
  setupUltrasonic();
  setupColoursensor();
  setupMusic();
  enablePullups();
  initializeState();
  sei();
}

void handleCommand(TPacket *command)
{
  switch (command->command)
  {
    // For movement commands, param[0] = distance, param[1] = speed.
  case COMMAND_FORWARD:
    sendOK();
    forward((float) command->params[0], (float) command->params[1]);
    break;

  case COMMAND_REVERSE:
    sendOK();
    reverse((float) command->params[0], (float) command->params[1]);
    break;

  case COMMAND_TURN_LEFT:
    sendOK();
    left((float) command->params[0], (float) command->params[1]);
    break;

  case COMMAND_TURN_RIGHT:
    sendOK();
    right((float) command->params[0], (float) command->params[1]);
    break;

  case COMMAND_STOP:
    sendOK();
    stop();
    break;

  case COMMAND_GET_STATS:
    sendStatus();
    sendOK();
    break;
    
  case COMMAND_GET_COLOUR:
    getColour();
    determineColour();
    sendColour();
    sendOK();
    break;

  case COMMAND_DIST:
    sendDistance();
    break;
    
  case COMMAND_PLAY_MUSIC:
    playMusic(command->params[0]);
    sendOK();
    break;
    
  case COMMAND_CLEAR_STATS:
    sendOK();
    clearOneCounter(command->params[0]); // Alex-pi specifies which counter to clear in params[0] of the command packet
    break;

  default:
    sendBadCommand();
  }
}

void handlePacket(TPacket *packet)
{
  switch (packet->packetType)
  {
  case PACKET_TYPE_COMMAND:
    handleCommand(packet);
    break;

  case PACKET_TYPE_RESPONSE:
    break;

  case PACKET_TYPE_ERROR:
    break;

  case PACKET_TYPE_MESSAGE:
    break;

  case PACKET_TYPE_HELLO:
    break;
  }
}

#define STOP_DELAY 100 // 100ms delay to prevent bad packet errors
void loop() {
  // getColour();
  // getDistance();

  TPacket recvPacket; // This holds commands from the Pi

  TResult result = readPacket(&recvPacket);


  if (result == PACKET_OK)
    handlePacket(&recvPacket);
  else if (result == PACKET_BAD)
  {
    sendBadPacket();
  }
  else if (result == PACKET_CHECKSUM_BAD)
  {
    sendBadChecksum();
  }

  if (deltaDist > 0) {
    if (dir == FORWARD) {
      if (forwardDist > newDist) {
        deltaDist = 0;
        newDist = 0;
        stop();
        delay(STOP_DELAY);
        // TODO: send distance
      }
    }
    else if (dir == BACKWARD) {
      if (reverseDist > newDist) {
        deltaDist = 0;
        newDist = 0;
        stop();
        delay(STOP_DELAY);
        // TODO: send distance
      }

    }
    else if (dir == STOP) {
      deltaDist = 0;
      newDist = 0;
      stop();
      delay(STOP_DELAY);
    }
  }

  if (deltaTicks > 0) {
    if (dir == LEFT) {
      if (leftReverseTicksTurns >= targetTicks || rightForwardTicksTurns >= targetTicks) {
        deltaTicks = 0;
        targetTicks = 0;
        stop();
        delay(STOP_DELAY);
        // TODO: send distance
      }
    }
    else if (dir == RIGHT) {
      if (rightReverseTicksTurns >= targetTicks || leftForwardTicksTurns >= targetTicks) {
        deltaTicks = 0;
        targetTicks = 0;
        stop();
        delay(STOP_DELAY);
        // TODO: send distance
      }
    }
    else if (dir == STOP) {
      deltaTicks = 0;
      targetTicks = 0;
      stop();
      delay(STOP_DELAY);
    }
  }
}

