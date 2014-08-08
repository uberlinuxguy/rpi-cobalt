obj-m += rpi-cobalt.o 

all:
	make -C $(HOME)/linux M=$(PWD) modules

clean:
	make -C $(HOME)/linux M=$(PWD) clean

modules_install:
	make -C $(HOME)/linux M=$(PWD) modules_install
