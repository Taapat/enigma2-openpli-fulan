installdir = $(pkglibdir)/python/Plugins/SystemPlugins

SUBDIRS = SoftwareManager Satfinder \
	SkinSelector Videomode VideoTune \
	HdmiCEC NetworkWizard

if ! ENABLE_MINIMAL
SUBDIRS += Hotplug KeymapManager PositionerSetup SatelliteEquipmentControl \
	VideoEnhancement OSDPositionSetup OSD3DSetup WirelessLan FastScan DiseqcTester \
	DefaultServicesScanner
endif

if HAVE_TEMPFANCONTROL
SUBDIRS += TempFanControl
endif

if DREAMBOX
SUBDIRS += FrontprocessorUpgrade NFIFlash
endif

install_PYTHON =	\
	__init__.py
