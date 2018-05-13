#ifndef _BUTTON_H_
#define _BUTTON_H_

#define DEBOUNCE_INTERVAL	100 //time in ms in which the button can bounce
#define LONG_PRESS_INTERVAL	500	//time in ms after which a button will be considered to be long pressed
#define MAX_BUTTONS	10 //max number of buttons that can be tracked
enum btn_action{short_press, long_press};

typedef void (*btn_callback)(btn_action action, int pin);


void btn_main();
//register a button to be tracked
void btn_track_pin(int pin, btn_callback callback, bool pullup);



#endif
