/* This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details. */
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Ticker.h>
#include <PubSubClient.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <ArduinoJson.h>
#include "knn.h"
#include "constants.h"
#include "config.h"

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2,13,NEO_GRBW + NEO_KHZ800);
SCD30 airSensorSCD30; // Objekt SDC30 Umweltsensor
uint16_t counter;
Ticker seconds_timer;
Ticker sec_timer;
String output_str = "";
char cli_command[CLI_MAX_BUF];
uint8_t command_index;

WiFiClient   espClient; 
PubSubClient mqttclient(espClient);

struct state_t {
    unsigned measure:1;
    unsigned checkIncidence:1;
    unsigned state:2;
    uint8_t  warn;
    uint16_t co2;
    float    temp;
    float    hum;
    float    incidence;
#if DETECTION_TYPE==OFFLINE
    float    co2_window[WINDOW_SIZE];
    uint8_t  knn_class;
#else
    unsigned window_open:1;
#endif
}sys;

String MQTT_Rx_Payload = "" ;
//--------- mqtt callback function 
void mqttcallback(char* to, byte* pay, unsigned int len) {
    String topic   = String(to);
    String payload = String((char*)pay);
#if DETECTION_TYPE==ONLINE
    MQTT_Rx_Payload=payload.substring(0,len);
    Serial.println(MQTT_Rx_Payload);
    if(MQTT_Rx_Payload == "trained") {
        mqttclient.publish(STATE_TOPIC, "rec: trained");
        sys.state = TRAINED;
    }
    else if(MQTT_Rx_Payload == "vent fault") {
        mqttclient.publish(STATE_TOPIC, "rec: vent fault");
        if(!sys.warn)  ///////////////////// aenderung am 14.01.2022 um doppelblinken zu vermeiden
            sys.warn = WARNING_DURATION;
    }
    else if(MQTT_Rx_Payload == "untrained") {
        mqttclient.publish(STATE_TOPIC, "rec: not trained");
        sys.state = UNTRAINED;
    }
#endif
}

//------------ reconnect mqtt-client
void mqttreconnect() { // Loop until we're reconnected 
    if (!mqttclient.connected()) { 
        while (!mqttclient.connected()) { 
            Serial.print("Attempting MQTT connection...");
            if (mqttclient.connect("myroom_sensor" , "Text", "Text" )) {
                Serial.println("connected");
                mqttclient.subscribe(CMD_TOPIC);
            } 
            else { 
                Serial.print("failed, rc=");
                Serial.print(mqttclient.state());
                Serial.println(" try again in 5 seconds");
                delay(5000);
            }
        }
    } 
    else { 
        mqttclient.loop(); 
    }
}

int httpGET(String host, String cmd, String &antwort,int Port) {
    WiFiClient client; // Client über WiFi
    String text = "GET " + cmd + " HTTP/1.1\r\n";
    text = text + "Host:" + host + "\r\n";
    text = text + "Connection:close\r\n\r\n";
    int ok = 1;
    if (ok) { // Netzwerkzugang vorhanden 
        ok = client.connect(host.c_str(),Port);// verbinde mit Client
        if (ok) {
            client.print(text);                 // sende an Client 
            for (int tout=1000;tout>0 && client.available()==0; tout--)
                delay(10);                         // und warte auf Antwort
            if (client.available() > 0)         // Anwort gesehen 
                while (client.available())         // und ausgewertet
                    antwort = antwort + client.readStringUntil('\r');
            else ok = 0;
            client.stop(); 
            Serial.println(antwort);
        } 
    } 
    if (!ok) Serial.print(" no connection"); // Fehlermeldung
    return ok;
}

int httpsGETinsec(String host, String cmd, String &antwort,int Port) {
    WiFiClientSecure client; // Client über WiFi
    client.setInsecure();
    String text = "GET "+ cmd + " HTTP/1.1\r\n"
        + "Host:" + host + "\r\n"
        + "Connection: close\r\n\r\n";
    int ok = 1;
    ok = client.connect(host.c_str(),Port);// verbinde mit Client
    if (ok) {
        client.print(text);                 // sende an Client 
        //Serial.print("Client text: "); Serial.println(text);
        for (int tout=1000; tout>0 && client.available()==0; tout--)
            delay(10);                         // und warte auf Antwort
        if (client.available() > 0)         // Anwort gesehen 
            while (client.available())         // und ausgewertet
                antwort = antwort + client.readStringUntil('\r');
        else ok = 0;
        client.stop(); 
    }
    if (!ok) Serial.print(" no connection"); // Fehlermeldung
    return ok;
}

void setup(){ // Einmalige Initialisierung
    // basic io
    Serial.begin(115200);
    pinMode(REED_SWITCH, INPUT);
    pinMode(LED_BUILTIN,OUTPUT);
    digitalWrite(LED_BUILTIN,1); // turn led off
    pixels.begin(); // neopixel
    delay(1);
    pixels.show();
    set_led_off(); // show signal wifi not connected yet
    Wire.begin();
    while (Wire.status() != I2C_OK){
        Serial.println("Something wrong with I2C");
        delay(1000);
    }

    //scd30 init
    if (airSensorSCD30.begin() == false) {
        while(1) {
            Serial.println("The SCD30 did not respond. Please check wiring."); 
            yield(); 
            delay(1000);
        } 
    }

#if DETECTION_TYPE==OFFLINE
    sys.knn_class = 3;
#else
    sys.window_open = digitalRead(REED_SWITCH);
#endif

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID,WIFI_PASSWD);
    while (WiFi.status() != WL_CONNECTED) { // Warte bis Verbindung steht 
        delay(500); 
        Serial.print(".");
    };
    Serial.println ("\nconnected, meine IP:"+ WiFi.localIP().toString());

    mqttclient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttclient.setCallback(mqttcallback);
    mqttclient.subscribe(CMD_TOPIC);

    sec_timer.attach(1,sec_handler);
    seconds_timer.attach(60,minute_handler);

    getIncidence();
    requestTrainingState();
}

void loop() {
    while(Serial.available() > 0)
    {
        char c = Serial.read();
        if((c >= ' ') && command_index < CLI_MAX_BUF-2) // Steuerzeichen als Trennzeichen verwenden
        {
            cli_command[command_index++] = c;
        }
        else
        {
            cli_command[command_index] = '\0';
            for(int i=0; i<CLI_MAX_BUF; i++)
            {
                cli_command[i] = tolower(cli_command[i]);
            }
            command_interpreter();
            command_index = 0;
            Serial.flush(); // restlichen Buffer verwerfen
        }
    }

    if(sys.warn%2!=0) {
        set_led_red();
    }
    else {
        set_led_off();
    }

    if(sys.measure)
    {
        sys.co2    = airSensorSCD30.getCO2();
        sys.temp   = airSensorSCD30.getTemperature();
        sys.hum    = airSensorSCD30.getHumidity();

#if DETECTION_TYPE==OFFLINE
        add_value_to_win(sys.co2_window, WINDOW_SIZE, sys.co2);
        uint8_t previously_knn_class = sys.knn_class;
        sys.knn_class = knn(&training_data, 1, sys.co2_window, WINDOW_SIZE, 2);

        if( !ventilation_active() && previously_knn_class < 3) { // Lüftung Ende => gegen CO2-Schwellwert prüfen
            Serial.print("Lüftungsqualität: ");
            if(sys.co2 >= 600) {
                Serial.println("keine gute Lüftung!");
                mqttreconnect();
                mqttclient.publish(STATE_TOPIC,"Lüftungsqualität: keine gute Lüftung");
                sys.warn = WARNING_DURATION;
            }
            else {
                Serial.println("Lüftung ok");
                mqttreconnect();
                mqttclient.publish(STATE_TOPIC,"Lüftungsqualität: Lüftung ok");
            }
        }
        print_class();        // show knn class in console
#else
        if(digitalRead(REED_SWITCH)) { // window open
            if(sys.window_open == 0) {
                sys.window_open=1;
                mqttclient.publish(STATE_TOPIC, "window opened");
            }
        }
        else { // window close
            if(sys.window_open == 1) {
                sys.window_open=0;
                mqttclient.publish(STATE_TOPIC, "window closed");
            }
        }
#endif

        print_measurements(); // show measurements in console
        mqtt_send_measurements(); // send measurements
        sys.measure = 0;
    }
    if(sys.checkIncidence)
    {
        getIncidence();
        deltaCO2(); // Grenzwert berechnen
        sys.checkIncidence = 0;
    }
    output_str = "";
}

void startTraining()
{
    mqttreconnect();
    mqttclient.publish(STATE_TOPIC,String("start training "+TRAINING_DURATION).c_str());
    sys.state = TRAINING;
    Serial.println("Training gestartet");
}

void calibrate()
{
    Serial.println("Kalibrierung gestartet");
    mqttreconnect();
    mqttclient.publish(STATE_TOPIC,"Kalibrierung gestartet");
    airSensorSCD30.setAltitudeCompensation(0); // Altitude in m ü NN 
    airSensorSCD30.setForcedRecalibrationFactor(400); // fresh air
    delay(4000);
    Serial.println("Kalibrierung abgeschlossen");
    mqttreconnect();
    mqttclient.publish(STATE_TOPIC,"Kalibrierung abgeschlossen");
}

void sec_handler()
{
    static uint8_t five_sec=0;
    if(++five_sec >= 5) {
        five_sec = 0;
        sys.measure = 1;
    }
    if(sys.warn > 0)
        sys.warn--;
}

void minute_handler()
{
    static uint8_t incidence_counter = 0;
    if(incidence_counter++ >= 60*2) { // =2h
        sys.checkIncidence = 1;
        incidence_counter = 0;
    }
}

void command_interpreter()
{
    if(strcmp(cli_command,"kalibrieren") == 0) {
        if(sys.state != TRAINING)
            calibrate();
        else
            Serial.println("Training läuft, Kalibrierung nicht möglich.");
    }
    else if(strcmp(cli_command,"trainieren") == 0) {
        switch(sys.state) {
            case TRAINED:
                Serial.println("Ampel ist bereits trainiert. Neu anlernen? (Eingabe: j)");
                while(Serial.available() <= 0) {}; // warten auf Eingabe
                if(tolower((char)Serial.read()) == 'j') {
                    startTraining();
                }
                else
                    Serial.println("Abbruch");
                break;

            case TRAINING:
                Serial.println("Trainingsphase läuft bereits");
                break;

            case UNTRAINED:
                startTraining();
                break;
        }
    }
    else if(strcmp(cli_command,"reset")==0) {
        Serial.println("Reset ESP");
        delay(1);
        ESP.reset();
    }
    else {
        Serial.print(output_str);
        output_str = "";
    }
}

void getIncidence() {
    if(strlen(AGS) != 5) {
        Serial.println("Ungueltiger Gemeindeschluessel. Inzidenz nicht ermittelbar");
        return;
    }
    else {
        Serial.print("Verwende AGS: "); Serial.println(AGS);
        String response;
        String url = "/districts/" + (String)AGS;
        yield();
        if(httpsGETinsec("api.corona-zahlen.org", url, response, 443)) {
            uint32_t json_start = response.length()-1;
            for(;json_start>0;json_start--)
                if(response[json_start]==0x0a)
                    break;
            response = response.substring(json_start,response.length()); // filter json string from response
            //Serial.print("JSON: "); Serial.println(response);
            StaticJsonDocument <1000> json;
            DeserializationError err = deserializeJson(json, response);
            if(err) {
                Serial.print("Deserialization error: ");
                Serial.println(err.c_str());
                mqttclient.publish(STATE_TOPIC,String(String("Deserialization error: ")+String(err.c_str())).c_str());
                return;
            }
            String kreis =  json["data"][AGS]["name"];
            sys.incidence = json["data"][AGS]["weekIncidence"];
            String lastUpdate = json["meta"]["lastUpdate"];
            Serial.printf("Inzidenz von %s betraegt %f. Letztes Inzidenzupdate: %s\r\n",kreis.c_str(), sys.incidence, lastUpdate.c_str());
            mqttreconnect();
            mqttclient.publish(STATE_TOPIC,(String("Inzidenz von "+kreis+" betraegt "+sys.incidence+". Letztes Inzidenzupdate: "+lastUpdate)).c_str());
        }
        else {
            Serial.println("Fehler bei Anfrage des Inzidenzwerts");
        }
    }
}

float deltaCO2() {
    eta_i = 1-pow((1-sys.incidence/100000.0),N);
    float delta_co2 = (( (0.0001*N*Epco2) / ((1-eta_im)*eta_i*(N-1)*Ep*(1-m_ex)*(1-m_in)*B) )  *  ( ((1/lambd_0) - (1-exp(-lambd_0*D))/pow(lambd_0,2)*D)) / ((1/lambd) - ((1-exp(-lambd*D))/pow(lambd,2)*D))) * 1000000;
    Serial.println("delta_co2: "+String(delta_co2));
    return delta_co2;
}

void print_measurements()
{
    Serial.println("CO2: "+String(sys.co2));
    Serial.println("Temperatur: "+String(sys.temp));
    Serial.println("Luftfeuchte: "+String(sys.hum));
}

#if DETECTION_TYPE == OFFLINE
void print_class()
{
    if(sys.knn_class == 1)
        Serial.println("Lüftung Beginn");
    else if(sys.knn_class == 2)
        Serial.println("Volle Lüftung");
    else if(sys.knn_class == 3)
        Serial.println("Lüftung Ende");
}

void add_value_to_win(float *p, uint8_t ts_size, uint16_t val) // add value in timeseries
{
    for(uint8_t i=1; i<WINDOW_SIZE; i++) {
        p[i-1] = p[i];
    }
    p[WINDOW_SIZE-1] = (float)val;
}
#endif

uint8_t ventilation_active()
{
#if DETECTION_TYPE == OFFLINE
    return sys.knn_class != 3;
#else
    return sys.window_open;
#endif
}

void mqtt_send_measurements()
{
    mqttreconnect();
    mqttclient.publish(MEAS_CO2_TOPIC,String(sys.co2).c_str());
    mqttclient.publish(MEAS_TEM_TOPIC,String(sys.temp).c_str());
    mqttclient.publish(MEAS_HUM_TOPIC,String(sys.hum).c_str());
#if DETECTION_TYPE == OFFLINE
    mqttclient.publish("myroom/knn_class",String(sys.knn_class).c_str());
#endif
}

inline void requestTrainingState()
{
#if DETECTION_TYPE == OFFLINE
    sys.state = TRAINED;
#else
    mqttclient.publish(STATE_TOPIC,String("request training state").c_str());
#endif
}
