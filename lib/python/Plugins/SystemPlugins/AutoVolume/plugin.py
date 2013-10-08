from Components.config import config
from Components.ServiceEventTracker import ServiceEventTracker
from Plugins.Plugin import PluginDescriptor

from enigma import iPlayableService, eDVBVolumecontrol

class AutoVolume:
	def __init__(self, session):
		self.session = session
		self.onClose = [ ]
		self.volumeUp = False
		self.newService = True
		self.volctrl = eDVBVolumecontrol.getInstance()
		self.volumeValue = self.volctrl.getVolume()
		self.__event_tracker = ServiceEventTracker(screen = self, eventmap =
			{
				iPlayableService.evUpdatedInfo: self.updateInfo,
				iPlayableService.evStart: self.__evStart
			})

	def __evStart(self):
		self.newService = True

	def updateInfo(self):
		if self.newService:
			self.newService = False
			if self.isCurrentAudioAC3DTS():
				if not self.volumeUp:
					self.volumeUp = True
					self.volumeValue = self.volctrl.getVolume()
					vol = self.volumeValue + 40
					if vol > 100:
						vol = 100
					self.setVolume(vol)
			elif self.volumeUp:
				self.volumeUp = False
				self.setVolume(self.volumeValue)

	def enigmaStop(self):
		if self.volumeUp:
			self.setVolume(self.volumeValue)

	def isCurrentAudioAC3DTS(self):
		service = self.session.nav.getCurrentService()
		audio = service and service.audioTracks()
		if audio:
			try:
				tracknr = audio.getCurrentTrack()
				i = audio.getTrackInfo(tracknr)
				description = i.getDescription() or "";
				if "AC3" in description or "DTS" in description:
					return True
			except:
				return False
		return False

	def setVolume(self, value):
		self.volctrl.setVolume(value, value)
		config.audio.volume.value = self.volctrl.getVolume()
		config.audio.volume.save()

AutoVolumeInstance = None

def main(session, **kwargs):
	global AutoVolumeInstance
	if AutoVolumeInstance is None:
		AutoVolumeInstance = AutoVolume(session)
		AutoVolumeInstance.updateInfo()

def autoend(reason, **kwargs):
	if reason == 1:
		AutoVolumeInstance.enigmaStop()

def Plugins(**kwargs):
	return [PluginDescriptor(where = PluginDescriptor.WHERE_SESSIONSTART, fnc = main),
		PluginDescriptor(where = [PluginDescriptor.WHERE_AUTOSTART], fnc = autoend)]
