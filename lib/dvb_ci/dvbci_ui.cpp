#include <lib/dvb_ci/dvbci_ui.h>
#include <lib/dvb_ci/dvbci.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <lib/base/init.h>
#include <lib/base/init_num.h>
#include <lib/base/eerror.h>
#include <lib/base/estring.h>

#define MAX_SLOTS 4

eDVBCI_UI *eDVBCI_UI::instance;

eDVBCI_UI::eDVBCI_UI()
	:eMMI_UI(MAX_SLOTS)
{
#ifdef __sh__
	eDebug("%s >", __func__);
#endif
	ASSERT(!instance);
	instance = this;
#ifdef __sh__
	eDebug("%s <", __func__);
#endif
}

eDVBCI_UI *eDVBCI_UI::getInstance()
{
	return instance;
}

void eDVBCI_UI::setInit(int slot)
{
#ifdef __sh__
	eDebug("%s >", __func__);
#endif
	eDVBCIInterfaces::getInstance()->initialize(slot);
#ifdef __sh__
	eDebug("%s <", __func__);
#endif
}

void eDVBCI_UI::setReset(int slot)
{
#ifdef __sh__
	eDebug("%s >", __func__);
#endif
	eDVBCIInterfaces::getInstance()->reset(slot);
#ifdef __sh__
	eDebug("%s <", __func__);
#endif
}

int eDVBCI_UI::startMMI(int slot)
{
#ifdef __sh__
	eDebug("%s >", __func__);
#endif
	eDVBCIInterfaces::getInstance()->startMMI(slot);
	return 0;
}

int eDVBCI_UI::stopMMI(int slot)
{
#ifdef __sh__
	eDebug("%s >", __func__);
#endif
	eDVBCIInterfaces::getInstance()->stopMMI(slot);
#ifdef __sh__
	eDebug("%s <", __func__);
#endif
	return 0;
}

int eDVBCI_UI::answerMenu(int slot, int answer)
{
#ifdef __sh__
	eDebug("%s >", __func__);
#endif
	eDVBCIInterfaces::getInstance()->answerText(slot, answer);
#ifdef __sh__
	eDebug("%s <", __func__);
#endif
	return 0;
}

int eDVBCI_UI::answerEnq(int slot, char *value)
{
#ifdef __sh__
	eDebug("%s >", __func__);
#endif
	eDVBCIInterfaces::getInstance()->answerEnq(slot, value);
#ifdef __sh__
	eDebug("%s <", __func__);
#endif
	return 0;
}

int eDVBCI_UI::cancelEnq(int slot)
{
#ifdef __sh__
	eDebug("%s >", __func__);
#endif
	eDVBCIInterfaces::getInstance()->cancelEnq(slot);
#ifdef __sh__
	eDebug("%s <", __func__);
#endif
	return 0;
}

int eDVBCI_UI::getMMIState(int slot)
{
#ifdef __sh__
	eDebug("%s ><", __func__);
#endif
	return eDVBCIInterfaces::getInstance()->getMMIState(slot);
}

int eDVBCI_UI::setClockRate(int slot, int rate)
{
#ifdef __sh__
	eDebug("%s ><", __func__);
#endif
	return eDVBCIInterfaces::getInstance()->setCIClockRate(slot, rate);
}

//FIXME: correct "run/startlevel"
eAutoInitP0<eDVBCI_UI> init_dvbciui(eAutoInitNumbers::rc, "DVB-CI UI");
