from Components.ActionMap import ActionMap
from Components.ConfigList import ConfigListScreen
from Components.config import config, ConfigSelection, ConfigText, \
	getConfigListEntry
from Components.Sources.StaticText import StaticText
from Screens.MessageBox import MessageBox
from Plugins.Plugin import PluginDescriptor
from Screens.Screen import Screen

from os import path

class ModemSetup(Screen, ConfigListScreen):
	def __init__(self, session):
		Screen.__init__(self, session)
		self.skinName = ["Setup"]
		self.setTitle(_("Modem configuration"))
		self.createConfig()
		self.list = []
		ConfigListScreen.__init__(self, self.list, session)
		self.createSetup()
		self["key_red"] = StaticText(_("Cancel"))
		self["key_green"] = StaticText(_("OK"))
		self["actions"] = ActionMap(["SetupActions", "ColorActions"],
			{
				"cancel": self.cancel,
				"ok": self.ok,
				"green": self.ok,
				"red": self.cancel,
			}, -2)

	def createConfig(self):
		MODEMTYPE = "0"
		MODEMPORT = "ttyUSB0"
		MODEMSPEED = ""
		APN = "internet"
		MODEMUSERNAME = "username"
		MODEMPASSWORD = "password"
		MODEMMTU = "auto"
		MODEMMRU = "auto"
		MODEMPPPDOPTS = ""
		DIALNUMBER = "auto"
		MODEMAUTOSTART = "1"
		DEBUG = "0"
		SHARE = "0"
		if path.exists("/etc/modem.conf"):
			settings = []
			try:
				f = open("/etc/modem.conf", "r")
				for line in f.readlines():
					settings.append(line)
				f.close()
			except:
				print "[ModemSettings] ERROR in open configuration file"
			for line in settings:
				data = line.replace(' ', '').replace('\t', '')
				name = data.upper()
				if name.startswith('MODEMTYPE='):
					MODEMTYPE = data[10:11]
				elif name.startswith('MODEMPORT='):
					MODEMPORT = data[10:-1]
				elif name.startswith('MODEMSPEED='):
					MODEMSPEED = data[11:-1]
				elif name.startswith('APN='):
					start = line.find('"') + 1
					end = line[start:].find('"') + start
					APN = line[start:end]
				elif name.startswith('MODEMUSERNAME='):
					start = line.find('"') + 1
					end = line[start:].find('"') + start
					MODEMUSERNAME = line[start:end]
				elif name.startswith('MODEMPASSWORD='):
					start = line.find('"') + 1
					end = line[start:].find('"') + start
					MODEMPASSWORD = line[start:end]
				elif name.startswith('MODEMMTU='):
					MODEMMTU = data[9:-1]
				elif name.startswith('MODEMMRU='):
					MODEMMRU = data[9:-1]
				elif name.startswith('MODEMPPPDOPTS='):
					start = line.find('"') + 1
					end = line[start:].find('"') + start
					MODEMPPPDOPTS = line[start:end]
				elif name.startswith('DIALNUMBER='):
					DIALNUMBER = data[11:-1]
				elif name.startswith('MODEMAUTOSTART='):
					MODEMAUTOSTART = data[15:16]
				elif name.startswith('DEBUG='):
					DEBUG = data[6:7]
				elif name.startswith('SHARE='):
					SHARE = data[6:7]
		self.MODEMTYPE = ConfigSelection(default = MODEMTYPE,
			choices = [("0", _("gprs")), ("1", _("cdma"))])
		self.MODEMPORT = ConfigSelection(default = MODEMPORT,
			choices = [("auto", _("auto")), ("ttyACM0", _("ttyACM0")),
			("ttyUSB0", _("ttyUSB0")), ("ttyUSB1", _("ttyUSB1")),
			("ttyUSB2", _("ttyUSB2")), ("ttyUSB3", _("ttyUSB3")),
			("ttyUSB4", _("ttyUSB4")), ("ttyUSB5", _("ttyUSB5")),
			("ttyUSB6", _("ttyUSB6")), ("ttyUSB7", _("ttyUSB7")),
			("ttyUSB8", _("ttyUSB8")), ("ttyUSB9", _("ttyUSB9"))])
		self.MODEMSPEED = ConfigSelection(default = MODEMSPEED,
			choices = [('""', _("auto")), ("57600", _("57600")),
			("115200", _("115200")), ("230400", _("230400")),
			("460800", _("460800")), ("921600", _("921600"))])
		self.APN = ConfigText(default = APN, visible_width = 100,
			fixed_size = False)
		self.MODEMUSERNAME = ConfigText(default = MODEMUSERNAME,
			visible_width = 100, fixed_size = False)
		self.MODEMPASSWORD = ConfigText(default = MODEMPASSWORD,
			visible_width = 100, fixed_size = False)
		self.MODEMMTU = ConfigSelection(default = MODEMMTU,
			choices = [("auto", _("auto")), ("1000", _("1000")),
			("1100", _("1100")), ("1200", _("1200")), ("1300", _("1300")),
			("1400", _("1400")), ("1440", _("1440")), ("1460", _("1460")),
			("1492", _("1492")), ("1500", _("1500"))])
		self.MODEMMRU = ConfigSelection(default = MODEMMRU,
			choices = [("auto", _("auto")), ("1000", _("1000")),
			("1100", _("1100")), ("1200", _("1200")), ("1300", _("1300")),
			("1400", _("1400")), ("1440", _("1440")), ("1460", _("1460")),
			("1492", _("1492")), ("1500", _("1500"))])
		self.MODEMPPPDOPTS = ConfigText(default = MODEMPPPDOPTS,
			visible_width = 100, fixed_size = False)
		self.DIALNUMBER = ConfigSelection(default = DIALNUMBER,
			choices = [("auto", _("auto")), ('"*99#"', _("*99#")),
			('"*99***1#"', _("*99***1#")), ('"*99**1*1#"', _("*99**1*1#")),
			('"#777"', _("#777"))])
		self.MODEMAUTOSTART = ConfigSelection(default = MODEMAUTOSTART,
			choices = [("1", _("yes")), ("0", _("no"))])
		self.DEBUG = ConfigSelection(default = DEBUG,
			choices = [("1", _("yes")), ("0", _("no"))])
		self.SHARE = ConfigSelection(default = SHARE,
			choices = [("1", _("yes")), ("0", _("no"))])

	def createSetup(self):
		self.list = []
		self.list.append(getConfigListEntry(_("Modem type"), self.MODEMTYPE))
		self.list.append(getConfigListEntry(_("Port"), self.MODEMPORT))
		self.list.append(getConfigListEntry(_("Speed"), self.MODEMSPEED))
		self.list.append(getConfigListEntry(_("APN"), self.APN))
		self.list.append(getConfigListEntry(_("Username"), self.MODEMUSERNAME))
		self.list.append(getConfigListEntry(_("Password"), self.MODEMPASSWORD))
		self.list.append(getConfigListEntry(_("MTU"), self.MODEMMTU))
		self.list.append(getConfigListEntry(_("MRU"), self.MODEMMRU))
		self.list.append(getConfigListEntry(_("Additional PPPD options"),
			self.MODEMPPPDOPTS))
		self.list.append(getConfigListEntry(_("Dial number"), self.DIALNUMBER))
		self.list.append(getConfigListEntry(_("Auto start"),
			self.MODEMAUTOSTART))
		self.list.append(getConfigListEntry(_("Enable debug"), self.DEBUG))
		self.list.append(getConfigListEntry(_("Internet connection sharing"),
			self.SHARE))
		self["config"].list = self.list

	def ok(self):
		self.session.openWithCallback(self.updateConfig, MessageBox, (_(" Are you sure to save the current configuration?\n\n") ) )

	def cancel(self):
		ConfigListScreen.keyCancel(self)

	def updateConfig(self, ret = False):
		if ret == True:
			data = 'MODEMTYPE=' + self.MODEMTYPE.value
			data = data + '\nMODEMPORT=' + self.MODEMPORT.value
			data = data + '\nMODEMSPEED=' + self.MODEMSPEED.value
			data = data + '\nAPN="' + self.APN.value + '"'
			data = data + '\nMODEMUSERNAME="' + self.MODEMUSERNAME.value + '"'
			data = data + '\nMODEMPASSWORD="' + self.MODEMPASSWORD.value + '"'
			data = data + '\nMODEMMTU=' + self.MODEMMTU.value
			data = data + '\nMODEMMRU=' + self.MODEMMRU.value
			data = data + '\nMODEMPPPDOPTS="' + self.MODEMPPPDOPTS.value + '"'
			data = data + '\nDIALNUMBER='+ self.DIALNUMBER.value
			data = data + '\nMODEMAUTOSTART='+ self.MODEMAUTOSTART.value
			data = data + '\nDEBUG='+ self.DEBUG.value
			data = data + '\nSHARE=' + self.SHARE.value
			try:
				settings = open("/etc/modem.conf", "w")
				settings.write(data)
				settings.close()
				self.mbox = self.session.open(MessageBox, _("Configuration saved!"), MessageBox.TYPE_INFO, timeout = 3 )
			except:
				print "[ModemSettings] ERROR in save settings"
			self.close()

def main(session, **kwargs):
	session.open(ModemSetup)

def Plugins(**kwargs):
	return PluginDescriptor(name=_("Modem settings"),
		description=_("Plugin to set settings for modem connection."),
		where = PluginDescriptor.WHERE_PLUGINMENU, needsRestart = False, fnc=main)
