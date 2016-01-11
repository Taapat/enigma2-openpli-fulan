import os

def enumFeeds():
	for fn in os.listdir('/etc/opkg'):
		if fn[-10:] == '-feed.conf':
			try:
				for feed in open(os.path.join('/etc/opkg', fn)):
					yield feed.split()[1]
			except IndexError:
				pass
			except IOError:
				pass

def enumPlugins(filter_start=''):
	list_dir = listsDirPath()
	for feed in enumFeeds():
		package = None
		try:
			for line in open(os.path.join(list_dir, feed), 'r'):
				if line[:8] == 'Package:':
					package = line.split(":",1)[1].strip()
					version = ''
					description = ''
					if package[:len(filter_start)] == filter_start and package[-4:] != '-dev' and package[-10:] != '-staticdev' and package[-4:] != '-dbg' and package[-4:] != '-doc' and package[-4:] != '-src':
						continue
					package = None
				if package is None:
					continue
				if line[:8] == 'Version:':
					version = line.split(":",1)[1].strip()
				elif line[:12] == 'Description:':
					description = line.split(":",1)[1].strip()
				elif description and line[0] == ' ':
					description += line[:-1]
				elif len(line) <= 1:
					d = description.split(' ',3)
					if len(d) > 3:
						# Get rid of annoying "version" and package repeating strings
						if d[1] == 'version':
							description = d[3]
						if description[:10] == 'gitAUTOINC':
							description = description.split(' ',1)[1]
					yield package, version, description.strip()
					package = None
		except IOError:
			pass

def listsDirPath():
	try:
		for line in open('/etc/opkg/opkg.conf', "r"):
			if line[:9] == 'lists_dir':
				return line.replace('\n','').split(' ')[2]
	except IOError:
		print "[opkg] cannot open %s" % path
	return '/var/lib/opkg/lists'

if __name__ == '__main__':
	for p in enumPlugins('enigma'):
		print p
