# by Taapat <taapat@gmail.com> 08-10-2013

from Components.ServiceEventTracker import ServiceEventTracker
from Plugins.Plugin import PluginDescriptor

from enigma import iPlayableService, evfd
from ServiceReference import ServiceReference

class VFDIcons:
	def __init__(self, session):
		self.session = session
		self.onClose = [ ]
		self.__event_tracker = ServiceEventTracker(screen = self,
			eventmap = {iPlayableService.evStart: self.WriteName})

	def WriteName(self):
		service = self.session.nav.getCurrentlyPlayingServiceOrGroup()
		if service:
			if service.getPath():
				servicename = "PLAY"
			else:
				servicename = str(service.getChannelNum())
		else:
			servicename = "    "
		evfd.getInstance().vfd_write_string(servicename)

VFDIconsInstance = None

def main(session, **kwargs):
	global VFDIconsInstance
	if VFDIconsInstance is None:
		VFDIconsInstance = VFDIcons(session)
		VFDIconsInstance.WriteName()

def Plugins(**kwargs):
	return [PluginDescriptor(where = PluginDescriptor.WHERE_SESSIONSTART,
		fnc = main)]

