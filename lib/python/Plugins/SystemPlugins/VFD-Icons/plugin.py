# -*- coding: utf-8 -*-
#
# by Taapat <taapat@gmail.com> 25-08-2012
# review by <alexeytech@gmail.com> aka technic

from Plugins.Plugin import PluginDescriptor
from enigma import iPlayableService, eTimer, iServiceInformation, evfd
from time import localtime, strftime
from Components.ActionMap import ActionMap
from Components.ServiceEventTracker import ServiceEventTracker
from Components.config import config, ConfigSubsection, getConfigListEntry, ConfigSelection
from Components.ConfigList import ConfigListScreen
from Components.Sources.StaticText import StaticText
from Screens.Screen import Screen
from ServiceReference import ServiceReference

config.vfdicon = ConfigSubsection()
config.vfdicon.displayshow = ConfigSelection(default = "channel", choices = [
		("channel", _("channel")), ("channel number", _("channel number")), ("clock", _("clock")), ("blank", _("blank")) ])
displayshow = config.vfdicon.displayshow

class ConfigVFDDisplay(Screen, ConfigListScreen):

	def __init__(self, session):
		Screen.__init__(self, session)
		self.skinName = ["Setup"]
		self.setTitle(_("VFD display configuration"))
		self["key_red"] = StaticText(_("Cancel"))
		self["key_green"] = StaticText(_("OK"))
		self["actions"] = ActionMap(["SetupActions", "ColorActions"],
			{
				"cancel": self.cancel,
				"ok": self.keySave,
				"green": self.keySave,
				"red": self.cancel,
			}, -2)
		cfglist = []
		cfglist.append(getConfigListEntry(_("Show on VFD Display"), displayshow))
		ConfigListScreen.__init__(self, cfglist)

	def cancel(self):
		self.showVFD()
		ConfigListScreen.keyCancel(self)

	def keySave(self):
		self.showVFD()
		ConfigListScreen.keySave(self)

	def showVFD(self):
		main(self)
		if config.vfdicon.displayshow.value != "clock":
			VFDIconsInstance.WriteName()

def opencfg(session, **kwargs):
		session.open(ConfigVFDDisplay)
		evfd.getInstance().vfd_write_string( "VFD SETUP" )

def VFDdisplay(menuid, **kwargs):
	if menuid == "system":
		return [("VFD Display", opencfg, "vfd_display", 44)]
	else:
		return []

class VFDIcons:
	def __init__(self, session):
		self.session = session
		self.timer = eTimer()
		self.timer.callback.append(self.timerEvent)
		self.onClose = [ ]
		self.__event_tracker = ServiceEventTracker(screen=self,eventmap=
			{
				iPlayableService.evUpdatedInfo: self.__evUpdatedInfo,
			})

	def __evUpdatedInfo(self):
		self.WriteName()

	def WriteName(self):
		if config.vfdicon.displayshow.value == "clock":
			return
		service = self.session.nav.getCurrentService()
		if service is not None:
			print "[VFD Display] write name to VFD"
			# show blank
			servicename = "    "
			audio = service.audioTracks()
			if audio: # show the mp3 tag
				n = audio.getNumberOfTracks()
				for x in range(n):
					i = audio.getTrackInfo(x)
					description = i.getDescription();
					if description.find("MP3") != -1:
							servicename = service.info().getInfoString(iServiceInformation.sTagTitle)
					elif config.vfdicon.displayshow.value == "channel number":
						try: # show the service channel number
							servicename = str(self.session.nav.getCurrentlyPlayingServiceReference(False).getChannelNum())
						except:
							servicename = "    "
							print "[VFD Display] ERROR set channel number"
					elif config.vfdicon.displayshow.value == "channel":
						try: # show the service name
							servicename = ServiceReference(self.session.nav.getCurrentlyPlayingServiceReference(False)).getServiceName()
						except:
							servicename = "    "
							print "[VFD Display] ERROR set service name"
			print "[VFD Display] text ", servicename[0:20]
			evfd.getInstance().vfd_write_string(servicename[0:20])
			return 1

	def timerEvent(self):
		if config.vfdicon.displayshow.value == "clock":
			tm=localtime()
			servicename = strftime("%H%M",tm)
			evfd.getInstance().vfd_write_string(servicename[0:17])
			self.timer.startLongTimer(60-tm.tm_sec)

VFDIconsInstance = None

def main(session, **kwargs):
	# Create Instance if none present, show Dialog afterwards
	global VFDIconsInstance
	if VFDIconsInstance is None:
		VFDIconsInstance = VFDIcons(session)
	VFDIconsInstance.timerEvent()

def Plugins(**kwargs):
	return [
	PluginDescriptor(name="VFDdisplay", description="VFD Display config", where = PluginDescriptor.WHERE_MENU, fnc=VFDdisplay),
	PluginDescriptor(where = PluginDescriptor.WHERE_SESSIONSTART, fnc=main ) ]
