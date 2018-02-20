/*
 *  Arduino micro code for HACKberry.
 *  Origially created by exiii Inc.
 *  edited by Genta Kondo on 2017/6/11
 */
#include <Servo.h>
#include <HbButton.h>

//Settings
const char SerialNum[] = "";
const boolean isRight = ;//right:1, left:0
const int boardversion = ;//mk1:1, mk2:2
const int outThumbMax = ;//right:open, left:close
const int outIndexMax = ;//right:open, left:close
const int outOtherMax = ;//right:open, left:close
const int outThumbMin = ;//right:close, left:open
const int outIndexMin = ;//right:close, left:open
const int outOtherMin = ;//right:close, left:open
const int speedMax = ;
const int speedMin = ;
const int speedReverse = ;
const int thSpeedReverse = ;//0-100
const int thSpeedZero = ;//0-100
const boolean onSerial = ;

//Hardware
/// object generation
Servo servoIndex; //index finger
Servo servoOther; //other three fingers
Servo servoThumb; //thumb
HbButton buttonCalib; //calibration
HbButton buttonGrasp; //grasp mode
HbButton buttonThumb; //open/close thumb
HbButton buttonOther; //lock/unlock other three fingers
/// pin configuration
int pinCalib; //start calibration
int pinGrasp; //change grasp mode
int pinThumb; //open/close thumb
int pinOther; //lock/unlock other three fingers
int pinSensor; //sensor input
int pinServoIndex; //index servo pin
int pinServoOTher; //other servo pin
int pinServoThumb; //thumb servo pin

//Software
boolean isThumbOpen = 1;
boolean isOtherLock = 0;
boolean isGrasp = 1;
int swCount0,swCount1,swCount2,swCount3 = 0;
int sensorValue = 0; // value read from the sensor
int sensorMax = 700;
int sensorMin = 0;
int speed = 0;
int position = 0;
const int positionMax = 100;
const int positionMin = 0;
int prePosition = 0;
int outThumb,outIndex,outOther = 90;
int outThumbOpen,outThumbClose,outIndexOpen,outIndexClose,outOtherOpen,outOtherClose;

void setup() {
  Serial.begin(9600);
  switch (boardversion) {
    case 1: // mk1
          if(isRight){
            pinCalib =  A6;
            pinGrasp =  A5;
            pinThumb =  A4;
            pinOther =  A3;
            pinServoIndex = 3;
            pinServoOther = 5;
            pinServoThumb = 6;
            pinSensor = A0;
            outThumbOpen=outThumbMax; outThumbClose=outThumbMin;
            outIndexOpen=outIndexMax; outIndexClose=outIndexMin;
            outOtherOpen=outOtherMax; outOtherClose=outOtherMin;
          }
          else{
            pinCalib =  11;
            pinGrasp =  10;
            pinThumb =  8;
            pinOther =  7;
            pinServoIndex = 3;
            pinServoOther = 5;
            pinServoThumb = 6;
            pinSensor = A0;
            outThumbOpen=outThumbMin; outThumbClose=outThumbMax;
            outIndexOpen=outIndexMin; outIndexClose=outIndexMax;
            outOtherOpen=outOtherMin; outOtherClose=outOtherMax;
          }
          buttonCalib.attach(pinCalib,true,true);
          buttonGrasp.attach(pinGraps,true,true);
          buttonThumb.attach(pinThumb,true,true);
          buttonOther.attach(pinOther,true,true);
          break;
    case 2: // mk2
        pinCalib = A6;
        pinGrasp = A7;
        pinThumb = A0;
        pinOther = 10;
        pinServoIndex = 5;
        pinServoOther = 6;
        pinServoThumb = 9;
        pinSensor = A1;
        if(isRight) {
            outThumbOpen=outThumbMax; outThumbClose=outThumbMin;
            outIndexOpen=outIndexMax; outIndexClose=outIndexMin;
            outOtherOpen=outOtherMax; outOtherClose=outOtherMin;
        } else {
            outThumbOpen=outThumbMin; outThumbClose=outThumbMax;
            outIndexOpen=outIndexMin; outIndexClose=outIndexMax;
            outOtherOpen=outOtherMin; outOtherClose=outOtherMax;
        }
        buttonCalib.attach(pinCalib,false,true);
        buttonGrasp.attach(pinGraps,false,true);
        buttonThumb.attach(pinThumb,false,true);
        buttonOther.attach(pinOther,true,true);
        break;
    default:
      delay(10000);
  }

  servoIndex.attach(pinServoIndex);//index
  servoOther.attach(pinServoOther);//other
  servoThumb.attach(pinServoThumb);//thumb
}

void loop() {
//==waiting for calibration==
  if(onSerial) Serial.println("======Waiting for Calibration======");
  while (1) {
    servoIndex.write(outIndexOpen);
    servoOther.write(outOtherOpen);
    servoThumb.write(outThumbOpen);
    if(onSerial) serialMonitor();
    delay(10);
    if (buttonCalib.read() == LOW) {
      calibration();
      break;
    }
  }
 //==control==
  position = positionMin;
  prePosition = positionMin;
  while (1) {
    // ==Read Switch State==
    if (buttonCalib.read() == LOW) {
        calibration();
    }
    if (buttonGrasp.read() == LOW) {
        isGrasp = !isGrasp;
        while (buttonGrasp.read() == LOW) delay(1);
    }
    if (buttonThumb.read() == LOW) {
        isThumbOpen = !isThumbOpen;
        while (buttonThumb.read() == LOW) delay(1);
    }
    if (buttonOther.read() == LOW) {
        isOtherLock = !isOtherLock;
        while (buttonOther.read() == LOW) delay(1);
    }

    // ==Read Sensor Value==
    sensorValue = readSensor();
    delay(25);
    if(sensorValue<sensorMin) sensorValue=sensorMin;
    else if(sensorValue>sensorMax) sensorValue=sensorMax;
    sensorToPosition();

    // ==Drive servos==
    if (isGrasp) {
        outIndex = map(position, positionMin, positionMax, outIndexOpen, outIndexClose);
        servoIndex.write(outIndex);
    } else {
        outIndex = map(position, positionMin, positionMax, outIndexClose, outIndexOpen);
        servoIndex.write(outIndex);
    }
    if (!isOtherLock){
        if (isGrasp) {
            outOther = map(position, positionMin, positionMax, outOtherOpen, outOtherClose);
            servoOther.write(outOther);
        } else {
            outOther = map(position, positionMin, positionMax, outOtherClose, outOtherOpen);
            servoOther.write(outOther);
        }
    }
    if(isThumbOpen) servoThumb.write(outThumbOpen);
    else servoThumb.write(outThumbClose);
    if(onSerial) serialMonitor();
  }
}

/*
 * functions
 */
int readSensor() {
  int i, sval;
  for (i = 0; i < 10; i++) {
    sval += analogRead(pinSensor);
  }
  sval = sval/10;
  return sval;
}

void sensorToPosition(){
  int tmpVal = map(sensorValue, sensorMin, sensorMax, 100, 0);
  if(tmpVal<thSpeedReverse) speed=speedReverse;
  else if(tmpVal<thSpeedZero) speed=speedMin;
  else speed=map(tmpVal,40,100,speedMin,speedMax);
  position = prePosition + speed;
  if (position < positionMin) position = positionMin;
  if (position > positionMax) position = positionMax;
  prePosition = position;
}

void calibration() {
  outIndex=outIndexOpen;
  servoIndex.write(outIndexOpen);
  servoOther.write(outOtherClose);
  servoThumb.write(outThumbOpen);
  position=positionMin;
  prePosition=positionMin;

  delay(200);
  if(onSerial) Serial.println("======calibration start======");

  sensorMax = readSensor();
  sensorMin = sensorMax - 50;
  unsigned long time = millis();
  while ( millis() < time + 4000 ) {
    sensorValue = readSensor();
    delay(25);
    if ( sensorValue < sensorMin ) sensorMin = sensorValue;
    else if ( sensorValue > sensorMax )sensorMax = sensorValue;

    sensorToPosition();
    outIndex = map(position, positionMin, positionMax, outIndexOpen, outIndexClose);
    servoIndex.write(outIndex);

    if(onSerial) serialMonitor();
  }
  if(onSerial)  Serial.println("======calibration finish======");
}

void serialMonitor(){
  Serial.print("S/N="); Serial.print(SerialNum);
  Serial.print(", Min="); Serial.print(sensorMin);
  Serial.print(", Max="); Serial.print(sensorMax);
  Serial.print(", sensor="); Serial.print(sensorValue);
  Serial.print(", speed="); Serial.print(speed);
  Serial.print(", position="); Serial.print(position);
  Serial.print(", outIndex="); Serial.print(outIndex);
  Serial.print(", isThumbOpen="); Serial.print(isThumbOpen);
  Serial.print(", isOtherLock="); Serial.println(isOtherLock);
}
