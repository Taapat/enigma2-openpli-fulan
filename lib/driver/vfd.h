#ifndef VFD_H_
#define VFD_H_

#define ICON_ON  1
#define ICON_OFF 0

typedef enum { PLAYFASTBACKWARD=1, PLAYHEAD, PLAYLOG,  PLAYTAIL, PLAYFASTFORWARD, PLAYPAUSE, REC1, MUTE,
	CYCLE, DUBI, CA, CI, USB, DOUBLESCREEN, REC2, HDD_A8, HDD_A7, HDD_A6, HDD_A5,HDD_A4, HDD_A3,
	HDD_FULL, HDD_A2, HDD_A1, MP3, AC3, TVMODE_LOG, AUDIO, ALERT, HDD_A9, CLOCK_PM, CLOCK_AM, CLOCK,
	TIME_SECOND, DOT2, STANDBY, TER, DISK_S3, DISK_S2, DISK_S1, DISK_S0, SAT, TIMESHIFT, DOT1, CAB } tvfd_icon;

class evfd
{
protected:
	static evfd *instance;
	int file_vfd;
	int vfd_type;
#ifdef SWIG
	evfd();
	~evfd();
#endif
public:
#ifndef SWIG
	evfd();
	~evfd();
#endif
	void init();
	static evfd* getInstance();

	int getVfdType() { return vfd_type; }
	void vfd_set_SCROLL(int id);
	void vfd_set_CENTER(bool id);
	void vfd_set_icon(tvfd_icon id, bool onoff);
	void vfd_set_icon(tvfd_icon id, bool onoff, bool force);
	void vfd_clear_icons();

	void vfd_write_string(const char * string);
	void vfd_write_string(const char * str, bool force);
	void vfd_write_string_scrollText(const char* text);
	void vfd_clear_string();
	
	void vfd_set_brightness(unsigned char setting);
	void vfd_set_light(bool onoff);
	void vfd_set_fan(bool onoff);
};

#endif
