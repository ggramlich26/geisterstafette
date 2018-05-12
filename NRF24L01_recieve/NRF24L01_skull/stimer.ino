#include "stimer.h"

void (*callback)();
uint64_t end_time = 0;
bool timer_enabled = false;


void stimer_setCallback(void (*f)()){
	callback = f;
}

void stimer_startTimer(uint64_t ms){
	end_time = millis() + ms; 
	timer_enabled = true;
}

void stimer_update(){
	if(timer_enabled && millis() > end_time){
		timer_enabled = false;
		(*callback)();
	}
}

void stimer_stopTimer(){
	timer_enabled = false;
}
