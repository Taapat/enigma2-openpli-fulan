#include <lib/gdi/lcd.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <lib/gdi/esize.h>
#include <lib/base/init.h>
#include <lib/base/init_num.h>
#ifdef HAVE_TEXTLCD
	#include <lib/base/estring.h>
#endif
#include <lib/gdi/glcddc.h>

eLCD *eLCD::instance;

eLCD::eLCD()
{
	_buffer = NULL;
	lcdfd = -1;
	locked = 0;
	instance = this;
}

eLCD *eLCD::getInstance()
{
	return instance;
}

void eLCD::setSize(int xres, int yres, int bpp)
{
	res = eSize(xres, yres);
	_buffer = new unsigned char[xres * yres * bpp/8];
	memset(_buffer, 0, res.height() * res.width() * bpp / 8);
	_stride = res.width() * bpp / 8;
	eDebug("[eLCD] (%dx%dx%d) buffer %p %d bytes, stride %d", xres, yres, bpp, _buffer, xres * yres * bpp / 8, _stride);
}

eLCD::~eLCD()
{
	if (_buffer)
		delete [] _buffer;
	instance = NULL;
}

int eLCD::lock()
{
	if (locked)
		return -1;

	locked = 1;
	return lcdfd;
}

void eLCD::unlock()
{
	locked = 0;
}

#ifndef NO_LCD
#ifdef HAVE_TEXTLCD
void eLCD::renderText(ePoint start, const char *text)
{
	if (lcdfd >= 0 && start.y() < 5)
	{
		std::string message = text;
		message = replace_all(message, "\n", " ");
		::write(lcdfd, message.c_str(), message.size());
	}
}
#endif
#else
void eLCD::renderText(const char *text)
{
	//eDebug("[LCD] text: %s", text);
	vfd->vfd_write_string(text, true);
}
#endif

#ifndef HAVE_GRAPHLCD
eDBoxLCD::eDBoxLCD()
{
	int xres = 32, yres = 32, bpp = 8;
	flipped = false;
	inverted = 0;
	lcd_type = 0;
#ifndef NO_LCD
	lcdfd = open("/dev/dbox/oled0", O_RDWR);
	if (lcdfd < 0)
	{
		if (!access("/proc/stb/lcd/oled_brightness", W_OK) ||
		    !access("/proc/stb/fp/oled_brightness", W_OK) )
			lcd_type = 2;
		lcdfd = open("/dev/dbox/lcd0", O_RDWR);
	}
	else
		lcd_type = 1;

	if (lcdfd < 0)
		eDebug("[eDboxLCD] No oled0 or lcd0 device found!");
	else
	{

#ifndef LCD_IOCTL_ASC_MODE
#define LCDSET                  0x1000
#define LCD_IOCTL_ASC_MODE	(21|LCDSET)
#define	LCD_MODE_ASC		0
#define	LCD_MODE_BIN		1
#endif

		int i = LCD_MODE_BIN;
		ioctl(lcdfd, LCD_IOCTL_ASC_MODE, &i);
		FILE *f = fopen("/proc/stb/lcd/xres", "r");
		if (f)
		{
			int tmp;
			if (fscanf(f, "%x", &tmp) == 1)
				xres = tmp;
			fclose(f);
			f = fopen("/proc/stb/lcd/yres", "r");
			if (f)
			{
				if (fscanf(f, "%x", &tmp) == 1)
					yres = tmp;
				fclose(f);
				f = fopen("/proc/stb/lcd/bpp", "r");
				if (f)
				{
					if (fscanf(f, "%x", &tmp) == 1)
						bpp = tmp;
					fclose(f);
				}
			}
			lcd_type = 3;
		}
		eDebug("[eDboxLCD] xres=%d, yres=%d, bpp=%d lcd_type=%d", xres, yres, bpp, lcd_type);
	}
#endif
	instance = this;

	setSize(xres, yres, bpp);
#ifdef NO_LCD
	vfd = new evfd;
#endif
}

void eDBoxLCD::setInverted(unsigned char inv)
{
	inverted = inv;
	update();
}

void eDBoxLCD::setFlipped(bool onoff)
{
	flipped = onoff;
	update();
}

int eDBoxLCD::setLCDContrast(int contrast)
{
#ifndef NO_LCD
	if (lcdfd < 0)
		return(0);

#ifndef LCD_IOCTL_SRV
#define LCDSET                  0x1000
#define	LCD_IOCTL_SRV			(10|LCDSET)
#endif
	eDebug("[eDboxLCD] setLCDContrast %d", contrast);

	int fp;
	if((fp = open("/dev/dbox/fp0", O_RDWR)) < 0)
	{
		eDebug("[eDboxLCD] can't open /dev/dbox/fp0: %m");
		return(-1);
	}

	if(ioctl(lcdfd, LCD_IOCTL_SRV, &contrast) < 0)
		eDebug("[eDboxLCD] can't set lcd contrast: %m");
	close(fp);
#endif
	return(0);
}

int eDBoxLCD::setLCDBrightness(int brightness)
{
#ifndef NO_LCD
	if (lcdfd < 0)
		return(0);

	eDebug("[eDboxLCD] setLCDBrightness %d", brightness);
	FILE *f = fopen("/proc/stb/lcd/oled_brightness", "w");
	if (!f)
		f = fopen("/proc/stb/fp/oled_brightness", "w");
	if (f)
	{
		if (fprintf(f, "%d", brightness) == 0)
			eDebug("[eDboxLCD] write /proc/stb/lcd|fp/oled_brightness failed: %m");
		fclose(f);
	}
	else
	{
		int fp;
		if ((fp = open("/dev/dbox/fp0", O_RDWR)) < 0)
		{
			eDebug("[eDboxLCD] can't open /dev/dbox/fp0: %m");
			return(-1);
		}
#ifndef FP_IOCTL_LCD_DIMM
#define FP_IOCTL_LCD_DIMM       3
#endif
		if (ioctl(fp, FP_IOCTL_LCD_DIMM, &brightness) < 0)
			eDebug("[eDboxLCD] can't set lcd brightness: %m");
		close(fp);
	}
#endif
	return(0);
}

eDBoxLCD::~eDBoxLCD()
{
#ifndef NO_LCD
	if (lcdfd >= 0)
	{
		close(lcdfd);
		lcdfd = -1;
	}
#else
	delete vfd;
#endif
}

void eDBoxLCD::update()
{
#ifndef NO_LCD
#ifndef HAVE_TEXTLCD
	if (lcdfd < 0)
		return;

	if (lcd_type == 0 || lcd_type == 2)
	{
		unsigned char raw[132 * 8];
		int x, y, yy;
		for (y = 0; y < 8; y++)
		{
			for (x = 0; x < 132; x++)
			{
				int pix = 0;
				for (yy = 0; yy < 8; yy++)
					pix |= (_buffer[(y * 8 + yy) * 132 + x] >= 108) << yy;
				if (flipped)
				{
					/* 8 pixels per byte, swap bits */
#define BIT_SWAP(a) (( ((a << 7)&0x80) + ((a << 5)&0x40) + ((a << 3)&0x20) + ((a << 1)&0x10) + ((a >> 1)&0x08) + ((a >> 3)&0x04) + ((a >> 5)&0x02) + ((a >> 7)&0x01) )&0xff)
					raw[(7 - y) * 132 + (131 - x)] = BIT_SWAP(pix ^ inverted);
				}
				else
					raw[y * 132 + x] = pix ^ inverted;
			}
		}
		write(lcdfd, raw, 132 * 8);
	}
	else if (lcd_type == 3)
	{
		/* for now, only support flipping / inverting for 8bpp displays */
		if ((flipped || inverted) && _stride == res.width())
		{
			unsigned int height = res.height();
			unsigned int width = res.width();
			unsigned char raw[_stride * height];
			for (unsigned int y = 0; y < height; y++)
			{
				for (unsigned int x = 0; x < width; x++)
				{
					if (flipped)
						/* 8bpp, no bit swapping */
						raw[(height - 1 - y) * width + (width - 1 - x)] = _buffer[y * width + x] ^ inverted;
					else
						raw[y * width + x] = _buffer[y * width + x] ^ inverted;
				}
			}
			write(lcdfd, raw, _stride * height);
		}
		else
			write(lcdfd, _buffer, _stride * res.height());
	}
	else /* lcd_type == 1 */
	{
		unsigned char raw[64 * 64];
		int x, y;
		memset(raw, 0, 64 * 64);
		for (y = 0; y < 64; y++)
		{
			int pix = 0;
			for (x = 0; x < 128 / 2; x++)
			{
				pix = (_buffer[y * 132 + x * 2 + 2] & 0xF0) | (_buffer[y * 132 + x * 2 + 1 + 2] >> 4);
				if (inverted)
					pix = 0xFF - pix;
				if (flipped)
				{
					/* device seems to be 4bpp, swap nibbles */
					unsigned char byte;
					byte = (pix >> 4) & 0x0f;
					byte |= (pix << 4) & 0xf0;
					raw[(63 - y) * 64 + (63 - x)] = byte;
				}
				else
					raw[y * 64 + x] = pix;
			}
		}
		write(lcdfd, raw, 64 * 64);
	}
#endif
#endif
}

#else

void eDBoxLCD::setFlipped(bool onoff)
{
	flipped = onoff;
	update();
}

/* **************************************************************** */
/* Pearl LCD */

eDBoxLCD::eDBoxLCD()
{
	eDebug("eDBoxLCD::eDBoxLCD >");

	displayNumber = 0;
	is_oled = 1;

	instance=this;

	if (GLCD::Config.Load("/etc/graphlcd.conf") == false)
	{
		eDebug("Error loading config file!\n");
		return;
	}
	if (GLCD::Config.driverConfigs.size() <= 0)
	{
		eDebug("ERROR: No displays specified in config file!\n");
	}

	GLCD::Config.driverConfigs[displayNumber].upsideDown ^= 0;
	GLCD::Config.driverConfigs[displayNumber].invert ^= 0;

	lcd = GLCD::CreateDriver(GLCD::Config.driverConfigs[displayNumber].id, &GLCD::Config.driverConfigs[displayNumber]);

	if (!lcd)
	{
		eDebug("ERROR: Failed creating display object\n");
		return;
	}
	if (lcd->Init() != 0)
	{
#if 0
	// Returning an error here will break the code at various other places
		eDebug("ERROR: Failed initializing display\n");
		delete lcd;
		lcd = NULL;
		return;
#endif
	}
	lcd->SetBrightness(GLCD::Config.driverConfigs[displayNumber].brightness);

	lcd->GetFeature((std::string) "depth", depth);
	width = GLCD::Config.driverConfigs[displayNumber].width;
	height = GLCD::Config.driverConfigs[displayNumber].height;

	eDebug("config -> (w %d, h %d)", width, height);

	bitmap = new GLCD::cBitmap(width, height);
	bitmap->Clear();

	lcd->SetScreen(bitmap->Data(), bitmap->Width(), bitmap->Height());
	lcd->Refresh(true);

	lcdfd = 1; //needed for detected()
	setSize(width, height, depth);

	eDebug("eDBoxLCD::eDBoxLCD (w %d, h %d, depth %d)<", width, height, depth);
}

void eDBoxLCD::setInverted(unsigned char inv)
{
	eDebug("eDBoxLCD::setInverted");
	inverted = inv;
	update();
}

int eDBoxLCD::setLCDContrast(int contrast)
{
	eDebug("[LCD] setLCDContrast not supported");
	return(0);
}

int eDBoxLCD::setLCDBrightness(int brightness)
{
	eDebug("eDBoxLCD::setLCDBrightness");
/* fixme range check */
	lcd->SetBrightness(brightness);
	return(0);
}

eDBoxLCD::~eDBoxLCD()
{
	eDebug("eDBoxLCD::~eDBoxLCD");
}

eDBoxLCD *eDBoxLCD::getInstance()
{
	eDebug("eDBoxLCD::getInstance");
	return instance;
}

void eDBoxLCD::update()
{
	if (lcdfd == 1)
	{
		bitmap->Clear();
		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
			{
				uint16_t *buf16  = (uint16_t*) _buffer;
#if BYTE_ORDER == LITTLE_ENDIAN
				uint16_t col16 = bswap_16(*((uint16_t*)(((uint16_t*)buf16) + y * width + x)));
#else
				uint16_t col16 = *((uint16_t*)(((uint16_t*)buf16) + y * width + x));
#endif
				uint8_t red, green, blue, alpha; 
				uint32_t color32;

				/* BBBBB GGGGGG RRRRR */
				blue  = ((col16 & 0xF800) >> 11) * ( 255 / 31);
				green = ((col16 & 0x7E0) >> 5) * (255 / 63);
				red   = (col16 & 0x1f) * (255 / 31);
				alpha = 255;

				color32 = alpha << 24 | red << 16 | green << 8 | blue;

				if (inverted)
					color32 = 0xFFFFFF - color32;

				bitmap->DrawPixel(x, y, color32);
			}
		lcd->SetScreen(bitmap->Data(), bitmap->Width(), bitmap->Height());
		lcd->Refresh(false); /* partial update */
	}
}
#endif
