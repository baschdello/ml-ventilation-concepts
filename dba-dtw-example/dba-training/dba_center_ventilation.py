#!./bin/python3
import sys
import argparse
import numpy as np
import matplotlib.pyplot as plt
from tslearn.barycenters import dtw_barycenter_averaging

def main(argv):
    # parse arguments
    args = parseArguments(argv)
    training_ventilation_csv_file = args.training_ventilations_csv
    barycenter_output_filename = args.barycenter_output_filename

    # load ventilations of training phase
    train = np.genfromtxt(training_ventilation_csv_file, delimiter=',').transpose()

    # convert training ventilations to value range 0 and 1
    for i in range(len(train[:,1])):
        train[i] = np.interp(train[i], (train[i].min(), train[i].max()), (0, 1))

    # generate barycenter
    avg = dtw_barycenter_averaging(train)

    # generate plot and save
    plt.plot(train.transpose(), "k-", alpha=.2)
    plt.plot(avg, "r-", linewidth=2)
    plt.savefig(barycenter_output_filename+".jpg")

    # save barycenter as csv
    np.savetxt(barycenter_output_filename+".csv", avg,fmt="%.3f", delimiter=",")

def parseArguments(argv):
    parser = argparse.ArgumentParser(description='Demonstate the usage of DBA to generate barycenter of training ventilations')
    parser.add_argument('training_ventilations_csv',  action='store', help='csv file with training ventilations')
    parser.add_argument('barycenter_output_filename',  action='store', help='filename for barycenter output files')
    return parser.parse_args(argv)

if __name__ == "__main__":
    main(sys.argv[1:])
