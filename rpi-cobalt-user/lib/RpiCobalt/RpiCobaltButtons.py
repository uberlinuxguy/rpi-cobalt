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
		if(buttonFunc != ""):
			buttonFunc = '{:13}'.format(buttonFunc)
		return buttonFunc

	def buttonAction(self,callStr):
		callStr = callStr.rstrip()
		if(callStr=="resetPress"):
			self.resetPress()
		elif(callStr=="resetRelease"):
			self.resetRelease()
		elif(callStr=="upPress"):
			self.upPress()
		elif(callStr=="upRelease"):
			self.upRelease()
		elif(callStr=="downPress"):
			self.downPress()
		elif(callStr=="downRelease"):
			self.downRelease()
		elif(callStr=="leftPress"):
			self.leftPress()
		elif(callStr=="leftRelease"):
			self.leftRelease()
		elif(callStr=="rightPress"):
			self.rightPress()
		elif(callStr=="rightRelease"):
			self.rightRelease()
		elif(callStr=="selectPress"):
			self.selectPress()
		elif(callStr=="selectRelease"):
			self.selectRelease()
		elif(callStr=="enterPress"):
			self.enterPress()
		elif(callStr=="enterRelease"):
			self.enterRelease()
		
	def testFunction(self):
		print "This is a test."
		return 0; 
	@classmethod
	def resetPress(self):
		#default resetPress Handler
		return 0;
	@classmethod
	def resetRelease(self):
		#default resetRelease handler
		return 0;
	@classmethod
	def upPress(self):
		#default resetPress Handler
		return 0;
	@classmethod
	def upRelease(self):
		#default resetRelease handler
		return 0;
	@classmethod
	def downPress(self):
		#default resetPress Handler
		return 0;
	@classmethod
	def downRelease(self):
		#default resetRelease handler
		return 0;
	@classmethod
	def leftPress(self):
		#default resetPress Handler
		return 0;
	@classmethod
	def leftRelease(self):
		#default resetRelease handler
		return 0;
	@classmethod
	def rightPress(self):
		#default resetPress Handler
		return 0;
	@classmethod
	def rightRelease(self):
		#default resetRelease handler
		return 0;
	@classmethod
	def selectPress(self):
		#default resetPress Handler
		return 0;
	@classmethod
	def selectRelease(self):
		#default resetRelease handler
		return 0;
	@classmethod
	def enterPress(self):
		#default resetPress Handler
		return 0;
	@classmethod
	def enterRelease(self):
		#default resetRelease handler
		return 0;

