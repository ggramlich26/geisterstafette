
#ifndef DEV_H_
#define DEV_H_


#define DIP0				6 //16 //PC2
#define DIP1				7 //15 //PC1
#define DIP2				8 //14 //PC0
#define DIP3				9
#define LED_PIN				10

#define BAT_MEAS_PIN		A6
#define BAT_CONV_FACTOR		1.1*6
#define BAT_UPDATE_INTERVAL	1000 //time in ms after which a new battery measurement will be performed. It takes 5 times this time befor a low battery voltage will actually be considered as low
#define BAT_LOW_VOLTAGE		3.1		//Highest voltage that will still be considered as low

#define STEPUP_PIN				3	//OC2B = PD3 = P3
#define FLASH_PIN				A5	//PC5
#define STEPUP_VOLTAGE_PIN		A4	//ADC4 = A4
#define	STEPUP_CONV_FACTOR		420*1.1


void dev_init();
bool dev_getDip0();
bool dev_getDip1();
bool dev_getDip2();
bool dev_getDip3();
void dev_main();
bool dev_getBatteryLow();
void dev_ledOn();
void dev_ledOff();
void dev_startStepup();
void dev_stopStepup();
void dev_pauseStepup();
void dev_flash();

#endif
