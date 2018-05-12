/*
 * dev.h
 *
 * Created: 02.06.2014 09:10:48
 *  Author: tsugua
 */ 


#ifndef DEV_H_
#define DEV_H_

#define DIP0				6 //16 //PC2
#define DIP1				7 //15 //PC1
#define DIP2				8 //14 //PC0
#define DIP3				9
#define LED_PIN				10
#define SENSOR_PIN			A4


void dev_init();
bool dev_getDip0();
bool dev_getDip1();
bool dev_getDip2();
bool dev_getDip3();
void dev_ledOn();
void dev_ledOff();
bool dev_getSensor();


#endif /* DEV_H_ */
