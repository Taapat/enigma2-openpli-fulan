from os import path

from Components.ActionMap import ActionMap
from Components.ConfigList import ConfigListScreen
from Components.config import config, ConfigSelection, ConfigText, \
	getConfigListEntry
from Components.Label import Label
from Components.Sources.StaticText import StaticText
from Screens.MessageBox import MessageBox
from Plugins.Plugin import PluginDescriptor
from Screens.Screen import Screen

class ModemSetup(Screen, ConfigListScreen):
	def __init__(self, session):
		Screen.__init__(self, session)
		self.skinName = ["Setup"]
		self.setTitle(_("Modem configuration"))
		self["key_red"] = StaticText(_("Cancel"))
		self["key_green"] = StaticText(_("OK"))
		self["description"] = Label()
		self["actions"] = ActionMap(["SetupActions", "ColorActions"],
			{
				"cancel": self.cancel,
				"ok": self.ok,
				"green": self.ok,
				"red": self.cancel,
			}, -2)
		self.list = []
		ConfigListScreen.__init__(self, self.list, session)
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
			try:
				for line in open("/etc/modem.conf", "r").readlines():
					data = line.replace(' ', '').replace('\t', '')
					name = data.upper()
					if name[:10] == 'MODEMTYPE=':
						MODEMTYPE = data[10:11]
					elif name[:10] == 'MODEMPORT=':
						MODEMPORT = data[10:-1]
					elif name[:11] == 'MODEMSPEED=':
						MODEMSPEED = data[11:-1]
					elif name[:4] == 'APN=':
						start = line.find('"') + 1
						end = line[start:].find('"') + start
						APN = line[start:end]
					elif name[:14] == 'MODEMUSERNAME=':
						start = line.find('"') + 1
						end = line[start:].find('"') + start
						MODEMUSERNAME = line[start:end]
					elif name[:14] == 'MODEMPASSWORD=':
						start = line.find('"') + 1
						end = line[start:].find('"') + start
						MODEMPASSWORD = line[start:end]
					elif name[:9] == 'MODEMMTU=':
						MODEMMTU = data[9:-1]
					elif name[:9] == 'MODEMMRU=':
						MODEMMRU = data[9:-1]
					elif name[:14] == 'MODEMPPPDOPTS=':
						start = line.find('"') + 1
						end = line[start:].find('"') + start
						MODEMPPPDOPTS = line[start:end]
					elif name[:11] == 'DIALNUMBER=':
						DIALNUMBER = data[11:-1]
					elif name[:15] == 'MODEMAUTOSTART=':
						MODEMAUTOSTART = data[15:16]
					elif name[:6] == 'DEBUG=':
						DEBUG = data[6:7]
					elif name[:6] == 'SHARE=':
						SHARE = data[6:7]
			except:
				print "[ModemSettings] ERROR in open configuration file"
		self.MODEMTYPE = ConfigSelection(default = MODEMTYPE,
			choices = [("0", "gprs"), ("1", "cdma")])
		self.MODEMPORT = ConfigSelection(default = MODEMPORT,
			choices = [("auto", _("auto")), ("ttyACM0", "ttyACM0"),
			("ttyUSB0", "ttyUSB0"), ("ttyUSB1", "ttyUSB1"),
			("ttyUSB2", "ttyUSB2"), ("ttyUSB3", "ttyUSB3"),
			("ttyUSB4", "ttyUSB4"), ("ttyUSB5", "ttyUSB5"),
			("ttyUSB6", "ttyUSB6"), ("ttyUSB7", "ttyUSB7"),
			("ttyUSB8", "ttyUSB8"), ("ttyUSB9", "ttyUSB9")])
		self.MODEMSPEED = ConfigSelection(default = MODEMSPEED,
			choices = [('""', _("auto")), ("57600", "57600"),
			("115200", "115200"), ("230400", "230400"),
			("460800", "460800"), ("921600", "921600")])
		self.APN = ConfigText(default = APN, visible_width = 100,
			fixed_size = False)
		self.MODEMUSERNAME = ConfigText(default = MODEMUSERNAME,
			visible_width = 100, fixed_size = False)
		self.MODEMPASSWORD = ConfigText(default = MODEMPASSWORD,
			visible_width = 100, fixed_size = False)
		self.MODEMMTU = ConfigSelection(default = MODEMMTU,
			choices = [("auto", _("auto")), ("1000", "1000"),
			("1100", "1100"), ("1200", "1200"), ("1300", "1300"),
			("1400", "1400"), ("1440", "1440"), ("1460", "1460"),
			("1492", "1492"), ("1500", "1500")])
		self.MODEMMRU = ConfigSelection(default = MODEMMRU,
			choices = [("auto", _("auto")), ("1000", "1000"),
			("1100", "1100"), ("1200", "1200"), ("1300", "1300"),
			("1400", "1400"), ("1440", "1440"), ("1460", "1460"),
			("1492", "1492"), ("1500", "1500")])
		self.MODEMPPPDOPTS = ConfigText(default = MODEMPPPDOPTS,
			visible_width = 100, fixed_size = False)
		self.DIALNUMBER = ConfigSelection(default = DIALNUMBER,
			choices = [("auto", _("auto")), ('"*99#"', "*99#"),
			('"*99***1#"', "*99***1#"), ('"*99**1*1#"', "*99**1*1#"),
			('"#777"', "#777")])
		self.MODEMAUTOSTART = ConfigSelection(default = MODEMAUTOSTART,
			choices = [("1", _("yes")), ("0", _("no"))])
		self.DEBUG = ConfigSelection(default = DEBUG,
			choices = [("1", _("yes")), ("0", _("no"))])
		self.SHARE = ConfigSelection(default = SHARE,
			choices = [("1", _("yes")), ("0", _("no"))])
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
		self["config"].onSelectionChanged.append(self.selectionChanged)

	def selectionChanged(self):
		self["description"].text = self["config"].getCurrent()[0]

	def ok(self):
		self.session.openWithCallback(self.updateConfig, MessageBox, (_("Are you sure to save the current configuration?\n\n")))

	def cancel(self):
		ConfigListScreen.keyCancel(self)

	def updateConfig(self, ret = False):
		if ret == True:
			data = 'MODEMTYPE=' + self.MODEMTYPE.value
			data += '\nMODEMPORT=' + self.MODEMPORT.value
			data += '\nMODEMSPEED=' + self.MODEMSPEED.value
			data += '\nAPN="' + self.APN.value + '"'
			data += '\nMODEMUSERNAME="' + self.MODEMUSERNAME.value + '"'
			data += '\nMODEMPASSWORD="' + self.MODEMPASSWORD.value + '"'
			data += '\nMODEMMTU=' + self.MODEMMTU.value
			data += '\nMODEMMRU=' + self.MODEMMRU.value
			data += '\nMODEMPPPDOPTS="' + self.MODEMPPPDOPTS.value + '"'
			data += '\nDIALNUMBER='+ self.DIALNUMBER.value
			data += '\nMODEMAUTOSTART='+ self.MODEMAUTOSTART.value
			data += '\nDEBUG='+ self.DEBUG.value
			data += '\nSHARE=' + self.SHARE.value
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
