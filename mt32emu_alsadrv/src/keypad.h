#ifndef MT_KEYPAD_H
#define MT_KEYPAD_H

typedef struct
{
	Pixmap p;
	void (*cb)(int);
	int w, h;
} button_t;

typedef struct
{
	button_t buttons[9];
} button_set_t;


extern button_set_t *cset;
extern button_set_t mainmenu, setupmenu, reverbmenu, recordmenu;


void create_buttonsets();

#endif

