#Building Instructions

This document will tell you the best way to build this module on a Raspberry pi.

**NOTE:** This module is ONLY needed for the status LEDS on the Cobalt RaQ.  The Qube does not have
any status lights.  Feel free to add them and use them via this module if you like, but this module
is **NOT** needed for a Cobalt Qube implementation.

###Pre-requisits
- Raspbian or equivelant installed
- At least 1G of free space on the root filesystem
- root access to the system (or sudo)
- A working development environment on the RPi.

###Preparation
1. Make sure you run rpi-update to update to the latest kernel
2. Go get the rpi-source script to download your kernel. (https://github.com/notro/rpi-source/wiki)
3. Follow the instructions on there to download the sources for the latest kernel

Once you have the latest kernel sources, you are ready to begin.

###Building
1. Grab a copy of the latest rpi-cobalt repository
```bash
# git clone https://github.com/uberlinuxguy/rpi-cobalt
```
2. Go into the rpi-cobalt-kmod directory
```bash
# cd rpi-cobalt-kmod/
```
3. Run make to build the kernel module.
```bash
# make
```
4. Run make modules_install to install the kernel module.
```bash
# sudo make modules_install
```
5. Run depmod to rebuild the module deps.
```bash
# sudo depmod -a
```
6. Attempt to load the module
```bash
# sudo modprobe rpi-cobalt
```

If all goes well, you should not get any errors and if your circuit is wired right, you should see der blinken lights on your RaQ.

