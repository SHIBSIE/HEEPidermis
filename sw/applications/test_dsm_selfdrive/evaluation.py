#In[]:
# Check the results

%matplotlib widget
import numpy as np
import matplotlib.pyplot as plt
import re

header = open("output.txt").readline().strip()
x, y = np.loadtxt("output.txt", delimiter="\t", skiprows=1, unpack=True)

plt.figure()
plt.title(header)
plt.plot(x, y, marker="o")
# plt.scatter(x, y)
plt.xlabel("x")
plt.ylabel("y")
plt.show()

#In[]

length_n = len(y)
p2          = np.floor(np.log2(length_n))

y_chopped   = y[- int(2**p2):]
x_chopped   = x[- int(2**p2):]

plt.figure()
plt.plot(x_chopped,y_chopped)
plt.show()

m               = re.search(r"fclk:(\d+)\s*Hz.*DF:(\d+)", header)
f_clk_Hz        = int(m.group(1))
df              = int(m.group(2))

real_f_clk_Hz   = 1e6
fs_Hz           = real_f_clk_Hz/df

N = len(y_chopped)
Y = np.fft.fft(y_chopped)
f = np.fft.fftfreq(N, d=1/fs_Hz)

plt.figure()
plt.plot(f[:N//2], np.abs(Y[:N//2]))
plt.xlabel("Hz")
plt.ylabel("|FFT|")
plt.xscale('log')
plt.yscale('log')
plt.show()