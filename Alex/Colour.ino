/* Alex's Colour Sensor */

void setupColoursensor() {
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(colourOut, INPUT);
  
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);
}

// take 5 readings and return average frequency of Alex's colour sensor
int avgFreq() {
 int reading = 0;
 int total = 0;
 for(int i = 0; i < 5; i++){
    reading = pulseIn(colourOut, LOW);    
    total += reading;
    delay(colourAverageDelay);
 }
 
 return total/5;
}

// Find the colour of the paper
// Determination of the exact colour will be handled on alex-pi.cpp
void getColour() {
  // RED
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  delay(colourSensorDelay);
  redFreq = avgFreq(); 
  delay(colourSensorDelay);
  
  // GREEN
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  delay(colourSensorDelay);
  greenFreq = avgFreq();
  delay(colourSensorDelay);

  // BLUE
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  delay(colourSensorDelay);
  blueFreq = avgFreq();
  delay(colourSensorDelay);

  /*
   Serial.print("Red: ");
   Serial.println(redFreq);
   Serial.print("Green: ");
   Serial.println(greenFreq);
   Serial.print("Blue: ");
   Serial.println(blueFreq);
   */
}

/*
  returns:
  0 - no colour detected / dummy
  1 - red
  2 - green
  else 
  -1 - error
*/
// TEST LAB CONDITIONS
// TODO: make use of the percentage difference of the colours 
#define THRESHOLD_DIST 10
int determineColour(){
  int redColour = redFreq;
  int greenColour = greenFreq;
  int blueColour = blueFreq;
  int distance = getDistance(); // take in colour value if distance is below 20 for accuracy
  if (greenColour < 180 && redColour < 180 && distance <= THRESHOLD_DIST) {
    //Serial.println("NOT DETECTED");
    return 0;
  }
  //if (distance <= THRESHOLD_DIST) { //red detected
    if (redColour > 240 && redColour < 300) {
      if (greenColour > 300) {
        if (blueColour > 200) {
          // redMusic();
          return 1;
        }
      }
    }
    if (redColour > 290) { //green detected
      if (greenColour < 300) {
        if (blueColour > 205) {
          // greenMusic();
          return 2;
        }
      }
    }
  //}
  /*if (greenColour > redColour && distance <= THRESHOLD_DIST) { //red detected
    //Serial.println("RED");
    return 1;
  }
  if (greenColour < redColour && distance <= THRESHOLD_DIST) { //green detected
    //Serial.println("GREEN");
    return 2;
  }*/
 return -1;
  
}


void sendColour()
{
  TPacket colourPacket;
  colourPacket.packetType = PACKET_TYPE_RESPONSE;
  colourPacket.command = RESP_COLOUR;
  
  colourPacket.params[0] = redFreq;
  colourPacket.params[1] = greenFreq;
  colourPacket.params[2] = blueFreq;
  colourPacket.params[3] = determineColour(); // 0, 1, 2, -1
  sendResponse(&colourPacket);
  // statusPacket.params[10] = getColour();

}

//TODO: music played when identifying the colour - dummy, red, green
