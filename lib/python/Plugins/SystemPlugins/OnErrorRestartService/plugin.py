from Components.Console import Console
from Plugins.Plugin import PluginDescriptor
from enigma import eTimer

class OnErrorRestart:
	def __init__(self, session):
		self.Console = Console()
		self.session = session
		self.error = False
		self.timercount = 0
		self.Timer = eTimer()
		self.Timer.callback.append(self.checklog)

	def StartLongTimer(self):
		print "[OnErrorRestartService] start long timer"
		self.Timer.start(20000, False)

	def StartFastTimer(self):
		print "[OnErrorRestartService] check error, start fast timer"
		self.Timer.start(3000, False)

	def checklog(self):
		self.Console.ePopen('dmesg -c | grep "Error-pti_task"' , self.checkerror)

	def checkerror(self, result, retval, extra_args):
		if result.strip():
			if self.error:
				print "[OnErrorRestartService] restart service"
				self.error = False
				service = self.session.nav.getCurrentlyPlayingServiceOrGroup()
				if service:
					self.session.nav.stopService()
					self.session.nav.playService(service)
				self.ResetTimer()
			else:
				self.error = True
				self.Timer.stop()
				self.timercount += 1
				self.StartFastTimer()
		elif self.timercount > 0:
			if self.timercount < 11:
				self.timercount += 1
			else:
				self.ResetTimer()

	def ResetTimer(self):
		self.timercount = 0
		self.error = False
		self.Timer.stop()
		self.StartLongTimer()

ErrorCheckInstance = None

def main(session, **kwargs):
	global ErrorCheckInstance
	if ErrorCheckInstance is None:
		try:
			f = open("/proc/sys/kernel/printk", "r")
			for line in f:
				if int(line[:1]) > 0:
					ErrorCheckInstance = OnErrorRestart(session)
					ErrorCheckInstance.StartLongTimer()
			f.close()
		except:
			pass

def Plugins(**kwargs):
	return [PluginDescriptor(where = PluginDescriptor.WHERE_SESSIONSTART, fnc=main)]

