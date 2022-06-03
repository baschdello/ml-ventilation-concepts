#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#define REED_SWITCH 12          

#define UNTRAINED 0
#define TRAINING  1
#define TRAINED   2

#define ONLINE  1
#define OFFLINE 2

#define WINDOW_CLOSED 0
#define WINDOW_OPENED 1

#define CLI_MAX_BUF 15

#define set_led_red()           pixels.setPixelColor(0,40,0,0,0);  pixels.show();
#define set_led_green()         pixels.setPixelColor(0,0,30,0,0);  pixels.show();
#define set_led_off()           pixels.setPixelColor(0,0,0,0,0);   pixels.show();

#endif
