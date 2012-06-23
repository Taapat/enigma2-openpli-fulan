#ifndef VFD_H_
#define VFD_H_

#define ICON_ON  1
#define ICON_OFF 0

#ifndef PLATFORM_HDBOX
typedef enum { USB = 0x10, HD, HDD, LOCK, BT, MP3, MUSIC, DD, MAIL, MUTE, PLAY, PAUSE, FF, FR, REC, CLOCK } tvfd_icon;
#else
typedef enum { USB = 0x10, STANDBY, SAT, REC, TIMESHIFT, TIMER, HD, LOCK, DD, MUTE, TUNER1, TUNER2, MP3, REPEAT,
			   PLAY, PAUSE, TER, FILE, 480i, 480p, 576i, 576p, 720p, 1080i, 1080p } tvfd_icon;
#endif

class evfd
{
protected:
	static evfd *instance;
	int file_vfd;
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

        void vfd_set_SCROLL(int id);
	void vfd_set_CENTER(bool id);
	void vfd_set_icon(tvfd_icon id, bool onoff);
	void vfd_set_icon(tvfd_icon id, bool onoff, bool force);
	void vfd_clear_icons();

	void vfd_write_string(char * string);
	void vfd_write_string(char * str, bool force);
	void vfd_write_string_scrollText(char* text);
	void vfd_clear_string();
	
	void vfd_set_brightness(unsigned char setting);
	void vfd_set_light(bool onoff);
    void vfd_set_fan(bool onoff);
};


#endif
