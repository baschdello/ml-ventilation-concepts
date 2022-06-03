#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "constants.h"

#define DETECTION_TYPE ONLINE
#define WARNING_DURATION  255    // time in seconds to warn if a bad ventilation occurs
#define TRAINING_DURATION   4    // amount of ventilations for training (online type)

#define STATE_TOPIC    "state/myroom"
#define CMD_TOPIC      "cmd/myroom"   // subscribe topic for cmd from python => esp
#define MEAS_CO2_TOPIC "meas/myroom/co2"
#define MEAS_TEM_TOPIC "meas/myroom/temp"
#define MEAS_HUM_TOPIC "meas/myroom/humidity"

#define WIFI_SSID   ""
#define WIFI_PASSWD ""

#define MQTT_SERVER "192.168.2.27"
#define MQTT_PORT   1883

// Amtlicher Gemeindeschluessel => https://www.statistikportal.de/de/gemeindeverzeichnis
#define AGS     "07136"

float   B       = 0.516;
uint8_t N       = 10;
float   D       = 0.833;
float   m_ex    = 0.9;
float   m_in    = 0.9;
float   Ep      = 100;
float   Epco2   = 0.0203;
float   lambd_0 = 3.0;
float   lambd   = 3.92;
float   eta_im  = 0.0;
float   eta_i   = 0.001;

#endif
