#!/usr/bin/python
import sys, serial, os, time, select, tty, termios, signal
usleep = lambda x: time.sleep(x/1000000.0);

class RpiCobaltLCD:
	buttonLine = ["1", "1", "1", "1", "1", "1", "1"]
	buttonKeyMap = ["x", "l", "u", "d", "r", "e", "s" ] 
	pretend = False
	connected = False
	sPort = object
	termfd_old = object
	termfd = 0
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
			self.termfd = sys.stdin.fileno()
			self.termfd_old = termios.tcgetattr(self.termfd)
			tty.setraw(sys.stdin.fileno())
		else:
			self.sPort.write("\n")
			usleep(1000000)
			conn_check = self.sPort.readline().rstrip()
			if(conn_check == "~CNCT"):
				self.sPort.write("~HLO\n");
				usleep(1000000)
				conn_check = self.sPort.readline().rstrip()
				if(conn_check == "~Connected"):
					self.connected = True
			if(conn_check == "~OK") :
				self.connected = True
	def disconnect(self):
		if(self.pretend) :
			self.connected = False
			termios.tcsetattr(self.termfd, termios.TCSADRAIN, self.termfd_old)
		else:
			self.sPort.write("~DIS\n")
			usleep(1000000)
			conn_reply = self.sPort.readline()
	def close(self):
		if(self.pretend) :
			self.connected = False
			termios.tcsetattr(self.termfd, termios.TCSADRAIN, self.termfd_old)

	def write(self, outstr):
		self.home()
		if(self.pretend):
			#sys.stdout.write("                \n\r|               \n\n\r")
			#self.home()
			line1 = outstr.rstrip()
			if((len(line1)-16) < 0):
				tmpLen=0
			else: 
				tmpLen=len(line1)-16
			line2 = "|" + line1[16:33] + (" " * (32 - tmpLen) )
			lcdout = line1[:16] + "\n\r" + line2[:17] + "\n\n\r"
			sys.stdout.write(lcdout)
		else:
			self.sPort.write(outstr.rstrip() + "\n")
			#sys.stdout.write("\n\r")
			#sys.stdout.write("\033[1A\r")
	def home(self):
		if(self.pretend):
			sys.stdout.write("\033[3A\r|")

	def authKey(self):
		self.write("Please enter    secret sequence.")
		tmpKeyLine=""
		tmpKeyIn=""
		while(tmpKeyIn != "e"):
			# read in the keys pressed.
			keymap_in=self.readButtons();
			if(keymap_in!=""): 
				tmpCounter=0
				for key in keymap_in:
					if(key=="0"):
						tmpKeyIn=self.buttonKeyMap[tmpCounter]
						tmpKeyLine += tmpKeyIn
						break
					tmpCounter +=1
		return tmpKeyLine
						


	def readButtons(self):
		if(self.pretend):
			tmpBtnLine = []
			for btn in self.buttonLine:
				tmpBtnLine.append(btn)
			while sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
				inb = sys.stdin.read(1);
				# pressing q in pretent mode will quit.
				if(inb == 'q'):
					os.kill(os.getpid(), signal.SIGINT)
				if(str(inb) != "" ):
					# map the key pressed to a value in the buttonLine.
					b_key = 0
					for	btn in self.buttonKeyMap:
						if(btn == inb) :
							if(tmpBtnLine[b_key] == "1"):
								tmpBtnLine[b_key] = "0";
							else: 
								tmpBtnLine[b_key] = "1"; 
						b_key += 1
				if(''.join(tmpBtnLine) != ''.join(self.buttonLine)):
					self.buttonLine = tmpBtnLine
					#time.sleep(1)
					return ''.join(self.buttonLine)
			else :
				return ""
		else:
			return self.sPort.readline();
