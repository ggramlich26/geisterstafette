#ifndef  _STIMER_H
#define _STIMER_H

void stimer_setCallback(void (*f)());
void stimer_startTimer(uint64_t ms);
void stimer_stopTimer();
void stimer_update();

#endif
