#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/stat.h>
#include  <pthread.h>

#include <lib/base/eerror.h>
#include <lib/driver/vfd.h>

#define VFD_DEVICE "/dev/vfd"
#define VFDICONDISPLAYONOFF   0xc0425a0a
#define VFDDISPLAYCHARS       0xc0425a00
#define VFDBRIGHTNESS         0xc0425a03
//light on off
#define VFDDISPLAYWRITEONOFF  0xc0425a05
#define VFDDISPLAYCLR		0xc0425b00
#define VFDLENGTH 16

bool startloop_running = false;
static bool icon_onoff[45];
static pthread_t thread_start_loop = 0;
void * start_loop (void *arg);
bool blocked = false;
bool requested = false;
bool VFD_CENTER = false;
bool scoll_loop = false;
int VFD_SCROLL = 1;

char chars[64];
char g_str[64];

struct vfd_ioctl_data
{
	unsigned char start;
	unsigned char data[64];
	unsigned char length;
};

#if defined(PLATFORM_SPARK) || defined(PLATFORM_SPARK7162)
struct set_icon_s {
	int icon_nr;
	int on;
};
#endif

evfd* evfd::instance = NULL;

evfd* evfd::getInstance()
{
	if (instance == NULL)
		instance = new evfd;
	return instance;
}

evfd::evfd()
{
	file_vfd = 0;
	memset ( chars, ' ', 63 );
	vfd_type=4;
	eDebug("--debug-vfd->%d", vfd_type);
	FILE *vfd_proc = fopen ("/proc/aotom/display_type", "r");
	if (vfd_proc)
	{	char buf[2];
		fread(&buf,sizeof(buf),1,vfd_proc);
		vfd_type=atoi(&buf[0]);
		fclose (vfd_proc);	    
	}
	eDebug("--debug-vfd->%d", vfd_type);
}

void evfd::init()
{
	pthread_create (&thread_start_loop, NULL, &start_loop, NULL);
	return;
}

evfd::~evfd()
{
	//close (file_vfd);
}

void * start_loop (void *arg)
{
	evfd vfd;
	blocked = true;
	//vfd.vfd_clear_icons();
	vfd.vfd_write_string("Open AR-P ENIGMA2", true);
	//run 2 times through all icons 
	memset(&icon_onoff,0, sizeof(icon_onoff));
	for (int vloop = 0; vloop < 128; vloop++)
	{
		if (vloop%14 == 0 )
			vfd.vfd_set_brightness(1);
		else if (vloop%14 == 1 )
			vfd.vfd_set_brightness(2);
		else if (vloop%14 == 2 )
			vfd.vfd_set_brightness(3);
		else if (vloop%14 == 3 )
			vfd.vfd_set_brightness(4);
		else if (vloop%14 == 4 )
			vfd.vfd_set_brightness(5);
		else if (vloop%14 == 5 )
			vfd.vfd_set_brightness(6);
		else if (vloop%14 == 6 )
			vfd.vfd_set_brightness(7);
		else if (vloop%14 == 7 )
			vfd.vfd_set_brightness(6);
		else if (vloop%14 == 8 )
			vfd.vfd_set_brightness(5);
		else if (vloop%14 == 9 )
			vfd.vfd_set_brightness(4);
		else if (vloop%14 == 10 )
			vfd.vfd_set_brightness(3);
		else if (vloop%14 == 11 )
			vfd.vfd_set_brightness(2);
		else if (vloop%14 == 12 )
			vfd.vfd_set_brightness(1);
		else if (vloop%14 == 13 )
			vfd.vfd_set_brightness(0);
		usleep(75000);
	}
	vfd.vfd_set_brightness(7);
	blocked = false;
	return NULL;
}

#if 0//defined(PLATFORM_SPARK7162)
void evfd::vfd_write_string_scrollText(char* text)
{
	return;
}

//we can not use a member function (vfd_write_string_scrollText) in pthread, so we use a second (same content) non member function (vfd_write_string_scrollText1)
static void *vfd_write_string_scrollText1(void *arg)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	bool scoll_loop = true;
	char out[VFDLENGTH+1];
	int i, len;
	evfd vfd;
	len = strlen((char *) g_str);
	memset(out, 0, VFDLENGTH+1);
	while (scoll_loop && (len > VFDLENGTH))
	{
		if (blocked)
		{
			usleep(250000);
		}
		else
		{
			scoll_loop = false;
		}
		for (i=0; i<=(len-VFDLENGTH); i++)
		{
			if (blocked)
			{
				memset(out, ' ', VFDLENGTH);
				memcpy(out, g_str+i, VFDLENGTH);
				vfd.vfd_write_string(out,true);
				usleep(250000);
			}
			else
			{
				scoll_loop = false;
				i = len-VFDLENGTH;
			}
		}
		for (i=1; i < VFDLENGTH; i++)
		{
			if (blocked)
			{
				memset(out, ' ', VFDLENGTH);
				memcpy(out, g_str+len+i-VFDLENGTH, VFDLENGTH-i);
				vfd.vfd_write_string(out,true);
				usleep(250000);
			}
			else
			{
				scoll_loop = false;
				i = VFDLENGTH;
			}
		}
		memcpy(out, g_str, VFDLENGTH);
		vfd.vfd_write_string(out,true);
		if (VFD_SCROLL != 2 || !blocked)
			scoll_loop = false;
	}
	blocked = false;
	return NULL;
}

void evfd::vfd_write_string(char * str)
{
	int i = strlen(str);
	if (blocked)
	{
		pthread_cancel(thread_start_loop);
		pthread_join(thread_start_loop, NULL);
		blocked=false;
	}
	memset(g_str,0,64);
	strcpy(g_str,str);
	vfd_write_string(str, false);
	if (i > VFDLENGTH && VFD_SCROLL)
	{
		blocked = true;
		pthread_create(&thread_start_loop, NULL, vfd_write_string_scrollText1, (void *)str);
		pthread_detach(thread_start_loop);
	}
}

void evfd::vfd_write_string(char * str, bool force)
{
	int ws = 0;
	int i = strlen(str);
	if (VFD_CENTER)
	{
		if (i < VFDLENGTH)
			ws=(VFDLENGTH-i)/2;
		else
			ws=0;
	}
	if (i > VFDLENGTH) i = VFDLENGTH;
	struct vfd_ioctl_data data;
	memset(data.data, ' ', VFDLENGTH);
	if (VFD_CENTER)
		memcpy(data.data+ws, str, VFDLENGTH-ws);
	else
		memcpy(data.data, str, i);
	data.start = 0;
	if (VFD_CENTER)
		data.length = i+ws<=VFDLENGTH?i+ws:VFDLENGTH;
	else
		data.length = i;
		file_vfd = open (VFD_DEVICE, O_WRONLY);
		write(file_vfd,data.data,data.length);
		close (file_vfd);
	return;
}

#else

void evfd::vfd_write_string(char * str)
{
	vfd_write_string(str, false);
}

void evfd::vfd_write_string(char * str, bool force)
{
	int i;
	i = strlen ( str );
	if ( i > 63 ) i = 63;
	memset ( chars, ' ', 63 );
	memcpy ( chars, str, i);
	if (!blocked || force)
	{
		struct vfd_ioctl_data data;
		memset ( data.data, ' ', 63 );
		memcpy ( data.data, str, i );

		data.start = 0;
		data.length = i;

		file_vfd = open (VFD_DEVICE, O_WRONLY);
		ioctl ( file_vfd, VFDDISPLAYCHARS, &data );
		close (file_vfd);
	}
	return;
}

void evfd::vfd_write_string_scrollText(char* text)
{
	if (!blocked)
	{
		int i, len = strlen(text);
		char* out = (char *) malloc(16);
		for (i=0; i<=(len-16); i++)
		{ // scroll text till end
			memset(out, ' ', 16);
			memcpy(out, text+i, 16);
			vfd_write_string(out);
			usleep(200000);
		}
		for (i=1; i<16; i++)
		{ // scroll text with whitespaces from right
			memset(out, ' ', 16);
			memcpy(out, text+len+i-16, 16-i);
			vfd_write_string(out);
			usleep(200000);
		}
		memcpy(out, text, 16); // display first 16 chars after scrolling
		vfd_write_string(out);
		free (out);
	}
	return;
}
#endif
void evfd::vfd_clear_string()
{
	vfd_write_string("                ");
	return;
}

void evfd::vfd_set_icon(tvfd_icon id, bool onoff)
{
	vfd_set_icon(id, onoff, false);
	return;
}

void evfd::vfd_set_icon(tvfd_icon id, bool onoff, bool force)
{
	icon_onoff[id] = onoff;
	if (!blocked || force)
	{
#if defined(PLATFORM_SPARK) || defined(PLATFORM_SPARK7162)
	    	struct set_icon_s data;
#else
		struct vfd_ioctl_data data;
#endif
		if (!startloop_running)
		{
#if defined(PLATFORM_SPARK) || defined(PLATFORM_SPARK7162)
		    	memset(&data, 0, sizeof(struct set_icon_s));			
			data.icon_nr=id;
			data.on = onoff;
#else
			memset(&data, 0, sizeof(struct vfd_ioctl_data));
			data.start = 0x00;
			data.data[0] = id;
			data.data[4] = onoff;
			data.length = 5;
#endif
			file_vfd = open (VFD_DEVICE, O_WRONLY);
			ioctl(file_vfd, VFDICONDISPLAYONOFF, &data);
			close (file_vfd);
		}
	}
	return;
}

void evfd::vfd_clear_icons()
{
	for (int id = 1; id < 45; id++)
	{
		vfd_set_icon((tvfd_icon)id, false);
	}
	return;
}

void evfd::vfd_set_brightness(unsigned char setting)
{
	struct vfd_ioctl_data data;

	memset(&data, 0, sizeof(struct vfd_ioctl_data));

	data.start = setting & 0x07;
	data.length = 0;

	file_vfd = open (VFD_DEVICE, O_WRONLY);
	ioctl ( file_vfd, VFDBRIGHTNESS, &data );
	close (file_vfd);

	return;
}

void evfd::vfd_set_light(bool onoff)
{
	struct vfd_ioctl_data data;

	memset(&data, 0, sizeof(struct vfd_ioctl_data));

	if (onoff)
		data.start = 0x01;
	else
		data.start = 0x00;
		data.length = 0;

	file_vfd = open (VFD_DEVICE, O_WRONLY);
	ioctl(file_vfd, VFDDISPLAYWRITEONOFF, &data);

	close (file_vfd);
	return;
}

void evfd::vfd_set_fan(bool onoff)
{
	return;
}

void evfd::vfd_set_SCROLL(int id)
{
	VFD_SCROLL=id;
}

void evfd::vfd_set_CENTER(bool id)
{
	VFD_CENTER=id;
}
