# by Taapat <taapat@gmail.com> 16-01-2014

from time import localtime, strftime

from Components.ServiceEventTracker import ServiceEventTracker
from Plugins.Plugin import PluginDescriptor

from enigma import iPlayableService, eTimer, evfd 
from ServiceReference import ServiceReference

class VFDIcons:
	def __init__(self, session):
		self.session = session
		self.onClose = [ ]
		self.__event_tracker = ServiceEventTracker(screen = self,
			eventmap = {iPlayableService.evStart: self.CheckName})
		self.stringistime = False
		self.servicename = "    "
		self.time = "    "
		self.timer = eTimer()
		self.timer.callback.append(self.CheckTime)
		self.CheckTime()
		self.changetimer = eTimer()
		self.changetimer.callback.append(self.WriteStrig)
		self.changetimer.start(10000, False)

	def CheckName(self):
		self.changetimer.stop()
		self.stringistime = True
		service = self.session.nav.getCurrentlyPlayingServiceOrGroup()
		if service:
			if service.getPath():
				self.servicename = "PLAY"
			else:
				self.servicename = str(service.getChannelNum())
		else:
			self.servicename = "    "
		self.changetimer.start(10000, False)
		self.WriteStrig()

	def CheckTime(self):
		self.timer.stop()
		tm = localtime()
		self.time = strftime("%H%M", tm)
		self.timer.startLongTimer(60-tm.tm_sec)

	def WriteStrig(self):
		if self.stringistime:
			self.stringistime = False
			string = self.servicename
		else:
			self.stringistime = True
			string = self.time
		evfd.getInstance().vfd_write_string(string)
		

VFDIconsInstance = None

def main(session, **kwargs):
	global VFDIconsInstance
	if VFDIconsInstance is None:
		VFDIconsInstance = VFDIcons(session)
		VFDIconsInstance.WriteName()

def Plugins(**kwargs):
	return [PluginDescriptor(where = PluginDescriptor.WHERE_SESSIONSTART,
		fnc = main)]

