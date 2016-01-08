installdir = $(pkglibdir)/python/Plugins/Extensions

SUBDIRS = TuxboxPlugins CutListEditor DVDPlayer PicturePlayer MediaScanner MediaPlayer GraphMultiEPG SocketMMI

if ! ENABLE_MINIMAL
SUBDIRS += ModemSettings
endif

install_PYTHON =	\
	__init__.py
