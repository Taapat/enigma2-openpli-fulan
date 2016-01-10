# -*- coding: utf-8 -*-
import sys, os, time
from Tools.HardwareInfo import HardwareInfo

def getVersionString():
	return getImageVersionString()

def getImageVersionString():
	try:
		if os.path.isfile('/var/lib/opkg/status'):
			st = os.stat('/var/lib/opkg/status')
		else:
			st = os.stat('/usr/lib/opkg/status')
		tm = time.localtime(st.st_mtime)
		if tm.tm_year >= 2011:
			return time.strftime("%Y-%m-%d %H:%M:%S", tm)
	except:
		pass
	return _("unavailable")

def getFlashDateString():
	try:
		return time.strftime(_("%Y-%m-%d %H:%M"), time.localtime(os.stat("/boot").st_ctime))
	except:
		return _("unknown")

def getEnigmaVersionString():
	import enigma
	enigma_version = enigma.getEnigmaVersionString()
	if 'detached' in enigma_version:
		enigma_version = enigma_version[:12] + enigma_version.split('/')[1]
	return enigma_version

def getGStreamerVersionString():
	import enigma
	return enigma.getGStreamerVersionString()

def getKernelVersionString():
	try:
		return open("/proc/version","r").read().split(' ', 4)[2].split('-',2)[0]
	except:
		return _("unknown")

def getHardwareTypeString():
	return HardwareInfo().get_device_string()

def getImageTypeString():
	try:
		return "Based on " + open("/etc/issue").readlines()[-2].capitalize().strip()[:-6]
	except:
		return _("undefined")

def getCPUInfoString():
	try:
		for line in open("/proc/cpuinfo").readlines():
			line = [x.strip() for x in line.strip().split(":")]
			if line[0] == "cpu type":
				processor = line[1].split()[0]
			elif line[0] == "bogomips":
				cpu_speed = "%1.0f" % float(line[1])
		return "%s %s MHz" % (processor, cpu_speed)
	except:
		return _("undefined")

def getDriverInstalledDate():
	try:
		driver = os.popen("opkg list-installed | grep kernel-module-player2").read().strip()
		driver = driver.split("git")[1].split("-")[0]
		return  "%s-%s-%s" % (driver[:4], driver[4:6], driver[6:])
	except:
		return _("unknown")
def getPythonVersionString():
	try:
		import commands
		status, output = commands.getstatusoutput("python -V")
		return output.split(' ')[1]
	except:
		return _("unknown")

# For modules that do "from About import about"
about = sys.modules[__name__]
