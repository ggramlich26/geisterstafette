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
	digitalWrite(LED_PIN, LOW);
	pinMode(SENSOR_PIN, INPUT);

	analogReference(INTERNAL);	//set reverence voltage to 1.1V
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
	return digitalRead(SENSOR_PIN);
}


//////////////////////////////////////////////////////////////
//															//
//							Battery							//
//															//
//////////////////////////////////////////////////////////////

bool dev_getBatteryLow(){
	static bool isLow = false;
	static int nLow = 0;
	static long lastMeasTime = 0;
	if(millis()-lastMeasTime >= BAT_UPDATE_INTERVAL){
		if(analogRead(BAT_MEAS_PIN)*BAT_CONV_FACTOR/0x3FF <= BAT_LOW_VOLTAGE){
			nLow++;
			if(nLow >= 5){	//only accept voltage as low after consecutive low measurements
				isLow = true;
			}
		}
		else{
			isLow = false;
			nLow = 0;
		}
	}
	return isLow;
}
