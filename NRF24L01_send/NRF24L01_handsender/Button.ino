#include "Button.h"


enum btn_state{low, debounce_in, high, debounce_out};

typedef struct button_s{
int pin;
long start_millis;
btn_callback callback;
btn_state state;
bool pullup;
} button_t;

int n_buttons = 0;
button_t buttons[MAX_BUTTONS];

void btn_main(){
	for(uint8_t i = 0; i < n_buttons; i++){
		int state = digitalRead(buttons[i].pin);
		if(buttons[i].pullup == true){
			if(buttons[i].state == high && state == 0){
				buttons[i].state = debounce_in;
				buttons[i].start_millis = millis();
			}
			else if(buttons[i].state == debounce_in){
				if(millis() - buttons[i].start_millis >= DEBOUNCE_INTERVAL){
					buttons[i].state = low;
				}
			}
			else if(buttons[i].state == low && state > 0){
				buttons[i].state = debounce_out;
				if(millis()-buttons[i].start_millis >= LONG_PRESS_INTERVAL){
					buttons[i].callback(long_press, buttons[i].pin);
				}
				else{
					buttons[i].callback(short_press, buttons[i].pin);
				}
			}
			else if(buttons[i].state == debounce_out){
				if(millis()-buttons[i].start_millis >= DEBOUNCE_INTERVAL){
					buttons[i].state = high;
				}
			}
		}
		else{
			if(buttons[i].state == low && state > 0){
				buttons[i].state = debounce_in;
				buttons[i].start_millis = millis();
			}
			else if(buttons[i].state == debounce_in){
				if(millis() - buttons[i].start_millis >= DEBOUNCE_INTERVAL){
					buttons[i].state = high;
				}
			}
			else if(buttons[i].state == high && state == 0){
				buttons[i].state = debounce_out;
				if(millis()-buttons[i].start_millis >= LONG_PRESS_INTERVAL){
					buttons[i].callback(long_press, buttons[i].pin);
				}
				else{
					buttons[i].callback(short_press, buttons[i].pin);
				}
			}
			else if(buttons[i].state == debounce_out){
				if(millis()-buttons[i].start_millis >= DEBOUNCE_INTERVAL){
					buttons[i].state = low;
				}
			}
		}
	}
}

void btn_track_pin(int pin, btn_callback callback, bool pullup){
	if(n_buttons == MAX_BUTTONS)
		return;
	buttons[n_buttons].pin = pin;
	buttons[n_buttons].callback = callback;
	buttons[n_buttons].pullup = pullup;
	buttons[n_buttons].start_millis = 0;
	pinMode(pin, INPUT);
	if(pullup){
		digitalWrite(pin, HIGH);
		buttons[n_buttons].state = high;
	}
	else {
		digitalWrite(pin, LOW);
		buttons[n_buttons].state = low;
	}
	n_buttons++;
}
	
