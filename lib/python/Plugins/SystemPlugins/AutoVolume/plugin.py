from Components.config import config
from Components.ServiceEventTracker import ServiceEventTracker
from Plugins.Plugin import PluginDescriptor
from enigma import iPlayableService, eDVBVolumecontrol

class AutoVolume:
	def __init__(self, session):
		self.session = session
		self.onClose = [ ]
		self.pluginStarted = False
		self.volumeUp = False
		self.newService = False
		self.volctrl = eDVBVolumecontrol.getInstance()
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
					if self.pluginStarted:
						vol = self.volctrl.getVolume()
						vol += 40
						if vol > 100:
							vol = 100
						self.setVolume(vol)
					else:
						self.pluginStarted = True
			elif self.volumeUp:
				self.volumeUp = False
				vol = self.volctrl.getVolume()
				vol -= 40
				if vol < 1:
					vol = 1
				self.setVolume(vol)
			elif not self.pluginStarted:
				self.pluginStarted = True

	def isCurrentAudioAC3DTS(self):
		service = self.session.nav.getCurrentService()
		audio = service and service.audioTracks()
		if audio:
			try:
				tracknr = audio.getCurrentTrack()
				i = audio.getTrackInfo(tracknr)
				description = i.getDescription() or ""
				if "AC3" in description or "DTS" in description:
					return True
			except:
				return False
		return False

	def setVolume(self, volume):
		self.volctrl.setVolume(volume, volume)
		config.audio.volume.value = volume
		config.audio.volume.save()

AutoVolumeInstance = None

def main(session, **kwargs):
	global AutoVolumeInstance
	if AutoVolumeInstance is None:
		AutoVolumeInstance = AutoVolume(session)
		AutoVolumeInstance.updateInfo()

def Plugins(**kwargs):
	return [PluginDescriptor(where = PluginDescriptor.WHERE_SESSIONSTART, fnc = main)]
		
