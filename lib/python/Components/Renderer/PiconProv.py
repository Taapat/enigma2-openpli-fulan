from Renderer import Renderer
from Tools.Directories import pathExists, SCOPE_SKIN_IMAGE, \
	SCOPE_CURRENT_SKIN, resolveFilename

from enigma import ePixmap
import os

searchPaths = []

def initPiconPaths():
	global searchPaths
	searchPaths = []
	for mp in ('/tmp/', '/media/hdd/', '/usr/share/enigma2/'):
		onMountpointAdded(mp)

def onMountpointAdded(mountpoint):
	global searchPaths
	try:
		piconPath = os.path.join(mountpoint, 'piconProv') + '/'
		if os.path.isdir(piconPath) and piconPath not in searchPaths:
			for fn in os.listdir(piconPath):
				if fn[-4:] == '.png':
					print "[PiconProv] adding path:", piconPath
					searchPaths.append(piconPath)
					break
	except Exception, ex:
		print "[PiconProv] Failed to investigate %s:" % mountpoint, ex

def findPicon(serviceName):
	for piconPath in searchPaths:
		if pathExists(piconPath):
			pngname = piconPath + serviceName + ".png"
			if pathExists(pngname):
				return pngname
	return ""

def getPiconName(serviceName):
	if serviceName == "Unknown":
		return ""
	sname = serviceName.upper()
	pngname = findPicon(sname)
	return pngname

class PiconProv(Renderer):
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
			if attrib == "path":
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
			if not pngname: # no picon for provider found
				pngname = self.defaultpngname
			if self.pngname != pngname:
				if pngname:
					#self.instance.setScale(1)
					self.instance.setPixmapFromFile(pngname)
					self.instance.show()
				else:
					self.instance.hide()
				self.pngname = pngname

initPiconPaths()

