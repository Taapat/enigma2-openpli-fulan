# -*- coding: utf-8 -*-
#
# Extended ServiceName Converter for Enigma2 Dreamboxes (ServiceName2.py)
# Coded by vlamo (c) 2011
#
# Version: 0.4 (03.06.2011 18:40)
# Version: 0.5 (08.09.2012) add Alternative numbering mode support - Dmitry73 & 2boom
# Version: 0.6 (13.11.2013) remove unned for my and optimization - Taapat
#

from Components.Converter.Converter import Converter
from enigma import iServiceInformation, iPlayableService, iPlayableServicePtr, eServiceReference, eServiceCenter
from Components.Element import cached
from Components.NimManager import nimmanager
from Screens.ChannelSelection import service_types_radio, service_types_tv

class ServiceName2(Converter, object):
	PROVIDER = 0
	REFERENCE = 1
	SATELLITE = 2
	FORMAT = 3

	def __init__(self, type):
		Converter.__init__(self, type)
		if type == "Provider":
			self.type = self.PROVIDER
		elif type == "Reference":
			self.type = self.REFERENCE
		elif type == "Satellite":
			self.type = self.SATELLITE
		else:
			self.type = self.FORMAT
			self.sfmt = type[:]
		self.what = None

	def getProviderName(self, ref):
		if isinstance(ref, eServiceReference):
			typestr = ref.getData(0) in (2,10) and service_types_radio or service_types_tv
			pos = typestr.rfind(':')
			rootstr = '%s (channelID == %08x%04x%04x) && %s FROM PROVIDERS ORDER BY name' %(typestr[:pos+1],ref.getUnsignedData(4),ref.getUnsignedData(2),ref.getUnsignedData(3),typestr[pos+1:])
			provider_root = eServiceReference(rootstr)
			serviceHandler = eServiceCenter.getInstance()
			providerlist = serviceHandler.list(provider_root)
			if not providerlist is None:
				while True:
					provider = providerlist.getNext()
					if not provider.valid(): break
					if provider.flags & eServiceReference.isDirectory:
						servicelist = serviceHandler.list(provider)
						if not servicelist is None:
							while True:
								service = servicelist.getNext()
								if not service.valid(): break
								if service == ref:
									info = serviceHandler.info(provider)
									return info and info.getName(provider) or "Unknown"
		return " "

	def getSatelliteName(self, ref):
		if isinstance(ref, eServiceReference):
			orbpos = ref.getData(4) >> 16
			if orbpos == 0:
				return ""
			if orbpos < 0: orbpos += 3600
			try:
				return str(nimmanager.getSatDescription(orbpos))
			except:
				return orbpos > 1800 and "%d.%d°W"%((3600-orbpos)/10, (3600-orbpos)%10) or "%d.%d°E"%(orbpos/10, orbpos%10)
		return ""

	@cached
	def getText(self):
		service = self.source.service
		if isinstance(service, iPlayableServicePtr):
			info = service and service.info()
			ref = None
		else: # reference
			info = service and self.source.info
			ref = service
		if info is None:
			return ""
		if self.type == self.PROVIDER:
			return ref and self.getProviderName(ref) or info.getInfoString(iServiceInformation.sProvider)
		elif self.type == self.REFERENCE:
			return ref and ref.toString() or info.getInfoString(iServiceInformation.sServiceref)
		elif self.type == self.SATELLITE:
			return self.getSatelliteName(ref or eServiceReference(info.getInfoString(iServiceInformation.sServiceref)))
		elif self.type == self.FORMAT:
			sname = ref and ref.toString() or info.getInfoString(iServiceInformation.sServiceref)
			if "%3a//" in sname:
				return sname.split("%3a//")[1].split("/")[0].replace("%3a", ":")
			tpdata = ref and (info.getInfoObject(ref, iServiceInformation.sTransponderData) or -1) or info.getInfoObject(iServiceInformation.sTransponderData)
			if not isinstance(tpdata, dict):
				return ""
			result = '%d '%(tpdata.get('frequency', 0) / 1000)
			result += '%d '%(tpdata.get('symbol_rate', 0) / 1000)
			x = tpdata.get('polarization', 0)
			result += (x in range(4) and {0:'H',1:'V',2:'L',3:'R'}[x] or '?') + ' '
			x = tpdata.get('fec_inner', 15)
			result += (x in range(10)+[15] and {0:'Auto',1:'1/2',2:'2/3',3:'3/4',4:'5/6',5:'7/8',6:'8/9',7:'3/5',8:'4/5',9:'9/10',15:'None'}[x] or '') + ' '
			x = tpdata.get('modulation', 1)
			result += (x in range(4) and {0:'Auto',1:'QPSK',2:'8PSK',3:'QAM16'}[x] or '') + ' '
			x = tpdata.get('system', 0)
			result += x in range(2) and {0:'DVB-S',1:'DVB-S2'}[x] or ''
			return '%s'%(result)

	text = property(getText)

	def changed(self, what):
		if what[0] != self.CHANGED_SPECIFIC or what[1] in (iPlayableService.evStart,):
			Converter.changed(self, what)
