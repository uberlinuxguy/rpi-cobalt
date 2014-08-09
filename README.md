Rpi-Cobalt

This project aims at creating software and a kernel module to interface a 
raspberry pi to the old cobalt chassis front panel.  Features include:

- working LCD support
- working button action support
- working kernel level front panel LEDs.

The hardware you will need includes:

- an old Cobalt RaQ case (Qube support is to follow)
- a raspberry pi
- an arduiono mega (or something with a good amount of GPIO pins)
- A logic leveler for the RPi <---> Arduino Serial 
  (if you are running a 5V Arduino, like the Mega 2560 that I have.)


Experience to set up:

- experience running raspbian on a raspbery pi
- experience with arduino sketches.
- python and c experience is a plus
- simple hardware hacking skills ( wiring and such )

For the more advanced amoung you, if you want to make your own interface 
boards, you can probably do that too.  I plan on making one for the 
Cobalt Front Panel Cable, Arduino and RPi once I have all the software,
cabling, and kernel stuff sorted out.  A board layout may show up in this 
repository once it's ready.
