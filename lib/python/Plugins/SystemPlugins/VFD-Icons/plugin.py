# -*- coding: utf-8 -*-
from Plugins.Plugin import PluginDescriptor
import ServiceReference
from enigma import iPlayableService, eTimer, eServiceCenter, iServiceInformation
from enigma import evfd
import time
from Components.ServiceEventTracker import ServiceEventTracker, InfoBarBase

class VFDIcons:
	def __init__(self, session):
		# Save Session&Servicelist, Create Timer, Init Services
		self.session = session
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
		self.checkAudioTracks()
		self.writeChannelName()
		self.showCrypted()
		self.showDolby()
		self.showMp3()
		
	def writeChannelName(self):
		print "[writeChannelName]"
		servicename = ""
		currPlay = self.session.nav.getCurrentService()
		if currPlay != None and self.mp3Available:
			# show the MP3 tag
			servicename = currPlay.info().getInfoString(iServiceInformation.sTagTitle)
		else:
			# show the service name
			self.service = self.session.nav.getCurrentlyPlayingServiceReference()
			if not self.service is None:
				service = self.service.toCompareString()
				servicename = ServiceReference.ServiceReference(service).getServiceName().replace('\xc2\x87', '').replace('\xc2\x86', '').ljust(16)
				subservice = self.service.toString().split("::")
				if subservice[0].count(':') == 9:
					servicename = subservice[1].replace('\xc2\x87', '').replace('\xc3\x9f', 'ss').replace('\xc2\x86', '').ljust(16)
				else:
					servicename=servicename
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
		

VFDIconsInstance = None

def main(session, **kwargs):
	# Create Instance if none present, show Dialog afterwards
	global VFDIconsInstance
	if VFDIconsInstance is None:
		VFDIconsInstance = VFDIcons(session)

def Plugins(**kwargs):
 	return [ PluginDescriptor(name="VFDIcons", description="Icons in VFD", where = PluginDescriptor.WHERE_SESSIONSTART, fnc=main ) ]
