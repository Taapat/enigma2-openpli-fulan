from enigma import *
from Screens.Screen import Screen
from Screens.MessageBox import MessageBox
from Screens.ChoiceBox import ChoiceBox
from Components.ActionMap import ActionMap
from Components.MenuList import MenuList
from Screens.Console import Console
from Plugins.Plugin import PluginDescriptor
import os
import gettext


quickepg_title= _("Quick EPG Import")
quickepg_plugindir="/usr/lib/enigma2/python/Plugins/Extensions/QuickEPG" 

os.environ['LANGUAGE']='en'
quickepg_language='en'

if os.path.exists("/etc/enigma2/settings") == True:
   f = open("/etc/enigma2/settings")
   line = f.readline()
   while (line):
	line = f.readline().replace("\n","")
	sp = line.split("=")
	if (sp[0] == "config.osd.language"):
	   sp2 = sp[1].split("_")
           quickepg_language = sp2[0]
           if os.path.exists("%s/locale/%s" % (quickepg_plugindir, quickepg_language)) == True:
              os.environ["LANGUAGE"] = sp[1]
           else:
              os.environ['LANGUAGE']='en'
   f.close

_=gettext.Catalog('quickepg', '%s/locale' % quickepg_plugindir).gettext


def main(session,**kwargs):
    session.open(QuickEPGPlugin)

def autostart(reason,**kwargs):
    if reason == 0:
        print "[QUICKEPG] no autostart"

def Plugins(path,**kwargs):
    return [PluginDescriptor(
        name=_("Quick EPG Import"), 
        description="Download and import EPG for exUSSR channels", 
        where = PluginDescriptor.WHERE_PLUGINMENU,icon="quick_epg.png",
        fnc = main
        ),
	PluginDescriptor(name=_("Quick EPG Import"), where = PluginDescriptor.WHERE_EXTENSIONSMENU, fnc=main)]

class QuickEPGPlugin(Screen):
    skin = """
        <screen position="center,center" size="500,100" title="%s" >
            <widget name="menu" position="10,10" size="490,90" scrollbarMode="showOnDemand" />
        </screen>""" % quickepg_title
        
    def __init__(self, session, args = 0):
        self.skin = QuickEPGPlugin.skin
        self.session = session
        Screen.__init__(self, session)
        self.menu = args
        quickepglist = []
        quickepglist.append((_("Download EPG - events in ukr. language"), "uaepg"))
        quickepglist.append((_("Download EPG - events in rus. language"), "ruepg"))     
        self["menu"] = MenuList(quickepglist)
        self["actions"] = ActionMap(["WizardActions", "DirectionActions"],{"ok": self.go,"back": self.close,}, -1)
        
    def go(self):
        returnValue = self["menu"].l.getCurrentSelection()[1]
        if returnValue is not None:
           if returnValue is "quickepg":
              QuickEPG(self.session)
           elif returnValue is "uaepg":
              self.session.open(Console,_("Downloading EPG ..."),["echo 'epg_ua.dat.gz' > /etc/epgdat && /usr/lib/enigma2/python/Plugins/Extensions/QuickEPG/quick_epg.sh"])
           elif returnValue is "ruepg":
              self.session.open(Console,_("Downloading EPG ..."),["echo 'epg_ru.dat.gz' > /etc/epgdat && /usr/lib/enigma2/python/Plugins/Extensions/QuickEPG/quick_epg.sh"])



