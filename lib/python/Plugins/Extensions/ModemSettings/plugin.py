from Screens.Screen import Screen
from Screens.MessageBox import MessageBox
from Components.ActionMap import ActionMap
from Components.config import config, ConfigSubsection, getConfigListEntry, ConfigSelection, ConfigText, getConfigListEntry
from Components.ConfigList import ConfigListScreen
from Components.Pixmap import Pixmap
from Components.Sources.StaticText import StaticText
from Plugins.Plugin import PluginDescriptor
import os
from os import path as os_path

class ModemSetup(Screen, ConfigListScreen):

	def __init__(self, session):
		self.session = session
		Screen.__init__(self, self.session)
		self.skinName = ["Setup"]
		self.setTitle(_("Modem configuration"))
		self.createConfig()
		self["actions"] = ActionMap(["SetupActions", "ColorActions"],
			{
				"cancel": self.cancel,
				"ok": self.ok,
				"green": self.ok,
				"red": self.cancel,
			}, -2)
		self.list = []
		ConfigListScreen.__init__(self, self.list, session = self.session)
		self.createSetup()
		self["key_red"] = StaticText(_("Cancel"))
		self["key_green"] = StaticText(_("OK"))

	def createConfig(self):
		if os.path.exists("/etc/modem.conf"):
			settings = open("/etc/modem.conf")
			self.MODEMTYPE = settings.readline().strip()[10:11]
			self.MODEMPORT = settings.readline().strip()[10:]
			self.MODEMSPEED = settings.readline().strip()[11:]
			self.APN = settings.readline().strip()[5:-1]
			self.MODEMUSERNAME = settings.readline().strip()[15:-1]
			self.MODEMPASSWORD = settings.readline().strip()[15:-1]
			self.MODEMMTU = settings.readline().strip()[9:]
			self.MODEMMRU = settings.readline().strip()[9:]
			self.MODEMPPPDOPTS = settings.readline().strip()[15:-1]
			self.DIALNUMBER = settings.readline().strip()[11:]
			self.MODEMAUTOSTART = settings.readline().strip()[15:16]
			self.DEBUG = settings.readline().strip()[6:7]
			self.SHARE = settings.readline().strip()[6:7]
			settings.close()
		else:
			self.MODEMTYPE = "0"
			self.MODEMPORT = "ttyUSB0"
			self.MODEMSPEED = ""
			self.APN = "internet"
			self.MODEMUSERNAME = "username"
			self.MODEMPASSWORD = "password"
			self.MODEMMTU = "auto"
			self.MODEMMRU = "auto"
			self.MODEMPPPDOPTS = ""
			self.DIALNUMBER = '"*99#"'
			self.MODEMAUTOSTART = "1"
			self.DEBUG = "0"
			self.SHARE = "0"
		
		self.MODEMTYPEEntry = ConfigSelection(default = self.MODEMTYPE, choices = [ ("0", _("gprs")), ("1", _("cdma")) ])
		self.MODEMPORTEntry = ConfigSelection(default = self.MODEMPORT, choices = [ ("auto", _("auto")), ("ttyACM0", _("ttyACM0")), ("ttyUSB0", _("ttyUSB0")), ("ttyUSB1", _("ttyUSB1")), ("ttyUSB2", _("ttyUSB2")), ("ttyUSB3", _("ttyUSB3")), ("ttyUSB4", _("ttyUSB4")), ("ttyUSB5", _("ttyUSB5")), ("ttyUSB6", _("ttyUSB6")), ("ttyUSB7", _("ttyUSB7")), ("ttyUSB8", _("ttyUSB8")), ("ttyUSB9", _("ttyUSB9")) ])
		self.MODEMSPEEDEntry = ConfigSelection(default = self.MODEMSPEED, choices = [ ('""', _("auto")), ("57600", _("57600")), ("115200", _("115200")), ("230400", _("230400")), ("460800", _("460800")), ("921600", _("921600")) ])
		self.APNEntry = ConfigText(default = self.APN, visible_width = 100, fixed_size = False)
		self.MODEMUSERNAMEEntry = ConfigText(default = self.MODEMUSERNAME, visible_width = 100, fixed_size = False)
		self.MODEMPASSWORDEntry = ConfigText(default = self.MODEMPASSWORD, visible_width = 100, fixed_size = False)
		self.MODEMMTUEntry = ConfigSelection(default = self.MODEMMTU, choices = [ ("auto", _("auto")), ("1000", _("1000")), ("1100", _("1100")), ("1200", _("1200")), ("1300", _("1300")), ("1400", _("1400")), ("1440", _("1440")), ("1460", _("1460")), ("1492", _("1492")), ("1500", _("1500")) ])
		self.MODEMMRUEntry = ConfigSelection(default = self.MODEMMRU, choices = [ ("auto", _("auto")), ("1000", _("1000")), ("1100", _("1100")), ("1200", _("1200")), ("1300", _("1300")), ("1400", _("1400")), ("1440", _("1440")), ("1460", _("1460")), ("1492", _("1492")), ("1500", _("1500")) ])
		self.MODEMPPPDOPTSEntry = ConfigText(default = self.MODEMPPPDOPTS, visible_width = 100, fixed_size = False)
		self.DIALNUMBEREntry = ConfigSelection(default = self.DIALNUMBER, choices = [ ('"*99#"', _("*99#")), ('"*99***1#"', _("*99***1#")), ('"*99**1*1#"', _("*99**1*1#")), ('"#777"', _("#777")) ])
		self.MODEMAUTOSTARTEntry = ConfigSelection(default = self.MODEMAUTOSTART, choices = [ ("1", _("yes")), ("0", _("no")) ])
		self.DEBUGEntry = ConfigSelection(default = self.DEBUG, choices = [ ("1", _("yes")), ("0", _("no")) ])
		self.SHAREEntry = ConfigSelection(default = self.SHARE, choices = [ ("1", _("yes")), ("0", _("no")) ])

	def createSetup(self):
		self.list = []
		self.list.append(getConfigListEntry(_("Modem type"), self.MODEMTYPEEntry))
		self.list.append(getConfigListEntry(_("Port"), self.MODEMPORTEntry))
		self.list.append(getConfigListEntry(_("Speed"), self.MODEMSPEEDEntry))
		self.list.append(getConfigListEntry(_("APN"), self.APNEntry))
		self.list.append(getConfigListEntry(_("Username"), self.MODEMUSERNAMEEntry))
		self.list.append(getConfigListEntry(_("Password"), self.MODEMPASSWORDEntry))
		self.list.append(getConfigListEntry(_("MTU"), self.MODEMMTUEntry))
		self.list.append(getConfigListEntry(_("MRU"), self.MODEMMRUEntry))
		self.list.append(getConfigListEntry(_("Additional PPPD options"), self.MODEMPPPDOPTSEntry))
		self.list.append(getConfigListEntry(_("Dial number"), self.DIALNUMBEREntry))
		self.list.append(getConfigListEntry(_("Auto start"), self.MODEMAUTOSTARTEntry))
		self.list.append(getConfigListEntry(_("Enable debug"), self.DEBUGEntry))
		self.list.append(getConfigListEntry(_("Internet connection sharing"), self.SHAREEntry))
		self["config"].list = self.list

	def ok(self):
		self.session.openWithCallback(self.updateConfig, MessageBox, (_(" Are you sure to save the current configuration?\n\n") ) )

	def cancel(self):
		self.close()

	def updateConfig(self, ret = False):
		self["VirtualKB"].setEnabled(False)
		if (ret == True):
			self.mbox = self.session.open(MessageBox, _("Configuration saved!"), MessageBox.TYPE_INFO, timeout = 3 )
			settings = open("/etc/modem.conf", "w")
			settings.write('MODEMTYPE=%s\nMODEMPORT=%s\nMODEMSPEED=%s\nAPN="%s"\nMODEMUSERNAME="%s"\nMODEMPASSWORD="%s"\nMODEMMTU=%s\nMODEMMRU=%s\nMODEMPPPDOPTS="%s"\nDIALNUMBER=%s\nMODEMAUTOSTART=%s\nDEBUG=%s\nSHARE=%s' % (self.MODEMTYPEEntry.value, self.MODEMPORTEntry.value, self.MODEMSPEEDEntry.value, self.APNEntry.value, self.MODEMUSERNAMEEntry.value, self.MODEMPASSWORDEntry.value, self.MODEMMTUEntry.value, self.MODEMMRUEntry.value, self.MODEMPPPDOPTSEntry.value, self.DIALNUMBEREntry.value, self.MODEMAUTOSTARTEntry.value, self.DEBUGEntry.value, self.SHAREEntry.value))
			settings.close()
			self.close()

def main(session, **kwargs):
	session.open(ModemSetup)

def Plugins(**kwargs):
	return PluginDescriptor(name="Modem settings", description="Plugin to set settings for modem connection", where = PluginDescriptor.WHERE_PLUGINMENU, needsRestart = False, fnc=main)

