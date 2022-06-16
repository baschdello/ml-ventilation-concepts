#!./bin/python
import dtw
import time
import os
import numpy as np
import glob
import matplotlib.pyplot as plt
import paho.mqtt.client as mqtt
from numpy.linalg import norm
from tslearn.barycenters import dtw_barycenter_averaging

mqtt_server        = "co2.fritz.box"
sensor_state_topic = "state/myroom"
sensor_cmd_topic   = "cmd/myroom"
sensor_co2_topic   = "meas/myroom/co2"

training_vent_size = 0
training_vents = {}
training_active = False
trained = True
window_open = False
curr_val = -1
vent_folder = "./last_ventilation/"
font = {
    'color':  'black',
    'weight': 'normal',
    'size': 16,
}

def on_connect(client, userdata, rc, properties=None):
    print("connected with result code "+str(rc))
    client.subscribe(sensor_state_topic)
    client.subscribe(sensor_co2_topic)

def on_message(client, userdata, msg):
    global training_active, training_vent_size, training_vents, curr_val, window_open, trained
    esp_msg = str(msg.payload, 'utf-8')
    if msg.topic == sensor_state_topic:
        if "start training " in esp_msg:
            training_vent_size = int(esp_msg.replace("start training ",""))
            training_vents = {}
            training_active = True
            trained = False
            window_open = False
            print("start training. collecting "+str(training_vent_size)+" ventilations")
        elif "window opened" in esp_msg:
            print("window opened")
            window_open = True
        elif "window closed" in esp_msg:
            print("window closed")
            window_open = False
        elif "request training state" in esp_msg:
            print("esp asked for training state")
            if trained:
                client.publish(sensor_cmd_topic,"trained")
            else:
                client.publish(sensor_cmd_topic,"untrained")
    elif msg.topic == sensor_co2_topic:
        curr_val = float(msg.payload)

def saveDBAimg(filename,trainings, center, str=""):
    plt.plot(trainings.transpose(), "k-", alpha=.2)
    plt.plot(center, "r-", linewidth=2)
    plt.title(str, fontdict=font)
    plt.savefig(filename)

def wait_for_co2():
    global curr_val, window_open
    curr_val = -1
    while curr_val == -1 and window_open:  # wait for new value
        time.sleep(0.1)
    return curr_val
    

if __name__ == '__main__':
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(mqtt_server, 1883, 60)
    client.loop_start()

    if os.path.isfile("center.csv") == False:
        trained = False
        print("dba center.csv not found. Please start training first!")
    if not os.path.exists("last_ventilation"):
        os.makedirs("last_ventilation")

    print("ready...")
    while 1:
        if training_active:   # training
            for i in range(training_vent_size): # collect some ventilations
                while window_open == False: # wait for open window
                    time.sleep(0.1)
                print("open window detected")
                print("collecting ventilation "+str(i))
                training_vents[i] = np.empty(0)
                while window_open:
                    co2 = wait_for_co2()
                    if co2 != -1:
                        training_vents[i] = np.append(training_vents[i],co2)  # append value
                        print("appended: "+str(co2))
                training_vents[i] = np.interp(training_vents[i], (training_vents[i].min(), training_vents[i].max()), (0, 1))
            training_active = False
            client.publish(sensor_cmd_topic,"trained")

            min_len = len(training_vents[0])
            for item in training_vents.items():  # search for minimum length
                if min_len > len(item[1]):
                    min_len = len(item[1])
            sized_trainings = np.zeros((0, min_len))
            for item in training_vents.items():
                sized_trainings = np.append(sized_trainings, [item[1][0:min_len]],axis=0)
            avg = dtw_barycenter_averaging(sized_trainings)

            # save trainings and dba center
            np.savetxt("training_data.csv",sized_trainings,fmt="%.3f", delimiter=",")
            np.savetxt("center.csv", avg,fmt="%.3f", delimiter=",")
            saveDBAimg("center.jpg", sized_trainings, avg) # save image of training data and dba center
            print("exit training")

        elif window_open:   # validate ventilation
            files = glob.glob("./last_ventilation/*.jpg")
            for file in files:
                os.remove(file)
            avg = np.genfromtxt('center.csv',delimiter=",") # load dba from csv
            co2_vent = np.empty(0)
            i = 0
            while window_open:
                j = i
                if len(avg) < j:
                    j = len(avg)
                print("waiting for co2")
                co2 = wait_for_co2()
                if co2 != -1:
                    co2_vent = np.append(co2_vent, co2)
                    co2_mapped = np.interp(co2_vent, (400, co2_vent[0]), (0,1))
                    if i>1:
                        metr = dtw.dtw(co2_mapped[:i],avg[:j],lambda x, y: norm(x - y))[0]
                        saveDBAimg(vent_folder+"step%04d"%i+".jpg",co2_mapped[:i].transpose(), avg[:j], "dtw: "+str("%.2f" % metr))
                        print("step "+str(i)+"  dtw: "+str(metr))
                        if metr > 10.0 and min(co2_vent) > 600:
                            client.publish(sensor_cmd_topic,"vent fault")
                i = i+1
        time.sleep(0.1)
