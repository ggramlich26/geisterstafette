#include "dev.h"
#include "stimer.h"

void dev_updateStepupFrequency();


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
	pinMode(STEPUP_PIN, OUTPUT);
	pinMode(FLASH_PIN, OUTPUT);

	digitalWrite(LED_PIN, LOW);
	digitalWrite(STEPUP_PIN, LOW);
	digitalWrite(FLASH_PIN, LOW);


	//set reverence voltage to 1.1V
	analogReference(INTERNAL);

	stimer_setCallback(dev_updateStepupFrequency);
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


void dev_ledOn(){
	digitalWrite(LED_PIN, HIGH);
}

void dev_ledOff(){
	digitalWrite(LED_PIN, LOW);
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
		lastMeasTime = millis();
		int conv = analogRead(BAT_MEAS_PIN);
		float voltage = conv*BAT_CONV_FACTOR/0x3FF;
		if(voltage <= BAT_LOW_VOLTAGE){
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


//////////////////////////////////////////////////////////////
//															//
//							Stepup							//
//															//
//////////////////////////////////////////////////////////////

bool stepup_enabled = false;

/** The stepup pin will be cleared when reaching val, so a value of 255
 *  results in one low cycle per Period
**/
void dev_setStepupValue(uint8_t val){
	OCR2B = val;
}

void dev_updateStepupFrequency(){
	//DEVICE_ID == 90
	uint8_t val[] = {215, 240, 240, 240};
	//DEVICE_ID == 91
	//uint8_t val[] = {217, 240, 240, 240};
	//DEVICE_ID == 92
	//uint8_t val[] = {217, 242, 242, 242};
	uint16_t voltage[] = {120, 220, 270, 300};
	uint16_t times[] = {700,1400,2100,3200};
	uint8_t steps = 4;
	uint16_t cur_voltage = analogRead(STEPUP_VOLTAGE_PIN)/1024.0*STEPUP_CONV_FACTOR;
	if(cur_voltage >= 300){
		dev_pauseStepup();
		stimer_startTimer(5000);
		return;
	}
	uint8_t i = 0;
	for(i = 0; i < steps; i++){
		if(cur_voltage < voltage[i]){
			break;
		}
	}
	dev_setStepupValue(val[i]);
	if(i > 0){
		stimer_startTimer((times[i]-times[i-1])*(float)(voltage[i]-cur_voltage)/(voltage[i]-voltage[i-1]));
	}
	else{
		stimer_startTimer((times[0])*(float)(voltage[0])/(voltage[0]));
	}
}

void dev_startStepup(){
	stepup_enabled = true;
	//Initialize Timer 2 for PWM Mode on OC2B, fast PWM mode, count till MAX
	TCCR2A = (1<<COM2B1)|(1<<WGM21)|(1<<WGM20);
	//start PWM with prescaler 8
	TCCR2B = (1<<CS21);
	dev_updateStepupFrequency();
}

void dev_stopStepup(){
	stepup_enabled = false;
	stimer_stopTimer();
	TCCR2B = 0x00;
	digitalWrite(STEPUP_PIN, LOW);
}

void dev_pauseStepup(){
	dev_setStepupValue(0);
}

void dev_flash(){
	bool enabled = stepup_enabled;
	dev_stopStepup();
	digitalWrite(FLASH_PIN, HIGH);
	delay(5);
	digitalWrite(FLASH_PIN, LOW);
	if(enabled){
		dev_startStepup();
	}
}

