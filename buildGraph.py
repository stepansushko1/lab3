import matplotlib.pyplot as plt
import numpy as np

def getData(fileName):
    
    with open(fileName, 'r') as file:
        pass
    
    return timeList


def visualizeData(timeList):
    
    x = list(range(1, len(timeList)+1))
    plt.plot(x, timeList)
    plt.xlabel("Threads")
    plt.ylabel("T, Seconds")
    
    ax1 = plt.subplot()
    ax1.set_xticks(x)
    ax1.set_xticklabels(x)
    plt.show()
    
visualizeData([150, 100, 86, 67, 75])