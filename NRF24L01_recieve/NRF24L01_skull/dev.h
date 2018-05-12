/*
 * dev.h
 *
 * Created: 02.06.2014 09:10:48
 *  Author: tsugua
 */ 


#ifndef DEV_H_
#define DEV_H_

#define LED0				2 //PB3	//weiss
#define LED1				3 //PB2	//weiss	
#define LED2				4 //PB1	//rot
#define LED3				5 //PB0	//rot

#define DIP0				6 //16 //PC2
#define DIP1				7 //15 //PC1
#define DIP2				8 //14 //PC0
#define DIP3				9

#define PULSEPART1TIME		10000
#define PULSEPART2TIME		5000


void dev_init();
bool dev_getDip0();
bool dev_getDip1();
bool dev_getDip2();
bool dev_getDip3();
void dev_setLED0(uint8_t value);
void dev_setLED1(uint8_t value);
void dev_setLED2(uint8_t value);
void dev_setLED3(uint8_t value);
void dev_main();
void dev_startPulse();
void dev_stopPulse();
void dev_pulse();
uint8_t dev_getRandomNumber(uint8_t low, uint8_t high);


#endif /* DEV_H_ */
