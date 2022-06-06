# ML-ventilation-concepts
This repository contains scripts and measurement data for machine learning based ventilation classification.

## How to use DBA-DTW example
Folder *dba-dtw-example* contains examples to show a DBA-DTW based ventilation classification approach without the usage of special hardware like ESP8266, CO2 sensors, reed sensors.

### First step - create a barycenter ventilation
Create a python venv, install necessary modules. After that use the python script in folder *dba-dtw-example/dba-training/*:
```
./dba_center_ventilation.py training_ventilations.csv barycenter_output
```
This script outputs a jpg file which shows training ventilations in grey (inputs) and the generated barycenter in red (output). The generated barycenter is also in the csv file 'barycenter_output.csv' which we need in next step.
You will get further help with parameter *-h*.

### Second step - get a statement about the quality of ventilation
Create a python venv, install necessary modules. After that use the python script in folder *dba-dtw-example/dtw-test/*:
```
./dtw_test.py ../dba-training/barycenter_output.csv window_opened_ventilation.csv
```
This script uses values of ventilation timeseries, e.g. *window_opened_ventilation.csv* and will determine the DTW score step by step (in the embedded version, a new CO2 value will arrive every 5 seconds). For better traceability, each individual step is stored in a jpg file in the folder *last_ventilation*. If the determined DTW score is above 10 and our previous CO2 levels are above 600 ppm, the ventilation is significantly worse than our sample ventilation specified by the barycenter. This will be the case if using example *window_tilted_ventilation.csv* which contains a ventilation with tilted window. The script will output a message that the ventilation is poor.
You will get further help with parameter *-h*.

## Embedded version
The embedded version requires a mqtt broker, IoT Kit Octopus, Arduino IDE, CO2 sensor. If using the online approach (DBA and DTW) you also need a reed sensor on window to get a accurate detection of the ventilation. The online approach also needs a cloud computer to run the python script *dba_dtw_ventilation.py* 
For offline approach the ventilation detection is based on CO2 sensor measurements (lower accuracy).
A knn model for pattern matching of the 3 ventilation classes is predefined in the file *knn.c*.
Note defines in file *config.h*. You can change the approach here with define *DETECTION_TYPE*. Choose between *ONLINE* (DBA and DTW) and *OFFLINE* (KMEANS and KNN).

## Sensor data
In csv file *measurements.csv* you will find some measurements with corresponding the timestamp. They contains CO2 value and temperatures (inner and outer). During measurements many ventilations were done. Because of missing wind sensor it was not possible to measure wind speeds. Maybe the data is also helpful for other projects.
