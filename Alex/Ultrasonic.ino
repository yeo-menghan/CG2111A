/* Alex's Ultrasonic Sensor */
void setupUltrasonic() {
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);
}

uint32_t getDistance()
{
  // clears trigger pin
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);

  unsigned long duration = pulseIn(echo, HIGH);
  unsigned long distance = duration * 344 / 20000;
  // Serial.print(distance);
  // Serial.println(" cm");
  return distance;
}

void sendDistance(){
 
 uint32_t ultrasonicDist = getDistance();
 
 TPacket distancePacket;
 distancePacket.packetType = PACKET_TYPE_RESPONSE;
 distancePacket.command = RESP_DIST;
 distancePacket.params[0] = ultrasonicDist;
 sendResponse(&distancePacket);
 
 sendOK();
}
