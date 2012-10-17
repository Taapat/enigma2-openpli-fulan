# -*- coding: utf-8 -*-
#
# by Taapat <taapat@gmail.com> 17-10-2012
# review by <alexeytech@gmail.com> aka technic

from Components.ActionMap import ActionMap
from Components.config import config, ConfigSubsection, getConfigListEntry, ConfigSelection
from Components.ConfigList import ConfigListScreen
from Components.ServiceEventTracker import ServiceEventTracker
from Components.Sources.StaticText import StaticText
from enigma import iPlayableService, eTimer, iServiceInformation, evfd
from Plugins.Plugin import PluginDescriptor
from Screens.Screen import Screen
from ServiceReference import ServiceReference
from time import localtime, strftime

config.plugins.vfdicon = ConfigSubsection()
config.plugins.vfdicon.displayshow = ConfigSelection(default = "channel", choices = [
		("channel", _("channel")), ("channel number", _("channel number")), ("clock", _("clock")), ("blank", _("blank")) ])

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
		cfglist.append(getConfigListEntry(_("Show on VFD display"), config.plugins.vfdicon.displayshow))
		ConfigListScreen.__init__(self, cfglist)

	def cancel(self):
		main(self)
		ConfigListScreen.keyCancel(self)

	def keySave(self):
		main(self)
		ConfigListScreen.keySave(self)

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
				iPlayableService.evStart: self.WriteName,
			})

	def WriteName(self):
		if config.plugins.vfdicon.displayshow.value != "clock":
			servicename = "    "
			if config.plugins.vfdicon.displayshow.value != "blank":
				service = self.session.nav.getCurrentService()
				if service:
					servicename = service.info().getInfoString(iServiceInformation.sTagTitle)
					if servicename == "":
						if config.plugins.vfdicon.displayshow.value == "channel number":
							try:
								servicename = str(self.session.nav.getCurrentlyPlayingServiceReference(False).getChannelNum())
							except:
								servicename = "    "
								print "[VFD Display] ERROR set channel number"
						else:
							try:
								servicename = ServiceReference(self.session.nav.getCurrentlyPlayingServiceReference(False)).getServiceName()
							except:
								servicename = "    "
								print "[VFD Display] ERROR set service name"
					elif servicename == "not found":
						servicename = "PLAY"
			print "[VFD Display] text ", servicename[0:20]
			evfd.getInstance().vfd_write_string(servicename[0:20])

	def timerEvent(self):
		if config.plugins.vfdicon.displayshow.value == "clock":
			tm=localtime()
			servicename = strftime("%H%M", tm)
			evfd.getInstance().vfd_write_string(servicename[0:4])
			self.timer.startLongTimer(60-tm.tm_sec)

VFDIconsInstance = None

def main(session, **kwargs):
	global VFDIconsInstance
	if VFDIconsInstance is None:
		VFDIconsInstance = VFDIcons(session)
	if config.plugins.vfdicon.displayshow.value == "clock":
		VFDIconsInstance.timerEvent()
	else:
		VFDIconsInstance.WriteName()

def Plugins(**kwargs):
	return [
	PluginDescriptor(name="VFDdisplay", description="VFD display config", where = PluginDescriptor.WHERE_MENU, fnc=VFDdisplay),
	PluginDescriptor(where = PluginDescriptor.WHERE_SESSIONSTART, fnc=main ) ]