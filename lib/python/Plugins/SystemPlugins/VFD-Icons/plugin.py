# -*- coding: utf-8 -*-
#
# modified by Taapat 17-06-2012
# review by <alexeytech@gmail.com> aka technic
# Displaying the time from pinky EvoLux_on_Pingulux image PPDisplayTime plugin:
#		https://gitorious.org/~pinky1981/open-duckbox-project-sh4/pingulux-git/trees/master/tdt/cvs/cdk/root/usr/lib/enigma2/python/Plugins/Extensions/PPDisplayTime
# design from 2boom plugin:
#		Easy Pnel for Pli http://gisclub.tv/index.php?topic=5075.0
#
from Plugins.Plugin import PluginDescriptor
import ServiceReference
from enigma import iPlayableService, eTimer, eServiceCenter, iServiceInformation, eServiceReference
from enigma import evfd
from time import localtime, strftime
from Components.ServiceEventTracker import ServiceEventTracker, InfoBarBase
from Components.ActionMap import ActionMap
from Screens.Screen import Screen
from Components.config import config, ConfigSubsection, getConfigListEntry, ConfigSelection
from Components.ConfigList import ConfigListScreen
from Components.Label import Label

config.vfdicon = ConfigSubsection()
config.vfdicon.displayshow = ConfigSelection(default = "channel", choices = [
		("channel", _("channel")), ("channel number", _("channel number")), ("clock", _("clock")), ("blank", _("blank")) ])
displayshow = config.vfdicon.displayshow

class ConfigVFDDisplay(Screen, ConfigListScreen):
	skin = """
<screen name="ConfigVFDDisplay" position="center,180" size="500,200" title="VFD display configuration">
	<eLabel position="5,0" size="490,2" backgroundColor="#aaaaaa" />
<widget name="config" position="30,20" size="460,50" zPosition="1" scrollbarMode="showOnDemand" />
	<widget name="key_red" position="83,150" zPosition="2" size="170,30" valign="center" halign="center" font="Regular;22" transparent="1" />
	<widget name="key_green" position="257,150" zPosition="2" size="170,30" valign="center" halign="center" font="Regular;22" transparent="1" />
	<eLabel position="85,180" size="166,2" backgroundColor="#00ff2525" />
	<eLabel position="255,180" size="166,2" backgroundColor="#00389416" />
</screen>"""

	def __init__(self, session):
		self.session = session
		Screen.__init__(self, session)
		cfglist = []
		cfglist.append(getConfigListEntry(_("Show on VFD Display"), displayshow))
		ConfigListScreen.__init__(self, cfglist)
		self["actions"] = ActionMap(["SetupActions", "ColorActions"],
			{
				"cancel": self.cancel,
				"ok": self.keySave,
				"green": self.keySave,
				"red": self.cancel,
			}, -2)
		self["key_red"] = Label(_("Exit"))
		self["key_green"] = Label(_("Ok"))

	def cancel(self):
		self.showVFD()
		ConfigListScreen.keyCancel(self)

	def keySave(self):
		self.showVFD()
		ConfigListScreen.keySave(self)

	def showVFD(self):
		main(self.session)
		if config.vfdicon.displayshow.value != "clock":
			VFDIconsInstance.writeChannelName()

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
		# Save Session&Servicelist, Create Timer, Init Services
		self.list = []
		self.getList()
		self.session = session
		self.timer = eTimer()
		self.timer.callback.append(self.timerEvent)
		self.service = None
		self.onClose = [ ]
		self.__event_tracker = ServiceEventTracker(screen=self,eventmap=
			{
				iPlayableService.evUpdatedInfo: self.__evUpdatedInfo,
				iPlayableService.evUpdatedEventInfo: self.__evUpdatedEventInfo,
				iPlayableService.evVideoSizeChanged: self.__evVideoSizeChanged,
				iPlayableService.evSeekableStatusChanged: self.__evSeekableStatusChanged,
				iPlayableService.evStart: self.__evStart,
			})
		session.nav.record_event.append(self.gotRecordEvent)
		self.mp3Available = False
		self.dolbyAvailable = False
		
	def __evStart(self):
		print "[__evStart]"
		self.__evSeekableStatusChanged()	
		
	def __evUpdatedInfo(self):
		print "[__evUpdatedInfo]"
		if config.vfdicon.displayshow.value == "clock":
			return
		self.checkAudioTracks()
		self.writeChannelName()
		self.showCrypted()
		self.showDolby()
		self.showMp3()
		
	def writeChannelName(self):
		print "[writeVFDDisplay]"
		servicename = ""
		currPlay = self.session.nav.getCurrentService()
		if config.vfdicon.displayshow.value == "clock":
			pass
		elif config.vfdicon.displayshow.value == "blank":
			servicename = "    "
		elif currPlay != None and self.mp3Available:
			# show the MP3 tag
			servicename = currPlay.info().getInfoString(iServiceInformation.sTagTitle)
		else:
			# show the service information
			self.service = self.session.nav.getCurrentlyPlayingServiceReference()
			if not self.service is None:
				service = self.service.toCompareString()
				if config.vfdicon.displayshow.value == "channel":
					# show the service name
					servicename = ServiceReference.ServiceReference(service).getServiceName().replace('\xc2\x87', '').replace('\xc2\x86', '').ljust(16)
					subservice = self.service.toString().split("::")
					if subservice[0].count(':') == 9:
						servicename = subservice[1].replace('\xc2\x87', '').replace('\xc3\x9f', 'ss').replace('\xc2\x86', '').ljust(16)
				if config.vfdicon.displayshow.value == "channel number":
					# show the service channel number
					name = ServiceReference.ServiceReference(service).getServiceName().replace('\xc2\x87', '').replace('\xc2\x86', '')
					servicename = self.getServiceNumber(name)

			else:
				print "no Service found"
		
		print "vfd display text:", servicename[0:63]
		evfd.getInstance().vfd_write_string(servicename[0:63])
		return 1
		
	def showCrypted(self):
		print "[showCrypted]"
		service=self.session.nav.getCurrentService()
		if service is not None:
			info=service.info()
			crypted = info and info.getInfo(iServiceInformation.sIsCrypted) or -1
			if crypted == 1 : #set crypt symbol
				evfd.getInstance().vfd_set_icon(0x13,1)
			else:
				evfd.getInstance().vfd_set_icon(0x13,0)
	
	def checkAudioTracks(self):
		self.dolbyAvailable = False
		self.mp3Available = False
		service=self.session.nav.getCurrentService()
		if service is not None:
			audio = service.audioTracks()
			if audio:
				n = audio.getNumberOfTracks()
				for x in range(n):
					i = audio.getTrackInfo(x)
					description = i.getDescription();
					if description.find("MP3") != -1:
						self.mp3Available = True
					if description.find("AC3") != -1 or description.find("DTS") != -1:
						self.dolbyAvailable = True
	
	def showDolby(self):
		print "[showDolby]"
		if self.dolbyAvailable:
			evfd.getInstance().vfd_set_icon(0x17,1)
		else:
			evfd.getInstance().vfd_set_icon(0x17,0)
		
	def showMp3(self):
		print "[showMp3]"
		if self.mp3Available:
			evfd.getInstance().vfd_set_icon(0x15,1)
		else:
			evfd.getInstance().vfd_set_icon(0x15,0)
		
	def __evUpdatedEventInfo(self):
		print "[__evUpdatedEventInfo]"
		
	def getSeekState(self):
		service = self.session.nav.getCurrentService()
		if service is None:
			return False
		seek = service.seek()
		if seek is None:
			return False
		return seek.isCurrentlySeekable()
		
	def __evSeekableStatusChanged(self):
		print "[__evSeekableStatusChanged]"
		if self.getSeekState():
			evfd.getInstance().vfd_set_icon(0x1A,1)
		else:
			evfd.getInstance().vfd_set_icon(0x1A,0)
		
	def __evVideoSizeChanged(self):
		print "[__evVideoSizeChanged]"
		service=self.session.nav.getCurrentService()
		if service is not None:
			info=service.info()
			height = info and info.getInfo(iServiceInformation.sVideoHeight) or -1
			if height > 576 : #set HD symbol
				evfd.getInstance().vfd_set_icon(0x11,1)
			else:
				evfd.getInstance().vfd_set_icon(0x11,0)
		
	def gotRecordEvent(self, service, event):
		recs = self.session.nav.getRecordings()
		nrecs = len(recs)
		if nrecs > 0: #set rec symbol
			evfd.getInstance().vfd_set_icon(0x1e,1)
		else:
			evfd.getInstance().vfd_set_icon(0x1e,0)

	def getServiceNumber(self, name):
		if name in self.list:
			for idx in range(1, len(self.list)):
				if name == self.list[idx-1]:
					return str(idx)
		else:
			return ""

	def getList(self):
		serviceHandler = eServiceCenter.getInstance()
		services = serviceHandler.list(eServiceReference('1:134:1:0:0:0:0:0:0:0:(type == 1) || (type == 17) || (type == 195) || (type == 25) FROM BOUQUET "bouquets.tv" ORDER BY bouquet'))
		bouquets = services and services.getContent("SN", True)
		for bouquet in bouquets:
			services = serviceHandler.list(eServiceReference(bouquet[0]))
			channels = services and services.getContent("SN", True)
			for channel in channels:
				if not channel[0].startswith("1:64:"): # Ignore marker
					self.list.append(channel[1].replace('\xc2\x86', '').replace('\xc2\x87', ''))

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
	PluginDescriptor(name="VFDIcons", description="Icons in VFD", where = PluginDescriptor.WHERE_SESSIONSTART, fnc=main ) ]
