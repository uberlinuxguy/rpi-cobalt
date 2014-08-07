obj-m += rpi-cobalt.o ledblink.o

all:
	make -C $(HOME)/linux M=$(PWD) modules

clean:
	make -C $(HOME)/linux M=$(PWD) clean
