installdir = $(pkglibdir)/python/Plugins/SystemPlugins

SUBDIRS = AutoVolume SoftwareManager Satfinder \
	SkinSelector Videomode VideoTune Hotplug \
	DefaultServicesScanner DiseqcTester \
	FastScan HdmiCEC \
	NetworkWizard VFD-Icons OnErrorRestartService

if ! ENABLE_MINIMAL
SUBDIRS += KeymapManager PositionerSetup SatelliteEquipmentControl \
	VideoEnhancement OSDPositionSetup OSD3DSetup WirelessLan
endif

if HAVE_TEMPFANCONTROL
SUBDIRS += TempFanControl
endif

if DREAMBOX
SUBDIRS += FrontprocessorUpgrade NFIFlash
endif

install_PYTHON =	\
	__init__.py
