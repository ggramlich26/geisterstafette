/*
 * dev.ino
 *
 * Created: 20.03.2018
 *  Author: tsugua
 */ 

#include "dev.h"

void dev_init(){
	
	pinMode(DIP0, INPUT);
	pinMode(DIP1, INPUT);
	pinMode(DIP2, INPUT);
	pinMode(DIP3, INPUT);

	//enable pullup resistors
	digitalWrite(DIP0, HIGH);
	digitalWrite(DIP1, HIGH);
	digitalWrite(DIP2, HIGH);
	digitalWrite(DIP3, HIGH);

	pinMode(LED_PIN, OUTPUT);
	if(SENSOR_PIN == A6 || SENSOR_PIN == A7){
		analogReference(DEFAULT);	//set reverence voltage to AVCC, which is 3.3V on this board
	}
	else{
		pinMode(SENSOR_PIN, INPUT);
	}
}


//////////////////////////////////////////////////////////////
//															//
//					DIP Switches							//
//															//
//////////////////////////////////////////////////////////////

bool dev_getDip0(){
	return !digitalRead(DIP0);
}

bool dev_getDip1(){
	return !digitalRead(DIP1);
}

bool dev_getDip2(){
	return !digitalRead(DIP2);
}

bool dev_getDip3(){
	return !digitalRead(DIP3);
}


//////////////////////////////////////////////////////////////
//															//
//							 LED							//
//															//
//////////////////////////////////////////////////////////////

void dev_ledOn(){
	digitalWrite(LED_PIN, HIGH);
}

void dev_ledOff(){
	digitalWrite(LED_PIN, LOW);
}


//////////////////////////////////////////////////////////////
//															//
//							 Sensor							//
//															//
//////////////////////////////////////////////////////////////

bool dev_getSensor(){
	if(SENSOR_PIN == A6 || SENSOR_PIN == A7){
		return analogRead(SENSOR_PIN) > 0x3FF*0.5/3.3; //voltages higher than 0.5V will be considered as high
	}
	else{
		return digitalRead(SENSOR_PIN);
	}
}
