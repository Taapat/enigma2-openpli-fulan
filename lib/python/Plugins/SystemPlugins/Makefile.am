installdir = $(pkglibdir)/python/Plugins/SystemPlugins

SUBDIRS = AutoVolume SoftwareManager Satfinder \
	SkinSelector Videomode VideoTune DefaultServicesScanner \
	HdmiCEC NetworkWizard

if ! ENABLE_MINIMAL
SUBDIRS += Hotplug KeymapManager PositionerSetup SatelliteEquipmentControl \
	VideoEnhancement OSDPositionSetup OSD3DSetup WirelessLan FastScan DiseqcTester
endif

if HAVE_TEMPFANCONTROL
SUBDIRS += TempFanControl
endif

if DREAMBOX
SUBDIRS += FrontprocessorUpgrade NFIFlash
endif

install_PYTHON =	\
	__init__.py
