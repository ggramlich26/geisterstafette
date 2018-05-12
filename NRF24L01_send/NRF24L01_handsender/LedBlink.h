#ifndef _LEDBLINK_H_
#define _LEDBLINK_H_

#define SLOW_INTERVAL	500		//time in ms untill the LED will be toggled in slow blink mode
#define FAST_INTERVAL	50		//time in ms untill the LED will be toggled in fast blink mode


enum blk_mode{off, steady, slow, fast, three_slow, three_fast};
enum blk_color{green, red, orange};

void blk_init(int pin_green, int pin_red);
void blk_main();
void blk_set_mode(blk_mode, blk_color);


#endif
