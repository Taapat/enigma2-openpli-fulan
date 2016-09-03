from Screens.Screen import Screen
from Components.ConfigList import ConfigListScreen
from Components.config import config, ConfigSubsection, ConfigInteger, ConfigSlider, getConfigListEntry

from enigma import fbClass


config.plugins.OSDPositionSetup = ConfigSubsection()
config.plugins.OSDPositionSetup.dst_left = ConfigInteger(default = 0)
config.plugins.OSDPositionSetup.dst_top = ConfigInteger(default = 0)
config.plugins.OSDPositionSetup.dst_right = ConfigInteger(default = 0)
config.plugins.OSDPositionSetup.dst_bottom = ConfigInteger(default = 0)


def setPosition(dst_left, dst_right, dst_top, dst_bottom):
	if dst_left > 150:
		dst_left = 150
	if dst_right < -150:
		dst_right = -150
	if dst_top > 150:
		dst_top = 150
	if dst_bottom < -150:
		dst_bottom = -150
	fbClass.getInstance().setFBdiff(dst_top, dst_left, dst_right, dst_bottom)
	fbClass.getInstance().clearFBblit()

def setConfiguredPosition():
	setPosition(int(config.plugins.OSDPositionSetup.dst_left.value), int(config.plugins.OSDPositionSetup.dst_right.value), int(config.plugins.OSDPositionSetup.dst_top.value), int(config.plugins.OSDPositionSetup.dst_bottom.value))

def main(session, **kwargs):
	from overscanwizard import OverscanWizard
	session.open(OverscanWizard, timeOut=False)

def startSetup(menuid):
	return menuid == "system" and [(_("Overscan wizard"), main, "sd_position_setup", 0)] or []

def startup(reason, **kwargs):
	setConfiguredPosition()

def Plugins(**kwargs):
	from Plugins.Plugin import PluginDescriptor
	return [PluginDescriptor(name = "Overscan wizard", description = "", where = PluginDescriptor.WHERE_SESSIONSTART, fnc = startup),
		PluginDescriptor(name = "Overscan wizard", description = _("Wizard to arrange the overscan"), where = PluginDescriptor.WHERE_MENU, fnc = startSetup)]
