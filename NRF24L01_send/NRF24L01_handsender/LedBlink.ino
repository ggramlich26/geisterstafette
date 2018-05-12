#include "LedBlink.h"

int m_pin_green = 0;
int m_pin_red = 0;

int m_count = 0;
blk_mode m_mode = off;
blk_color m_color = green;
bool m_status = false;
long m_last_toggle = 0;

void blk_init(int pin_green, int pin_red){
	m_pin_green = pin_green;
	m_pin_red = pin_red;
	pinMode(pin_green, OUTPUT);
	pinMode(pin_red, OUTPUT);
	led_off();
}

void color_selective_on(){
	if(green == m_color){
		digitalWrite(m_pin_green, HIGH);
		digitalWrite(m_pin_red, LOW);
	}
	else if(red == m_color){
		digitalWrite(m_pin_green, LOW);
		digitalWrite(m_pin_red, HIGH);
	}
	if(orange == m_color){
		digitalWrite(m_pin_green, HIGH);
		digitalWrite(m_pin_red, HIGH);
	}
}

void led_off(){
	digitalWrite(m_pin_green, LOW);
	digitalWrite(m_pin_red, LOW);
}

void blk_main(){
	if(off == m_mode){
		led_off();
		return;
	}
	else if(steady == m_mode){
		color_selective_on();
	}
	else if(slow == m_mode){
		if(millis()-m_last_toggle >= SLOW_INTERVAL){
			m_last_toggle = millis();
			if(m_status){
				m_status = false;
				led_off();
			}
			else{
				m_status = true;
				color_selective_on();
			}
		}
	}
	else if(fast == m_mode){
		if(millis()-m_last_toggle >= FAST_INTERVAL){
			m_last_toggle = millis();
			if(m_status){
				m_status = false;
				led_off();
			}
			else{
				m_status = true;
				color_selective_on();
			}
		}
	}
	else if(three_slow == m_mode){
		if(m_count < 3){
			if(millis()-m_last_toggle >= SLOW_INTERVAL){
				m_last_toggle = millis();
				if(m_status){
					m_status = false;
					led_off();
					m_count++;
				}
				else{
					m_status = true;
					color_selective_on();
				}
			}
		}
	}
	else if(three_fast == m_mode){
		if(m_count < 3){
			if(millis()-m_last_toggle >= FAST_INTERVAL){
				m_last_toggle = millis();
				if(m_status){
					m_status = false;
					led_off();
					m_count++;
				}
				else{
					m_status = true;
					color_selective_on();
				}
			}
		}
	}
	
}

void blk_set_mode(blk_mode mode, blk_color color){
	m_mode = mode;
	m_color = color;
	m_count = 0;
	m_status = false;
	m_last_toggle = 0;
	blk_main();
}
