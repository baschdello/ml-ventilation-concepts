#!./bin/python3
import os
import sys
import glob
import argparse
import dtw
import numpy as np
import matplotlib.pyplot as plt
from numpy.linalg import norm

vent_folder = "./last_ventilation/"
font = {
    'color':  'black',
    'weight': 'normal',
    'size': 16,
}


def saveDBAimg(filename,trainings, center, str=""):
    plt.plot(trainings.transpose(), "k-", alpha=.2)
    plt.plot(center, "r-", linewidth=2)
    plt.title(str, fontdict=font)
    plt.savefig(filename)

def main(argv):
    # parse arguments
    args = parseArguments(argv)
    barycenter_csv_file = args.barycenter_csv
    testventilation_csv_file = args.testventilation_csv

    if not os.path.exists(vent_folder):
        os.makedirs(vent_folder)
    else:
        files = glob.glob(vent_folder+"*.jpg")
        for file in files:
            os.remove(file)
    
    avg         = np.genfromtxt(barycenter_csv_file, delimiter=",") # load dba barycenter from csv
    vent        = np.genfromtxt(testventilation_csv_file, delimiter=",", usecols=(1), skip_header=1) # load test ventilation
    vent_mapped = np.interp(vent, (400, vent[0]), (0, 1)) # map test ventilation to range 0 and 1, using 400ppm as and value (calibrated CO2 sensor important)

    for i in range(1,len(vent_mapped)):
        score = dtw.dtw(vent_mapped[:i],avg[:i],lambda x, y: norm(x - y))[0]
        saveDBAimg(vent_folder+"step%04d"%i+".jpg",vent_mapped[:i].transpose(), avg[:i], "dtw: "+str("%.2f" % score))
        print("step "+str(i)+"  dtw: "+str("%.2f" % score))
        if score > 10.0 and min(vent[:i]) > 600:
            print("ventilation bad")
            exit(1)
    print("ventilation ok")
    exit(0)

def parseArguments(argv):
    parser = argparse.ArgumentParser(description='Demonstate the usage of DTW to evaluate ventilations')
    parser.add_argument('barycenter_csv',  action='store', help='barycenter csv file generated with DBA')
    parser.add_argument('testventilation_csv',  action='store', help='test ventilation csv file')
    return parser.parse_args(argv)

if __name__ == "__main__":
    main(sys.argv[1:])
