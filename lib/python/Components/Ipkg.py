import os
from enigma import eConsoleAppContainer
from Components.Harddisk import harddiskmanager

opkgDestinations = []
opkgStatusPath = ''

def opkgExtraDestinations():
	global opkgDestinations
	return ''.join([" --dest %s:%s" % (i,i) for i in opkgDestinations])

def opkgAddDestination(mountpoint):
	pass
	#global opkgDestinations
	#if mountpoint not in opkgDestinations:
		#opkgDestinations.append(mountpoint)
		#print "[Ipkg] Added to OPKG destinations:", mountpoint

def onPartitionChange(why, part):
	global opkgDestinations
	global opkgStatusPath
	mountpoint = os.path.normpath(part.mountpoint)
	if mountpoint and mountpoint != '/':
		if why == 'add':
			if opkgStatusPath == '':
				# older opkg versions
				opkgStatusPath = 'usr/lib/opkg/status'
				if not os.path.exists(os.path.join('/', opkgStatusPath)):
					# recent opkg versions
					opkgStatusPath = 'var/lib/opkg/status'
			if os.path.exists(os.path.join(mountpoint, opkgStatusPath)):
				opkgAddDestination(mountpoint)
		elif why == 'remove':
			try:
				opkgDestinations.remove(mountpoint)
				print "[Ipkg] Removed from OPKG destinations:", mountpoint
			except:
				pass

harddiskmanager.on_partition_list_change.append(onPartitionChange)
for part in harddiskmanager.getMountedPartitions():
	onPartitionChange('add', part)

class IpkgComponent:
	EVENT_INSTALL = 0
	EVENT_DOWNLOAD = 1
	EVENT_INFLATING = 2
	EVENT_CONFIGURING = 3
	EVENT_REMOVE = 4
	EVENT_UPGRADE = 5
	EVENT_LISTITEM = 9
	EVENT_DONE = 10
	EVENT_ERROR = 11
	EVENT_MODIFIED = 12

	CMD_INSTALL = 0
	CMD_LIST = 1
	CMD_REMOVE = 2
	CMD_UPDATE = 3
	CMD_UPGRADE = 4
	CMD_UPGRADE_LIST = 5

	def __init__(self, ipkg = 'opkg'):
		self.ipkg = ipkg
		self.cmd = eConsoleAppContainer()
		self.cache = None
		self.callbackList = []
		self.setCurrentCommand()

	def setCurrentCommand(self, command = None):
		self.currentCommand = command

	def runCmdEx(self, cmd):
		self.runCmd(opkgExtraDestinations() + ' ' + cmd)

	def runCmd(self, cmd):
		print "executing", self.ipkg, cmd
		self.cmd.appClosed.append(self.cmdFinished)
		self.cmd.dataAvail.append(self.cmdData)
		if self.cmd.execute(self.ipkg + " " + cmd):
			self.cmdFinished(-1)

	def startCmd(self, cmd, args = None):
		if cmd is self.CMD_UPDATE:
			self.runCmdEx("update")
		elif cmd is self.CMD_UPGRADE:
			append = ""
			if args["test_only"]:
				append = " -test"
			self.runCmdEx("upgrade" + append)
		elif cmd is self.CMD_LIST:
			self.fetchedList = []
			if args['installed_only']:
				self.runCmdEx("list_installed")
			else:
				self.runCmd("list")
		elif cmd is self.CMD_INSTALL:
			self.runCmd("install " + args['package'])
		elif cmd is self.CMD_REMOVE:
			self.runCmd("remove " + args['package'])
		elif cmd is self.CMD_UPGRADE_LIST:
			self.fetchedList = []
			self.runCmdEx("list_upgradable")
		self.setCurrentCommand(cmd)

	def cmdFinished(self, retval):
		self.callCallbacks(self.EVENT_DONE)
		self.cmd.appClosed.remove(self.cmdFinished)
		self.cmd.dataAvail.remove(self.cmdData)

	def cmdData(self, data):
		print "data:", data
		if self.cache is None:
			self.cache = data
		else:
			self.cache += data

		if '\n' in data:
			splitcache = self.cache.split('\n')
			if self.cache[-1] == '\n':
				iteration = splitcache
				self.cache = None
			else:
				iteration = splitcache[:-1]
				self.cache = splitcache[-1]
			for mydata in iteration:
				if mydata != '':
					self.parseLine(mydata)

	def parseLine(self, data):
		try:
			if data.startswith('Not selecting'):
				return
			if self.currentCommand in (self.CMD_LIST, self.CMD_UPGRADE_LIST):
				item = data.split(' - ', 2)
			if len(item) < 3:
				self.callCallbacks(self.EVENT_ERROR, None)
				return
				self.fetchedList.append(item)
				self.callCallbacks(self.EVENT_LISTITEM, item)
				return
			if data[:11] == 'Downloading':
				self.callCallbacks(self.EVENT_DOWNLOAD, data.split(' ', 5)[1].strip())
			elif data[:9] == 'Upgrading':
				self.callCallbacks(self.EVENT_UPGRADE, data.split(' ', 2)[1])
			elif data[:10] == 'Installing':
				self.callCallbacks(self.EVENT_INSTALL, data.split(' ', 2)[1])
			elif data[:8] == 'Removing':
				self.callCallbacks(self.EVENT_REMOVE, data.split(' ', 3)[2])
			elif data[:11] == 'Configuring':
				self.callCallbacks(self.EVENT_CONFIGURING, data.split(' ', 2)[1])
			elif data[:17] == 'An error occurred':
				self.callCallbacks(self.EVENT_ERROR, None)
			elif data[:18] == 'Failed to download':
				self.callCallbacks(self.EVENT_ERROR, None)
			elif data[:21] == 'ipkg_download: ERROR:':
				self.callCallbacks(self.EVENT_ERROR, None)
			elif 'Configuration file \'' in data:
				# Note: the config file update question doesn't end with a newline, so
				# if we get multiple config file update questions, the next ones
				# don't necessarily start at the beginning of a line
				self.callCallbacks(self.EVENT_MODIFIED, data.split(' \'', 3)[1][:-1])
		except Exception, ex:
			print "[Ipkg] Failed to parse: '%s'" % data
			print "[Ipkg]", ex

	def callCallbacks(self, event, param = None):
		for callback in self.callbackList:
			callback(event, param)

	def addCallback(self, callback):
		self.callbackList.append(callback)

	def removeCallback(self, callback):
		self.callbackList.remove(callback)

	def getFetchedList(self):
		return self.fetchedList

	def stop(self):
		self.cmd.kill()

	def isRunning(self):
		return self.cmd.running()

	def write(self, what):
		if what:
			# We except unterminated commands
			what += "\n"
			self.cmd.write(what, len(what))
