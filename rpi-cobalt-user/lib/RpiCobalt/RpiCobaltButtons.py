#!/usr/bin/python

class RpiCobaltButtons:
	buttonLine=["1", "1", "1", "1", "1", "1", "1"]
	buttonMap = ["reset", "left", "up", "down", "right", "enter", "select"]
	def parseButtons(self, btnLine):
		btnLine = btnLine[:7]
		buttonFunc = ""
		if(btnLine != self.buttonLine) :
			l_cntr=0
			for btn in btnLine:
				if(self.buttonLine[l_cntr] != btn):
					if(btn == "0") :
						buttonFunc = self.buttonMap[l_cntr] + "Press"
					else :
						buttonFunc = self.buttonMap[l_cntr] + "Release"
					self.buttonLine[l_cntr] = btn
				l_cntr += 1
		return buttonFunc
		
	def testFunction(self):
		print "This is a test."
		return 0; 
	def resetPress(self):
		#default resetPress Handler
		return 0;
	def resetRelease(self):
		#default resetRelease handler
		return 0;
	def upPress(self):
		#default resetPress Handler
		return 0;
	def upRelease(self):
		#default resetRelease handler
		return 0;
	def downPress(self):
		#default resetPress Handler
		return 0;
	def downRelease(self):
		#default resetRelease handler
		return 0;
	def leftPress(self):
		#default resetPress Handler
		return 0;
	def leftRelease(self):
		#default resetRelease handler
		return 0;
	def rightPress(self):
		#default resetPress Handler
		return 0;
	def rightRelease(self):
		#default resetRelease handler
		return 0;
	def selectPress(self):
		#default resetPress Handler
		return 0;
	def selectRelease(self):
		#default resetRelease handler
		return 0;
	def enterPress(self):
		#default resetPress Handler
		return 0;
	def enterRelease(self):
		#default resetRelease handler
		return 0;

