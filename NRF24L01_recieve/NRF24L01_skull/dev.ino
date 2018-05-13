/*
 * dev.ino
 *
 * Created: 20.03.2018
 *  Author: tsugua
 */ 

#include "dev.h"
#include <avr/interrupt.h>


volatile uint8_t led0Value;		//these variables contain the PWM value for the LEDs. Mind that inverted PWM is used.
volatile uint8_t led1Value;
volatile uint8_t led2Value;
volatile uint8_t led3Value;
volatile uint8_t pulseOn;



void dev_init(){
	
	pinMode(LED0, OUTPUT);
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(LED3, OUTPUT);

	digitalWrite(LED0, LOW);
	digitalWrite(LED1, LOW);
	digitalWrite(LED2, LOW);
	digitalWrite(LED3, LOW);

	pinMode(DIP0, INPUT);
	pinMode(DIP1, INPUT);
	pinMode(DIP2, INPUT);
	pinMode(DIP3, INPUT);

	//enable pullup resistors
	digitalWrite(DIP0, HIGH);
	digitalWrite(DIP1, HIGH);
	digitalWrite(DIP2, HIGH);
	digitalWrite(DIP3, HIGH);

	//turn off LEDs
	led0Value = 0xFF;
	led1Value = 0xFF;
	led2Value = 0xFF;
	led3Value = 0xFF;
	
	pulseOn = 0;
	
	//Timer2 initialisation for LED PWM control
	//prescaler 64 Timer2 overflow interrupt enabled
	TCCR2B = (1<<CS20);//|(1<<CS20);
	TIMSK2 |= (1<<TOIE2);

	//set reverence voltage to 1.1V
	analogReference(INTERNAL);
}


//////////////////////////////////////////////////////////////
//															//
//					Dip Switches							//
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
//					LED										//
//															//
//////////////////////////////////////////////////////////////


void dev_setLED0(uint8_t value){
	led0Value = 255 - value;
}

void dev_setLED1(uint8_t value){
	led1Value = 255 - value;
}

void dev_setLED2(uint8_t value){
	led2Value = 255 - value;
}

void dev_setLED3(uint8_t value){
	led3Value = 255 - value;
}

ISR(TIMER2_OVF_vect){
	static uint8_t count = 0;
	count++;
	if(count == 0){
//		LEDPORT &= ~((1<<LED0)|(1<<LED1)|(1<<LED2)|(1<<LED3));
		digitalWrite(LED0, LOW);
		digitalWrite(LED1, LOW);
		digitalWrite(LED2, LOW);
		digitalWrite(LED3, LOW);
	}
	if(count > led0Value){
//		LEDPORT |= (1<<LED0);
		digitalWrite(LED0, HIGH);
	}
	if(count > led1Value){
//		LEDPORT |= (1<<LED1);
		digitalWrite(LED1, HIGH);
	}
	if(count > led2Value){
//		LEDPORT |= (1<<LED2);
		digitalWrite(LED2, HIGH);
	}
	if(count > led3Value){
//		LEDPORT |= (1<<LED3);
		digitalWrite(LED3, HIGH);
	}
}

void dev_main(){
	if(pulseOn){
		dev_pulse();
	}
}

void dev_allOn(){
	pulseOn = 0;
	dev_setLED0(255);
	dev_setLED1(255);
	dev_setLED2(255);
	dev_setLED3(255);
}

void dev_allOff(){
	pulseOn = 0;
	dev_setLED0(0);
	dev_setLED1(0);
	dev_setLED2(0);
	dev_setLED3(0);
}

void dev_startPulse(){
//	//make sure the pulse is actually being restarted
//	dev_stopPulse();

	dev_pulse();
}

void dev_stopPulse(){
	pulseOn = 0;
	dev_setLED0(0);
	dev_setLED1(0);
	dev_setLED2(0);
	dev_setLED3(0);
}

void dev_pulse(){
	static uint32_t pulseStartTime = 0;
	static uint32_t led0NextStepTime = 0;
	static uint32_t led1NextStepTime = 0;
	static uint32_t led2NextStepTime = 0;
	static uint32_t led3NextStepTime = 0;
	if(!pulseOn){
		pulseOn = 0x01;
		pulseStartTime = millis();
	}
	if(millis() > pulseStartTime + PULSEPART1TIME){
		if(millis() > led0NextStepTime){
			dev_setLED0(dev_getRandomNumber(180, 220));
			led0NextStepTime = millis() + dev_getRandomNumber(50, 100);
		}
		if(millis() > led1NextStepTime){
			dev_setLED1(dev_getRandomNumber(180, 220));
			led1NextStepTime = millis() + dev_getRandomNumber(50, 100);
		}
		dev_setLED2(255);
		dev_setLED3(255);
		if(millis() > pulseStartTime + PULSEPART1TIME + PULSEPART2TIME){
			dev_setLED0(0x00);
			dev_setLED1(0x00);
			dev_setLED2(0x00);
			dev_setLED3(0x00);
			led0NextStepTime = 0;
			led1NextStepTime = 0;
			led2NextStepTime = 0;
			led3NextStepTime = 0;
			pulseOn = 0;
		}
	}
	else{//falls noch in Part1
		if(millis() > led2NextStepTime){
			uint8_t value = dev_getRandomNumber(0, 150);
			if(value < 6){
				dev_setLED2(value);
				led2NextStepTime = millis() + dev_getRandomNumber(50, 100) * 4;
			}
			else{
				dev_setLED2(value);
				led2NextStepTime = millis() + dev_getRandomNumber(50, 100);
			}
		}
		if(millis() >= led3NextStepTime){
			uint8_t value = dev_getRandomNumber(0, 150);
			if(value < 6){
				dev_setLED3(value);
				led3NextStepTime = millis() + dev_getRandomNumber(50, 100) * 4;
			}
			else{
				dev_setLED3(value);
				led3NextStepTime = millis() + dev_getRandomNumber(50, 100);
			}
		}
	}
}

uint8_t dev_getRandomNumber(uint8_t low, uint8_t high){//low: unterge Grenze, high: obere Grenze, in der die Zufallszahl liegen darf
	static uint16_t lfsr = 0xACE1u;
	for(uint8_t i = 0; i < 8; i++){
		unsigned bit;
		bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
		lfsr =  (lfsr >> 1) | (bit << 15);
	}
	if(low <= high){
		while(!(low <= (lfsr & 0xFF) && (lfsr & 0xFF) <= high)){
			unsigned bit;
			bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
			lfsr =  (lfsr >> 1) | (bit << 15);
		}
		return (uint8_t)lfsr;
	}
	return 0x00;
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
