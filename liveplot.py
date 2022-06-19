import matplotlib.pyplot as plt
import numpy as np

# use ggplot style for more sophisticated visuals
plt.style.use('ggplot')

def live_plotter(x_vec: object, y1_data: object, line1: object, pause_time: object = 0.0001) -> object:
    if line1==[]:
        # this is the call to matplotlib that allows dynamic plotting
        plt.ion()
        fig = plt.figure(figsize=(10,7))
        
        ax1 = fig.add_subplot(111)
        # create a variable for the line so we can later update it
        #line1, = ax1.plot(x_vec,y1_data,'-o',alpha=0.5,linestyle='None')
        line1, = ax1.plot(x_vec,y1_data)
        

        #set y axis range of data
        ax1.set_ylim(np.min(y1_data),np.max(y1_data))  
        #ax3.set_prop_cycle(color=['red', 'green', 'blue'])
    
        #update plot label/title
        ax1.set_title('Real Time Temperature')

        
        fig.tight_layout()                
        plt.show()
    
    # after the figure, axis, and line are created, we only need to update the y-data    
    line1.set_ydata(y1_data)


    # adjust limits if new data goes beyond bounds
    if np.min(y1_data)<=line1.axes.get_ylim()[0] or np.max(y1_data)>=line1.axes.get_ylim()[1]:
        plt.ylim([np.min(y1_data)-np.std(y1_data),np.max(y1_data)+np.std(y1_data)])

    
    # this pauses the data so the figure/axis can catch up - the amount of pause can be altered above
    plt.pause(pause_time)
    
    # return line so we can update it again in the next iteration
    return line1


def live_close():
    plt.close()
