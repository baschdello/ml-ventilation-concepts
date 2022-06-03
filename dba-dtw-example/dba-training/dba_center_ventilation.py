#!./bin/python3
import numpy as np
import matplotlib.pyplot as plt
from tslearn.barycenters import dtw_barycenter_averaging

# load ventilations of training phase
train = np.genfromtxt('training_ventilations.csv',delimiter=',').transpose()

# convert training ventilations to value range 0 and 1
for i in range(len(train[:,1])):
    train[i] = np.interp(train[i], (train[i].min(), train[i].max()), (0, 1))

# generate barycenter
avg = dtw_barycenter_averaging(train)

# generate plot and save
plt.plot(train.transpose(), "k-", alpha=.2)
plt.plot(avg, "r-", linewidth=2)
plt.savefig("barycenter-output.jpg")

# save barycenter as csv
np.savetxt("barycenter-output.csv", avg,fmt="%.3f", delimiter=",")
