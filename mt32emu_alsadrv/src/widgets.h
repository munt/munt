#ifndef XGUI_WIDGETS_H
#define XGUI_WIDGETS_H

#define WINDOW_W     480
#define WINDOW_H     100 

/* geometry */
#define BUTTONPAD_X  253
#define BUTTONPAD_Y   20
#define BUTTONPAD_W  210
#define BUTTONPAD_H   60
//#define BUTTON_W      38
#define BUTTON_W      70
#define BUTTON_H      20

#define DISPLAY_X     22
#define DISPLAY_Y     10
#define DISPLAY_W     215
#define DISPLAY_H     79


/* General X variables */
extern Display  *dpy;
extern Window   rootWindow, mainWindow;
extern GC       mainGC;
extern Colormap mainCM;
extern int      mainScreen;
extern int      rootDepth;

/* Colours */
extern XColor LCDColour;
extern XColor LCDTextColour;
extern XColor LCDSoftColour, LCDHardColour;
extern XColor edgeColour;
extern XColor buttonColour;

/* Some settings */
extern int xreverb_switch;

XFontStruct * load_font(char **fontlist);
void get_string_dims(XFontStruct *fs, char *txt, int *_w, int *_h);
Pixmap create_button(char *txt);

void create_pixmaps();
void quit();

extern Pixmap pix_stripe, pix_logo, pix_button;
extern int stipe_width, stripe_height;
extern int logo_width, logo_height;
extern int button_width, button_height;


#endif


