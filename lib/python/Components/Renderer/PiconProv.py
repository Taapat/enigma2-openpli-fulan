from Renderer import Renderer
from Tools.Directories import fileExists, pathExists, SCOPE_SKIN_IMAGE, \
	SCOPE_CURRENT_SKIN, resolveFilename 

from enigma import ePixmap
from os import path, listdir

searchPaths = []

def initPiconPaths():
	global searchPaths
	searchPaths = []
	for mp in ('/tmp/', '/media/hdd/', '/usr/share/enigma2/'):
		onMountpointAdded(mp)

def onMountpointAdded(mountpoint):
	global searchPaths
	try:
		piconPath = path.join(mountpoint, 'piconProv') + '/'
		if path.isdir(piconPath) and piconPath not in searchPaths:
			for fn in listdir(piconPath):
				if fn.endswith('.png'):
					print "[PiconProv] adding path:", piconPath
					searchPaths.append(piconPath)
					break
	except Exception, ex:
		print "[PiconProv] Failed to investigate %s:" % mountpoint, ex

def findPicon(serviceName):
	for path in searchPaths:
		if pathExists(path):
			pngname = path + serviceName + ".png"
			if pathExists(pngname):
				return pngname
	return ""

class PiconProv(Renderer):
	__module__ = __name__

	def __init__(self):
		Renderer.__init__(self)
		self.path = 'piconProv'
		self.nameCache = {}
		self.pngname = ''

	def applySkin(self, desktop, parent):
		attribs = []
		for (attrib, value,) in self.skinAttributes:
			if (attrib == 'path'):
				self.path = value
			else:
				attribs.append((attrib,
				 value))
		self.skinAttributes = attribs
		return Renderer.applySkin(self, desktop, parent)

	GUI_WIDGET = ePixmap

	def changed(self, what):
		if self.instance:
			pngname = ''
			if (what[0] != self.CHANGED_CLEAR):
				sname = self.source.text
				sname = sname.upper()
				pngname = self.nameCache.get(sname, '')
				if (pngname == ''):
					pngname = findPicon(sname)
					if (pngname != ''):
						self.nameCache[sname] = pngname
			if (pngname == ''):
				pngname = self.nameCache.get('default', '')
				if (pngname == ''):
					pngname = findPicon('picon_default')
					if (pngname == ''):
						tmp = resolveFilename(SCOPE_CURRENT_SKIN, 'picon_default.png')
						if fileExists(tmp):
							pngname = tmp
						else:
							pngname = resolveFilename(SCOPE_SKIN_IMAGE, 'skin_default/picon_default.png')
					self.nameCache['default'] = pngname
			if (self.pngname != pngname):
				self.pngname = pngname
				self.instance.setPixmapFromFile(self.pngname)

initPiconPaths()
