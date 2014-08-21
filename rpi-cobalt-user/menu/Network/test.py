#!/usr/bin/python
#Set IP Config
import sys, time, subprocess, getopt, signal, os
sys.path.append("./lib")
from RpiCobalt.RpiCobaltButtons import RpiCobaltButtons
from RpiCobalt.RpiCobaltLCD import RpiCobaltLCD

# local class to implement buttons.
class rpiNetworkBtns(RpiCobaltButtons):
	rclcd=object()
	valueMode=True;
	ipLabels = [('ip',		"   IP Address   "),
				('mask',		"    Net Mask    "),
				('gateway',	"     Gateway    "),
				('pridns',	"   Primary DNS  ")
				]
	ipRegisters = {	'ip': "",
					'netmask': "",
					'gateway': "",
					'pridns': ""
				  }
	currRegister = 'ip'
	currValue=[0,0,0,0,0,0,0,0,0,0,0,0]
	currValPos=0
	savePrompt1="Save? Press Key:"
	savePrompt2="(S)Save, (E)Exit"


	def selectPress(self):
		return 0

	def selectRelease(self):
		if(self.valueMode):
			return 0
		else:
			# this is save mode
			self.doSave()
			sys.exit(0)
		return 0

	def upRelease(self):
		if(self.valueMode):
			tmpVal = int(self.currValue[self.currValPos]) + 1
			if((self.currValPos == 0) or (self.currValPos == 3) or (self.currValPos == 6) or (self.currValPos == 9)):
				if(tmpVal > 2) :
					tmpVal = 0
			else:
				# holy crap... ip address rules.
				if(((self.currValPos == 1 or self.currValPos == 2 ) and self.currValue[0] == 2) or
					((self.currValPos == 4 or self.currValPos == 5 ) and self.currValue[3] == 2) or
					((self.currValPos == 7 or self.currValPos == 8 ) and self.currValue[6] == 2) or
					((self.currValPos == 10 or self.currValPos == 11 ) and self.currValue[9] == 2)):
					if(tmpVal > 5):
						tmpVal = 0
				else:
					if(tmpVal > 9):		
						tmpVal = 0;

			self.currValue[self.currValPos]=tmpVal
		return 0

	def downRelease(self):
		if(self.valueMode):
			tmpVal = int(self.currValue[self.currValPos]) - 1
			if((self.currValPos == 0) or (self.currValPos == 3) or (self.currValPos == 6) or (self.currValPos == 9)):
				if(tmpVal < 0) :
					tmpVal = 2
			else:
				if(tmpVal < 0):		
					# holy crap... ip address rules.
					if(((self.currValPos == 1 or self.currValPos == 2 ) and self.currValue[0] == 2) or
						((self.currValPos == 4 or self.currValPos == 5 ) and self.currValue[3] == 2) or
						((self.currValPos == 7 or self.currValPos == 8 ) and self.currValue[6] == 2) or
						((self.currValPos == 10 or self.currValPos == 11 ) and self.currValue[9] == 2)):
							tmpVal = 5;
					else:
						tmpVal = 9;

			self.currValue[self.currValPos]=tmpVal
		return 0

	def leftRelease(self):
		if(self.valueMode):
			self.currValPos=self.currValPos - 1
			if(self.currValPos < 0):
				self.currValPos = len(self.currValue) -1

		return 0

	def rightRelease(self):
		if(self.valueMode):
			self.currValPos=self.currValPos + 1
			if(self.currValPos >= len(self.currValue)):
				self.currValPos = 0
		
		return 0
	
	def enterRelease(self):
		if(self.valueMode):
			self.ipRegisters[self.ipLabels[0][0]]=''.join(str(v) for v in self.currValue)
			self.currValue = [0,0,0,0,0,0,0,0,0,0,0,0]
			self.ipLabels.pop(0)
			if(len(self.ipLabels) > 0) :
				self.currRegister = self.ipLabels[0][0]
			else:
				self.displaySave()
		else:
			# time to exit without save.
			sys.exit(0)

	def displayPrompt(self):
		if(self.valueMode):
			tmpStr = ''.join(str(v) for v in self.currValue[0:3]) + "."
			tmpStr += ''.join(str(v) for v in self.currValue[3:6]) + "."
			tmpStr += ''.join(str(v) for v in self.currValue[6:9]) + "."
			tmpStr += ''.join(str(v) for v in self.currValue[9:12]) 

			self.rclcd.write(self.ipLabels[0][1] + tmpStr)
			
	
	def doSave(self):
		return 0
	
	def displaySave(self):
		self.valueMode=False
		self.rclcd.write(self.savePrompt1 + self.savePrompt2)
		return 0;


def signal_handler(signal, frame):
	rclcd.close()
	#print('LCD Daemon Exit.')
	sys.exit(0)

pretend = False
def parseArgs(argv):
	global pretend
	try:
		opts, args = getopt.getopt(argv, "hp",["help", "pretend"])
	except getopt.GetoptError:
		print sys.argv[0] + ": [-h | --help] [-p | --pretend]\n\n"
		sys.exit(2)
	for opt, arg in opts:
		if(opt in ("-h", "--help")):
			print sys.argv[0] + ": [-h | --help] [-p | --pretend]\n\n"
			sys.exit()
		elif(opt in ("-p", "--pretend")):
			pretend=True


if __name__ == "__main__":
	parseArgs(sys.argv[1:])
	
signal.signal(signal.SIGINT, signal_handler)

rcbtn = rpiNetworkBtns()

rclcd = RpiCobaltLCD()
rcbtn.rclcd = rclcd
rclcd.pretend = pretend
rclcd.init("/dev/ttyAMA0", "115200")
rclcd.connect()

time.sleep(1)
counter=0
button_line = "";
menuTimer=0
authed=False
while(rclcd.connected) :
	
	
	if(not authed):
		rclcd.write("Setup IP Config")
		time.sleep(1)
		tmpKey = rclcd.authKey()
		with open("./protected_key") as f_keyfile:
			secretKey = f_keyfile.readlines()[0].rstrip()
		if(tmpKey == secretKey): 
			authed=True
		else:
			rclcd.write("Secret Key      FAILED!!")
			time.sleep(3)
			sys.exit(0)
		rclcd.write("Auth Success")
	else:
		
		# button read from the LCD lib
		# and send it to the button lib for processing.
		tmpFuncStr=rcbtn.parseButtons(rclcd.readButtons())
		if(tmpFuncStr != ""):
			rcbtn.buttonAction(tmpFuncStr)
		rcbtn.displayPrompt()
