import os, re, unicodedata
from enigma import ePixmap

from Components.Harddisk import harddiskmanager
from Renderer import Renderer
from ServiceReference import ServiceReference
from Tools.Alternatives import GetWithAlternative
from Tools.Directories import pathExists, SCOPE_SKIN_IMAGE, \
	SCOPE_CURRENT_SKIN, resolveFilename


searchPaths = []
if pathExists('/tmp/picon/'):
	piconInTmp = True
	lastPiconPath = '/tmp/picon/'
	print "[Picon] use path:", lastPiconPath
else:
	piconInTmp = False
	lastPiconPath = None

def initPiconPaths():
	global searchPaths
	searchPaths = []
	for mp in ('/tmp/', '/media/hdd/', '/usr/share/enigma2/', '/'):
		onMountpointAdded(mp)
	for part in harddiskmanager.getMountedPartitions():
		onMountpointAdded(part.mountpoint)

def onMountpointAdded(mountpoint):
	global searchPaths
	try:
		piconPath = os.path.join(mountpoint, 'picon') + '/'
		if os.path.isdir(piconPath) and piconPath not in searchPaths:
			for fn in os.listdir(piconPath):
				if fn[-4:] == '.png':
					print "[Picon] adding path:", piconPath
					searchPaths.append(piconPath)
					break
	except Exception, ex:
		print "[Picon] Failed to investigate %s:" % mountpoint, ex

def onMountpointRemoved(mountpoint):
	global searchPaths
	path = os.path.join(mountpoint, 'picon') + '/'
	try:
		searchPaths.remove(path)
		print "[Picon] removed path:", path
	except:
		pass

def onPartitionChange(why, part):
	if why is 'add':
		onMountpointAdded(part.mountpoint)
	elif why is 'remove':
		onMountpointRemoved(part.mountpoint)

def findPicon(serviceName):
	global lastPiconPath
	if lastPiconPath:
		pngname = lastPiconPath + serviceName + ".png"
		if pathExists(pngname):
			return pngname
	if not piconInTmp:
		for piconPath in searchPaths:
			pngname = piconPath + serviceName + ".png"
			if pngname:
				if pathExists(pngname):
					lastPiconPath = piconPath
					return pngname
	return ""

def getPiconName(serviceName):
	#remove the path and name fields, and replace ':' by '_'
	name = '_'.join(GetWithAlternative(serviceName).split(':', 10)[:10])
	pngname = findPicon(name)
	if not pngname:
		fields = name.split('_', 3)
		if len(fields) > 2:
			if fields[0] == '4097' or fields[0] == '5002':
				#fallback to 1 for iptv gstreeamer services
				fields[0] = '1'
				pngname = findPicon('_'.join(fields))
			if not pngname and fields[2] != '2':
				#fallback to 1 for tv services with nonstandard servicetypes
				fields[2] = '1'
				pngname = findPicon('_'.join(fields))
	if not pngname: # picon by channel name
		name = ServiceReference(serviceName).getServiceName()
		name = unicodedata.normalize('NFKD', unicode(name, 'utf_8', errors='ignore')).encode('ASCII', 'ignore')
		name = re.sub('[^a-z0-9]', '', name.replace('&', 'and').replace('+', 'plus').replace('*', 'star').lower())
		if name:
			pngname = findPicon(name)
			if not pngname and len(name) > 2 and name.endswith('hd'):
				pngname = findPicon(name[:-2])
	return pngname

class Picon(Renderer):
	def __init__(self):
		Renderer.__init__(self)
		self.pngname = ""
		pngname = findPicon("picon_default")
		self.defaultpngname = None
		if not pngname:
			tmp = resolveFilename(SCOPE_CURRENT_SKIN, "picon_default.png")
			if pathExists(tmp):
				pngname = tmp
			else:
				pngname = resolveFilename(SCOPE_SKIN_IMAGE, "skin_default/picon_default.png")
		if os.path.getsize(pngname):
			self.defaultpngname = pngname

	def addPath(self, value):
		if pathExists(value):
			global searchPaths
			if value[-1] != '/':
				value += '/'
			if value not in searchPaths:
				searchPaths.append(value)

	def applySkin(self, desktop, parent):
		attribs = self.skinAttributes[:]
		for (attrib, value) in self.skinAttributes:
			if attrib is "path":
				self.addPath(value)
				attribs.remove((attrib,value))
		self.skinAttributes = attribs
		return Renderer.applySkin(self, desktop, parent)

	GUI_WIDGET = ePixmap

	def changed(self, what):
		if self.instance:
			pngname = ""
			if what[0] != self.CHANGED_CLEAR:
				pngname = getPiconName(self.source.text)
			if not pngname: # no picon for service found
				pngname = self.defaultpngname
			if self.pngname is not pngname:
				if pngname:
					#self.instance.setScale(1)
					self.instance.setPixmapFromFile(pngname)
					self.instance.show()
				else:
					self.instance.hide()
				self.pngname = pngname

if not piconInTmp:
	harddiskmanager.on_partition_list_change.append(onPartitionChange)
	initPiconPaths()

