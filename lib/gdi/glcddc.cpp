#include <lib/gdi/glcddc.h>
#include <lib/gdi/lcd.h>
#ifndef NO_LCD
#include <lib/gdi/fblcd.h>
#endif
#include <lib/base/init.h>
#include <lib/base/init_num.h>

gLCDDC *gLCDDC::instance;

#ifdef HAVE_GRAPHLCD
static inline int time_after(struct timespec oldtime, uint32_t delta_ms)
{
	// calculate the oldtime + add on the delta
	uint64_t oldtime_ms = (oldtime.tv_sec * 1000) + (oldtime.tv_nsec / 1000000);
	oldtime_ms += delta_ms;
	// calculate the nowtime
	struct timespec nowtime;
	clock_gettime(CLOCK_MONOTONIC, &nowtime);
	uint64_t nowtime_ms = (nowtime.tv_sec * 1000) + (nowtime.tv_nsec / 1000000);
	// check
	return nowtime_ms > oldtime_ms;
}
#endif

gLCDDC::gLCDDC()
{
#ifndef NO_LCD
	lcd = new eFbLCD();
	if (!lcd->detected())
	{
		delete lcd;
		lcd = new eDBoxLCD();
	}
#else
	lcd = new eDBoxLCD();
#endif
	instance = this;

	update = 1;

	surface.x = lcd->size().width();
	surface.y = lcd->size().height();
	surface.stride = lcd->stride();
	surface.bypp = surface.stride / surface.x;
	surface.bpp = surface.bypp*8;
	surface.data = lcd->buffer();
	surface.data_phys = 0;
	if (lcd->getLcdType() == 4)
	{
		surface.clut.colors = 256;
		surface.clut.data = new gRGB[surface.clut.colors];
		memset(surface.clut.data, 0, sizeof(*surface.clut.data)*surface.clut.colors);
	}
	else
	{
		surface.clut.colors = 0;
		surface.clut.data = 0;
	}
	eDebug("[gLCDDC] resolution: %dx%dx%d stride=%d", surface.x, surface.y, surface.bpp, surface.stride);

	m_pixmap = new gPixmap(&surface);
#ifdef HAVE_GRAPHLCD
	clock_gettime(CLOCK_MONOTONIC, &last_update);
#endif
}

gLCDDC::~gLCDDC()
{
#ifndef HAVE_GRAPHLCD
//konfetti: not sure why, but calling the destructor if external lcd (pearl) is selected
//e2 crashes. this is also true if the destructor does not contain any code !!!
	delete lcd;
#endif
	if (surface.clut.data)
		delete[] surface.clut.data;
	instance = 0;
}

void gLCDDC::exec(const gOpcode *o)
{
	switch (o->opcode)
	{
#ifndef NO_LCD
	case gOpcode::setPalette:
	{
		gDC::exec(o);
		lcd->setPalette(surface);
		break;
	}
#ifdef HAVE_TEXTLCD
	case gOpcode::renderText:
		if (o->parm.renderText->text)
		{
			lcd->renderText(gDC::m_current_offset, o->parm.renderText->text);
			free(o->parm.renderText->text);
		}
		delete o->parm.renderText;
		break;
#endif
#else
	case gOpcode::renderText:
		if (o->parm.renderText->text)
		{
			lcd->renderText(o->parm.renderText->text);
			free(o->parm.renderText->text);
		}
		delete o->parm.renderText;
		break;
#endif
	case gOpcode::flush:
#ifdef HAVE_GRAPHLCD
		if (update)
		{
			lcd->update();
			clock_gettime(CLOCK_MONOTONIC, &last_update);
		}
#else
		lcd->update();
#endif
	default:
		gDC::exec(o);
		break;
	}
}

void gLCDDC::setUpdate(int u)
{
	update = u;
}

eAutoInitPtr<gLCDDC> init_gLCDDC(eAutoInitNumbers::graphic-1, "gLCD");
