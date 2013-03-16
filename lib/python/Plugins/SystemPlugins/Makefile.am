installdir = $(pkglibdir)/python/Plugins/SystemPlugins

SUBDIRS = SoftwareManager PositionerSetup Satfinder \
	SkinSelector SatelliteEquipmentControl Videomode VideoTune Hotplug \
	KeymapManager DefaultServicesScanner DiseqcTester \
	FastScan OSDPositionSetup OSD3DSetup HdmiCEC \
	VideoEnhancement NetworkWizard VFD-Icons OnErrorRestartService

if ! ENABLE_MINIMAL
SUBDIRS += WirelessLan
endif

if HAVE_TEMPFANCONTROL
SUBDIRS += TempFanControl
endif

if DREAMBOX
SUBDIRS += FrontprocessorUpgrade NFIFlash
endif

install_PYTHON =	\
	__init__.py
