#include <SoftwareSerial.h>

SoftwareSerial nanoSerial(12, 11); // RX, TX
int pinNum = 0;
int freq = 3000;

void setup() {
  //sensor pin
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(4,OUTPUT);
  pinMode(5,OUTPUT);
  pinMode(6,OUTPUT);
  pinMode(7,OUTPUT);
  pinMode(8,OUTPUT);
  pinMode(9,OUTPUT);
  digitalWrite(2,HIGH);
  digitalWrite(3,HIGH);
  digitalWrite(4,HIGH);
  digitalWrite(5,HIGH);
  digitalWrite(6,HIGH);
  digitalWrite(7,HIGH);
  digitalWrite(8,HIGH);
  digitalWrite(9,HIGH);
 
  //transmit pin
  pinMode(11,OUTPUT);
  pinMode(12,INPUT);
  Serial.begin(9600);
  nanoSerial.begin(115200);
  
}

void loop() {
  Serial.print(analogRead(A0)); Serial.print(",");
  Serial.print(analogRead(A1)); Serial.print(",");
  Serial.print(analogRead(A2)); Serial.print(",");
  Serial.print(analogRead(A3)); Serial.print(",");
  Serial.print(analogRead(A4)); Serial.print(",");
  Serial.print(analogRead(A5)); Serial.print(",");
  Serial.print(analogRead(A6)); Serial.print(",");
  Serial.print(analogRead(A7)); Serial.print("\n");

  while(nanoSerial.available()>0)
    pinNum = nanoSerial.parseInt();
  if(pinNum>=1 && pinNum<=8){
    switchDPinValue(pinNum);
    pinNum = 0;
  }

  nanoSerial.print(analogRead(A0)); nanoSerial.print(",");
  nanoSerial.print(analogRead(A1)); nanoSerial.print(",");
  nanoSerial.print(analogRead(A2)); nanoSerial.print(",");
  nanoSerial.print(analogRead(A3)); nanoSerial.print(",");
  nanoSerial.print(analogRead(A4)); nanoSerial.print(",");
  nanoSerial.print(analogRead(A5)); nanoSerial.print(",");
  nanoSerial.print(analogRead(A6)); nanoSerial.print(",");
  nanoSerial.print(analogRead(A7)); nanoSerial.print("\n");
  
  delay(freq);
}

void switchDPinValue(int pin){
  pin+=1;
  if(digitalRead(pin) == HIGH)
    digitalWrite(pin, LOW);
  else
    digitalWrite(pin, HIGH);
}
