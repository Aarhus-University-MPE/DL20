#%%
from datetime import time
from matplotlib import colors
import matplotlib.pyplot as plt
import matplotlib.cbook as cbook

import numpy as np
import pandas as pd
import os

plt.rcParams.update({'font.size': 20})

def ConvertISD4000():
    for idy, line in enumerate(msft.iloc[:,column]):
        ISHPR = line.split(",")
        msft.iloc[idy, column] = float(ISHPR[5])

def set_size(w,h, ax=None):
    """ w, h: width, height in inches """
    if not ax: ax=plt.gca()
    l = ax.figure.subplotpars.left
    r = ax.figure.subplotpars.right
    t = ax.figure.subplotpars.top
    b = ax.figure.subplotpars.bottom
    figw = float(w)/(r-l)
    figh = float(h)/(t-b)
    ax.figure.set_size_inches(figw, figh)

#%%
folder = 'C:\\Users\\au540322\\Documents\\Projects\\Drill-Logger\\Data\\Calibration\\'
fileList = [name for name in os.listdir(folder) if os.path.isfile(os.path.join(folder, name))]

#%%
windowSize =[100, 30, 100, 100, 100, 100]
dataColumn = [3]

ax = plt.axes()
ax.set_title(str("Thermistor Data"),fontsize = 30)
ax.set_ylabel('Digital Value', fontsize = 25)

#ax.set_title(str("ISM4000 Temperature Values"),fontsize = 30)
#ax.set_ylabel('Temperature [°C]', fontsize = 25)
#ax.set_xlabel('Sample Nr', fontsize = 25)

ax.minorticks_on()
ax.grid(True)
ax.grid(True,which='minor',axis='x')

set_size(15,5)

for idx, file in enumerate(fileList):
    file_location = folder + file
    msft = pd.read_csv(file_location,delimiter='\t',decimal='.')

    for column in dataColumn:
        if(column == 6 or column == 7):
            ConvertISD4000()
        
        msft.iloc[:,column] = msft.iloc[:,column] * 0.00007629394 * 2
        rolling_windows = msft.iloc[:,column].rolling(windowSize[idx], min_periods=1)
        mus = rolling_windows.mean()
        medians = rolling_windows.median()
        sigmas = rolling_windows.std()

        mu = msft.iloc[:,column].mean()
        median = msft.iloc[:,column].median()
        sigma = msft.iloc[:,column].std()


        ax.plot(range(mus.__len__()),mus,linestyle='dashed')
        
        ax.fill_between(range(msft.__len__()) + msft.iloc[0,0],mus-sigmas,mus+sigmas,alpha=.2)

ax.legend(fileList, fontsize=25)

        

#%%
fig = ax[0].get_figure()

fig.savefig('figure.pdf')
# %%
#%%
msft.dtypes
#iloc[:,0].to_datetime
        

textstr = '\n'.join((
    r'$\mu=%.2f$' % (mu, ),
    r'$\mathrm{median}=%.2f$' % (median, ),
    r'$\sigma=%.2f$' % (sigma, )))

# these are matplotlib.patch.Patch properties
props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)

# place a text box in upper left in axes coords
#axes.text(0.95, 0.95, textstr, transform=axes.transAxes, fontsize=14,
#        verticalalignment='top', bbox=props, horizontalalignment = 'right', size = 25)