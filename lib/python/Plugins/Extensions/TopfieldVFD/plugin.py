from Screens.Screen import Screen
from Screens.MessageBox import MessageBox
from Plugins.Plugin import PluginDescriptor
from Tools import Notifications
from Components.Pixmap import Pixmap, MovingPixmap
from Components.ActionMap import ActionMap, NumberActionMap
from Components.Label import Label
from Components.Button import Button
from Components.Console import Console
from Components.ConfigList import ConfigList
from Components.config import config, ConfigSubsection, ConfigEnableDisable, \
     getConfigListEntry, ConfigInteger, ConfigSelection
from Components.ConfigList import ConfigListScreen
from Plugins.Plugin import PluginDescriptor
import ServiceReference
from enigma import iPlayableService, eTimer, eServiceCenter, iServiceInformation
from enigma import evfd
import time, fcntl, struct
from Components.ServiceEventTracker import ServiceEventTracker, InfoBarBase
from enigma import eTimer
from re import compile as re_compile, search as re_search
from time import time, localtime, strftime
from Components.UsageConfig import defaultMoviePath
from os import statvfs
import array

my_global_session = None
debug = False


config.plugins.TopfieldVFD = ConfigSubsection()
config.plugins.TopfieldVFD.allCaps = ConfigEnableDisable(default = False)
config.plugins.TopfieldVFD.scroll = ConfigSelection(choices = [("never"), ("once"), ("always")])
config.plugins.TopfieldVFD.brightness = ConfigSelection(default = "medium", choices = [("dark"), ("medium"), ("bright")])
config.plugins.TopfieldVFD.scrollPause = ConfigInteger(default = 100, limits = (1, 255))
config.plugins.TopfieldVFD.scrollDelay = ConfigInteger(default = 10, limits = (1, 255))
config.plugins.TopfieldVFD.typematicDelay = ConfigInteger(default = 3, limits = (0, 255))
config.plugins.TopfieldVFD.typematicRate = ConfigInteger(default = 10, limits = (0, 255))
config.plugins.TopfieldVFD.rcCommandSet = ConfigSelection(default = "TF7700 & Masterpiece", choices = [("TF7700"), ("Masterpiece"), ("TF7700 & Masterpiece")])
config.plugins.TopfieldVFD.showClock = ConfigEnableDisable(default = True)
config.plugins.TopfieldVFD.showEthernet = ConfigEnableDisable(default = True)

# ioctl definitions for the VFD
ioBootReason = 0x40003a0b
ioOffFlush = struct.pack('LLB', 0x2, 0x0, 0x6)
ioRec1Flush = struct.pack('LLB', 0x1000, 0x0, 0x6)
ioRec2Flush = struct.pack('LLB', 0x2000, 0x0, 0x6)
ioRecBothFlush = struct.pack('LLB', 0x3000, 0x0, 0x6)
ioClockFlush = struct.pack('LLB', 0x20, 0x0, 0x6)
ioClockOff = struct.pack('LLB', 0x20, 0x0, 0x0)
ioHddClear =    struct.pack('LLB', 0x0, 0xff8000,0x0)
ioHddUsage = (  struct.pack('LLB', 0x0, 0x006000,0xf),  # HDD empty
                struct.pack('LLB', 0x0, 0x00e000,0xf),
                struct.pack('LLB', 0x0, 0x01e000,0xf),
                struct.pack('LLB', 0x0, 0x03e000,0xf),
                struct.pack('LLB', 0x0, 0x07e000,0xf),
                struct.pack('LLB', 0x0, 0x0fe000,0xf),
                struct.pack('LLB', 0x0, 0x1fe000,0xf),
                struct.pack('LLB', 0x0, 0x3fe000,0xf),
                struct.pack('LLB', 0x0, 0x7fe000,0xf))  # HDD full
ioHddFull = struct.pack('LLB', 0x0, 0x800000,0x6)       # "HDD full" flashing
hddCheckPeriod = 60 # check every 60 seconds
ioIconCmd = 0x400b3a20
ioTimeshiftOn = struct.pack('LLB', 0x80, 0x0, 0xf)
ioTimeshiftOff = struct.pack('LLB', 0x80, 0x0, 0x0)
ioRec1On = struct.pack('LLB', 0x1000, 0x0, 0xf)
ioRec1Off = struct.pack('LLB', 0x1000, 0x0, 0x0)
ioRec2On = struct.pack('LLB', 0x2000, 0x0, 0xf)
ioRec2Off = struct.pack('LLB', 0x2000, 0x0, 0x0)
ioRecBothOn = struct.pack('LLB', 0x3000, 0x0, 0xf)
ioRecBothOff = struct.pack('LLB', 0x3000, 0x0, 0x0)
ioEthBothOff = struct.pack('LLB', 0x0c000000, 0x0, 0x0)
ioEthLeftOn = struct.pack('LLB', 0x08000000, 0x0, 0xe)
ioEthRightOn = struct.pack('LLB', 0x04000000, 0x0,0xb)

ioColonOn = struct.pack('LLB', 0x4, 0x0,0x3)
ioColonOff = struct.pack('LLB', 0x4, 0x0,0x0)
ioBrightnessCmd = 0x40013a05
ioIrFilter1Cmd = 0x40003a06
ioIrFilter4Cmd = 0x40003a09
ioTypematicDelayCmd = 0x40013a0d
ioTypematicRateCmd = 0x40013a0e
ioScrollModeCmd = 0x40033a15
ioAllcapsCmd = 0x40013a14

class TopfieldVFDSetup(ConfigListScreen, Screen):
        skin = """
                <screen position="100,100" size="550,400" title="TopfieldVFD Setup" >
                <widget name="config" position="20,10" size="460,350" scrollbarMode="showOnDemand" />
                <ePixmap position="140,350" size="140,40" pixmap="skin_default/buttons/green.png" alphatest="on" />
                <ePixmap position="280,350" size="140,40" pixmap="skin_default/buttons/red.png" alphatest="on" />
                <widget name="key_green" position="140,350" size="140,40" font="Regular;20" backgroundColor="#1f771f" zPosition="2" transparent="1" shadowColor="black" shadowOffset="-1,-1" />
                <widget name="key_red" position="280,350" size="140,40" font="Regular;20" backgroundColor="#9f1313" zPosition="2" transparent="1" shadowColor="black" shadowOffset="-1,-1" />
                </screen>"""


        def __init__(self, session, args = None):
                Screen.__init__(self, session)
                self.onClose.append(self.abort)

                # create elements for the menu list
                self.list = [ ]
                self.list.append(getConfigListEntry(_("Show clock"), config.plugins.TopfieldVFD.showClock))
                self.list.append(getConfigListEntry(_("Show Ethernet activity"), config.plugins.TopfieldVFD.showEthernet))
                self.list.append(getConfigListEntry(_("Brightness"), config.plugins.TopfieldVFD.brightness))
                self.list.append(getConfigListEntry(_("All caps"), config.plugins.TopfieldVFD.allCaps))
                self.list.append(getConfigListEntry(_("Scroll long strings"), config.plugins.TopfieldVFD.scroll))
                self.list.append(getConfigListEntry(_("Scroll pause"), config.plugins.TopfieldVFD.scrollPause))
                self.list.append(getConfigListEntry(_("Scroll delay"), config.plugins.TopfieldVFD.scrollDelay))
                self.list.append(getConfigListEntry(_("Typematic delay"), config.plugins.TopfieldVFD.typematicDelay))
                self.list.append(getConfigListEntry(_("Typematic rate"), config.plugins.TopfieldVFD.typematicRate))
                self.list.append(getConfigListEntry(_("RC command set"), config.plugins.TopfieldVFD.rcCommandSet))
                ConfigListScreen.__init__(self, self.list)

                self.Console = Console()
                self["key_red"] = Button(_("Cancel"))
                self["key_green"] = Button(_("Save"))

                # DO NOT ASK.
                self["setupActions"] = ActionMap(["SetupActions"],
                {
                        "save": self.save,
                        "cancel": self.cancel,
                        "ok": self.save,
                }, -2)

        def abort(self):
                print "aborting"

        def save(self):
                # save all settings
                for x in self["config"].list:
                        x[1].save()
                tfVfd.setValues()
                self.close()

        def cancel(self):
                for x in self["config"].list:
                        x[1].cancel()
                self.close()


class TopfieldVFD:
        def __init__(self, session):
                #print "TopfieldVFD initializing"
                self.session = session
                self.service = None
                self.onClose = [ ]
                self.__event_tracker = ServiceEventTracker(screen=self,eventmap=
                        {
                                iPlayableService.evSeekableStatusChanged: self.__evSeekableStatusChanged,
                                iPlayableService.evStart: self.__evStart,
                        })
                session.nav.record_event.append(self.gotRecordEvent)
                self.Console = Console()
                self.tsEnabled = False
                self.recNum = 0
                self.timer = eTimer()
                self.timer.callback.append(self.handleTimer)
                self.timer.start(1000, False)
                self.txCount = 0
                self.clock = 0
                self.valuesSet = 0
                self.hddUsed = 10 # initialize with an invalid value
                self.hddCheckCounter = hddCheckPeriod
                self.ethEnabled = config.plugins.TopfieldVFD.showEthernet.getValue()
                self.clockEnabled = config.plugins.TopfieldVFD.showClock.getValue()
                self.setValues()

        def setValues(self):
                #print "\nTopfiledVFD.setValues()\n"
                if config.plugins.TopfieldVFD.showClock.value:
                        self.enableClock()
                else:
                        self.disableClock()

                # enable/disable displaying Ethernet activity
                if config.plugins.TopfieldVFD.showEthernet.getValue():
                        self.enableEthernet()
                else:
                        self.disableEthernet()

                try:
                        fd = open("/dev/fpc")

                        # set the brightness
                        brightness = 3
                        if config.plugins.TopfieldVFD.brightness.getValue() == "dark":
                                brightness = 1
                        elif config.plugins.TopfieldVFD.brightness.getValue() == "bright":
                                brightness = 5
                        fcntl.ioctl(fd.fileno(), ioBrightnessCmd, struct.pack('B', brightness))

                        # set the the scroll mode
                        if config.plugins.TopfieldVFD.scroll.value == "once":
                                scrollMode = 1
                        elif config.plugins.TopfieldVFD.scroll.value == "always":
                                scrollMode = 2
                        else: # set to never by default
                                scrollMode = 0
                        scrollOpts = struct.pack('BBB', scrollMode,
                                                int(config.plugins.TopfieldVFD.scrollPause.value),
                                                int(config.plugins.TopfieldVFD.scrollDelay.value))
                        fcntl.ioctl(fd.fileno(), ioScrollModeCmd, scrollOpts)

                        # set the typematic values
                        tmp = struct.pack('B', int(config.plugins.TopfieldVFD.typematicRate.value))
                        fcntl.ioctl(fd.fileno(), ioTypematicRateCmd, tmp)
                        tmp = struct.pack('B', int(config.plugins.TopfieldVFD.typematicDelay.value))
                        fcntl.ioctl(fd.fileno(), ioTypematicDelayCmd, tmp)

                        # set the IR filters
                        if config.plugins.TopfieldVFD.rcCommandSet.getValue() == "Masterpiece":
                                fcntl.ioctl(fd.fileno(), ioIrFilter1Cmd, struct.pack('B', 1))
                                fcntl.ioctl(fd.fileno(), ioIrFilter4Cmd, struct.pack('B', 0))
                        elif config.plugins.TopfieldVFD.rcCommandSet.getValue() == "TF7700":
                                fcntl.ioctl(fd.fileno(), ioIrFilter1Cmd, struct.pack('B', 0))
                                fcntl.ioctl(fd.fileno(), ioIrFilter4Cmd, struct.pack('B', 1))
                        else: # enable both by default
                                fcntl.ioctl(fd.fileno(), ioIrFilter1Cmd, struct.pack('B', 1))
                                fcntl.ioctl(fd.fileno(), ioIrFilter4Cmd, struct.pack('B', 1))

                        # set the allcaps parameter
                        if config.plugins.TopfieldVFD.allCaps.value:
                                fcntl.ioctl(fd.fileno(), ioAllcapsCmd, struct.pack('B', 1))
                        else:
                                fcntl.ioctl(fd.fileno(), ioAllcapsCmd, struct.pack('B', 0))

#---- Topfi start ----#
                        buf = array.array('h', [0])
                        fcntl.ioctl(fd.fileno(),ioBootReason,buf,1)
                        if buf[0] == 2:
                                fcntl.ioctl(fd.fileno(), ioIconCmd, ioOffFlush)
#---- Topfi end ----#

                        fd.close()
                        self.valuesSet = 1
                except IOError,e:
                        if debug:
                                print "TopfieldVFD: setValues ", e

        def enableEthernet(self):
                self.ethEnabled = True

        def disableEthernet(self):
                self.ethEnabled = False
                try:
                        fd = open("/dev/fpc")
                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioEthBothOff)
                        fd.close()
                except IOError,e:
                        if debug:
                                print "TopfieldVFD: disableEthernet ", e

        def enableClock(self):
                self.clockEnabled = True
                self.clock = " "
                try:
                        fd = open("/dev/fpc")
                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioColonOn)
                        fd.close()
                except IOError,e:
                        if debug:
                                print "TopfieldVFD: enableClock ", e

        def disableClock(self):
                self.clockEnabled = False
                self.clock = " "
                try:
                        fd = open("/dev/fpc")
                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioColonOff)
                        fd.close()
                        open("/dev/fpsmall", "w").write("     ")
                except IOError,e:
                        if debug:
                                print "TopfieldVFD: disableClock ", e

        def regExpMatch(self, pattern, string):
                if string is None:
                        return None
                try:
                        return pattern.search(string).group()
                except AttributeError:
                        None

        def displayHddUsed(self):

                if debug:
                        print "TopfieldVFD: determine HDD usage"

                # determine the HDD usage
                used = 0;
                try:
                        f = statvfs(defaultMoviePath())
                        # there are 8 HDD segments in the VFD
                        used = (f.f_blocks - f.f_bavail) * 8 / f.f_blocks
                except:
                        used = 0;

                if self.hddUsed != used:
                        try:
                                fd = open("/dev/fpc")
                                if self.hddUsed > used:
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioHddClear)
                                fcntl.ioctl(fd.fileno(), ioIconCmd, ioHddUsage[used])
                                if used == 8:
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioHddFull)
                                fd.close();
                        except IOError,e:
                                self.hddUsed = used # dummy operation
                        self.hddUsed = used

        def handleTimer(self):
                #print "[ TopfieldVFD timer ]"
                if self.valuesSet == 0:
                        self.setValues()

                if self.clockEnabled:
                        clock = strftime("%k%M",localtime(time()))
                        if clock != self.clock:
                                self.clock = clock
                                try:
                                        open("/dev/fpsmall", "w").write(clock + "\0")
                                except IOError,e:
                                        if debug:
                                                print "TopfieldVFD: handleTimer (clock) ", e

                # check HDD periodically
                if self.hddCheckCounter < hddCheckPeriod:
                        self.hddCheckCounter += 1
                else:
                        self.hddCheckCounter = 0
                        self.displayHddUsed()

                if self.ethEnabled == False:
                        return

                result = open("/proc/net/dev").readlines()
                numRegExp = "[0-9]+"
                numPattern = re_compile(numRegExp)
                txPattern = re_compile("eth0:[ ]*" + numRegExp)
                for item in result:
                        tmp = self.regExpMatch(txPattern, item)
                        if tmp != None:
                                tmp = tmp[5:].lstrip()
                                try:
                                        fd = open("/dev/fpc")
                                        if self.txCount != tmp:
                                                fcntl.ioctl(fd.fileno(), ioIconCmd, ioEthLeftOn)
                                                fcntl.ioctl(fd.fileno(), ioIconCmd, ioEthRightOn)
                                                self.txCount = tmp
                                        else:
                                                fcntl.ioctl(fd.fileno(), ioIconCmd, ioEthBothOff)
                                        fd.close()
                                except IOError,e:
                                        if debug:
                                                print "TopfieldVFD: handleTimer (Ethernet) ", e
                                break

        def __evStart(self):
                self.__evSeekableStatusChanged()

        def getTimeshiftState(self):
                service = self.session.nav.getCurrentService()
                if service is None:
                        return False
                timeshift = service.timeshift()
                if timeshift is None:
                        return False
                return True

        def __evSeekableStatusChanged(self):
                tmp = self.getTimeshiftState()
                if tmp == self.tsEnabled:
                        return
                try:
                        fd = open("/dev/fpc")
                        if tmp:
                                print "[Timeshift enabled]"
                                fcntl.ioctl(fd.fileno(), ioIconCmd, ioTimeshiftOn)
                        else:
                                print "[Timeshift disabled]"
                                fcntl.ioctl(fd.fileno(), ioIconCmd, ioTimeshiftOff)
                        fd.close()
                except IOError,e:
                        if debug:
                                print "TopfieldVFD: __evSeekableStatusChanged ", e
                self.tsEnabled = tmp

        def gotRecordEvent(self, service, event):
                recs = self.session.nav.getRecordings()
                nrecs = len(recs)
                if nrecs == self.recNum:
                        return
                try:
                        fd = open("/dev/fpc")
                        if config.usage.blinking_display_clock_during_recording.value:
                                if nrecs > 1: # set rec 1+2 symbols
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioRecBothFlush)
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioClockFlush)
                                elif nrecs > 0: # set rec 1 symbol
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioRecBothOff)
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioClockFlush)
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioRec1Flush)
                                else:
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioClockOff)
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioRecBothOff)
                        else:
                                fcntl.ioctl(fd.fileno(), ioIconCmd, ioClockOff)
                                if nrecs > 1: # set rec 1+2 symbols
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioRecBothOn)
                                elif nrecs > 0: # set rec 1 symbol
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioRecBothOff)
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioRec1On)
                                else:
                                        fcntl.ioctl(fd.fileno(), ioIconCmd, ioRecBothOff)

                        fd.close()
                except IOError,e:
                        if debug:
                                print "TopfieldVFD: gotRecordEvent ", e
                self.recNum = nrecs

        def shutdown(self):
                self.abort()

        def abort(self):
                print "TopfieldVFD aborting"

def main(session, **kwargs):
        session.open(TopfieldVFDSetup)

tfVfd = None
gReason = -1
mySession = None

def controlTfVfd():
        global tfVfd
        global gReason
        global mySession

        if gReason == 0 and mySession != None and tfVfd == None:
                print "Starting TopfieldVFD"
                tfVfd = TopfieldVFD(mySession)
        elif gReason == 1 and tfVfd != None:
                print "Stopping TopfieldVFD"
                tfVfd.disableClock()
                tfVfd = None

def autostart(reason, **kwargs):
        global tfVfd
        global gReason
        global mySession

        if kwargs.has_key("session"):
                global my_global_session
                mySession = kwargs["session"]
        else:
                gReason = reason
        controlTfVfd()

def Plugins(**kwargs):
        return [ PluginDescriptor(name="TopfieldVFD", description="Change VFD display settings", where = PluginDescriptor.WHERE_PLUGINMENU, fnc=main),
                PluginDescriptor(where = [PluginDescriptor.WHERE_SESSIONSTART, PluginDescriptor.WHERE_AUTOSTART], fnc = autostart) ]
