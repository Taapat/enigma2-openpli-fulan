#include <lib/gdi/glcddc.h>
#include <lib/gdi/lcd.h>
#include <lib/base/init.h>
#include <lib/base/init_num.h>

gLCDDC *gLCDDC::instance;

#ifdef __sh__
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
	lcd = new eDBoxLCD();
	instance=this;
	
	update=1;

	surface.x=lcd->size().width();
	surface.y=lcd->size().height();
	surface.stride=lcd->stride();
	surface.bypp=surface.stride / surface.x;
	surface.bpp=surface.bypp*8;
	surface.data=lcd->buffer();
	surface.clut.colors=0;
	surface.clut.data=0;

	m_pixmap = new gPixmap(&surface);

#ifdef __sh__
	clock_gettime(CLOCK_MONOTONIC, &last_update);
#endif
}

gLCDDC::~gLCDDC()
{
#ifndef __sh__
//konfetti: not sure why, but calling the destructor if external lcd (pearl)
//is selected e2 crashes. this is also true if the destructor does not contain
//any code !!!
	delete lcd;
#endif
	instance=0;
}

void gLCDDC::exec(const gOpcode *o)
{
	switch (o->opcode)
	{
#ifdef HAVE_TEXTLCD
	case gOpcode::renderText:
		if (o->parm.renderText->text)
		{
			lcd->renderText(gDC::m_current_offset,o->parm.renderText->text);
			free(o->parm.renderText->text);
		}
		delete o->parm.renderText;
		break;
#endif
	case gOpcode::flush:
#ifdef __sh__
//		if (time_after(last_update, 1500) && update)
		if (update)
		{
			lcd->update();
			clock_gettime(CLOCK_MONOTONIC, &last_update);
		}
#else
//		if (update)
			lcd->update();
#endif
	default:
		gDC::exec(o);
		break;
	}
}

void gLCDDC::setUpdate(int u)
{
	update=u;
}

eAutoInitPtr<gLCDDC> init_gLCDDC(eAutoInitNumbers::graphic-1, "gLCDDC");
