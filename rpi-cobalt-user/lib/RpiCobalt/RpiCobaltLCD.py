#!/usr/bin/python
import sys, serial, os, time, select, tty, termios
usleep = lambda x: time.sleep(x/1000000.0);

class RpiCobaltLCD:
	buttonLine = ["1", "1", "1", "1", "1", "1", "1"]
	buttonKeyMap = ["x", "l", "u", "d", "r", "e", "s" ] 
	pretend = False
	connected = False
	sPort = object
	def init(self, device, baud): 
		if(self.pretend) :
			print "------------------"
			print "|                |"
			print "|                |"
			print "------------------"
		else: 
			self.sPort = serial.Serial(device, baud, timeout=3)
			usleep(1000) # sleep for a short period to allow for device setup

	def connect(self):
		if(self.pretend):
			# in pretend mode, we are always "connected"
			self.connected = True
		else:
			self.sPort.write("\n")
			usleep(1000000)
			conn_check = self.sPort.readline().rstrip()
			print "Status: " + conn_check
			if(conn_check == "~CNCT"):
				self.sPort.write("~HLO\n");
				usleep(1000000)
				conn_check = self.sPort.readline().rstrip()
				if(conn_check == "~Connected"):
					self.connected = True
					print "Connected."
			if(conn_check == "~OK") :
				print "Connected." 
				self.connected = True
	def disconnect(self):
		if(self.pretend) :
			self.connected = False
		else:
			self.sPort.write("~DIS\n")
			usleep(1000000)
			conn_reply = self.sPort.readline()

	def write(self, outstr):
		self.home()
		if(self.pretend):
			line1 = outstr.rstrip()
			line2 = "|" + line1[16:33]
			lcdout = line1[:16] + "\n" + line2[:17] + "\n\n"
			sys.stdout.write(lcdout)
		else:
			self.sPort.write(outstr.rstrip() + "\n")
	def home(self):
		if(self.pretend):
			sys.stdout.write("\033[3A\r|")

	def readButtons(self):
		if(self.pretend):
			fd = sys.stdin.fileno()
			old_settings = termios.tcgetattr(fd)
			try:
				tty.setraw(sys.stdin.fileno())
				while sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
					inb = sys.stdin.read(1);
					if(str(inb) == "" ):
						time.sleep(3) #emulate the serial timeout.
						# no buttons pressed so clear buttonLine.
						termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
						self.buttonLine = ["1", "1", "1", "1", "1", "1", "1"]
						return ""
					else :
						# map the key pressed to a value in the buttonLine.
						b_key = 0
						for	btn in self.buttonKeyMap:
							if(btn == inb) :
								self.buttonLine[b_key] = 0;
							else: 
								self.buttonLine[b_key] = 1; 
							b_key += 1
						termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
						return self.buttonLine
				else:
					time.sleep(3)
					termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
					self.buttonLine = ["1", "1", "1", "1", "1", "1", "1"]
					return ""
			finally:
				termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
		else:
			return self.sPort.readline();
