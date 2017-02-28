#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <pthread.h>

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

char g_str[64];

struct vfd_ioctl_data
{
	unsigned char start;
	unsigned char data[64];
	unsigned char length;
};

struct set_icon_s {
	int icon_nr;
	int on;
};

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
	vfd_type=8;
	FILE *vfd_proc = fopen ("/proc/aotom/display_type", "r");
	if (vfd_proc)
	{	char buf[2];
		fread(&buf,sizeof(buf),1,vfd_proc);
		vfd_type=atoi(&buf[0]);
		fclose (vfd_proc);
	}
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
	vfd.vfd_write_string("-00-", true);
	//run 2 times through all icons 
	if (vfd.getVfdType() != 4)
	{
		memset(&icon_onoff,0, sizeof(icon_onoff));
		static const unsigned char brightness[14]={1,2,3,4,5,6,7,6,5,4,3,2,1,0};

		for (int vloop = 0; vloop < 128; vloop++)
		{
			vfd.vfd_set_brightness(brightness[vloop%14]);
			usleep(75000);
		}

		vfd.vfd_set_brightness(7);
	}
	blocked = false;
	return NULL;
}

void evfd::vfd_write_string(const char * str)
{
	vfd_write_string(str, false);
}

void evfd::vfd_write_string(const char * str, bool force)
{
	if (!blocked || force)
	{
		struct vfd_ioctl_data data;
		data.length = (unsigned char)snprintf((char*)data.data, sizeof(data.data), "%s", str);
		data.start = 0;

		file_vfd = open (VFD_DEVICE, O_WRONLY);
		ioctl ( file_vfd, VFDDISPLAYCHARS, &data );
		close (file_vfd);
	}
}

void evfd::vfd_write_string_scrollText(const char* text)
{
	if (!blocked)
	{
		int i, len = strlen(text);
		char out[17]={'\0'};
		for (i=0; i<=(len-16); i++)
		{ // scroll text till end
			memcpy(out, text+i, 16);
			vfd_write_string(out);
			usleep(200000);
		}
		for (i=1; i<16; i++)
		{ // scroll text with whitespaces from right
			memcpy(out, text+len+i-16, 16-i);
			memset(out+(16-i-1), ' ', i);
			vfd_write_string(out);
			usleep(200000);
		}
		memcpy(out, text, 16); // display first 16 chars after scrolling
		vfd_write_string(out);
	}
	return;
}

void evfd::vfd_clear_string()
{
	vfd_write_string("                ");
}

void evfd::vfd_set_icon(tvfd_icon id, bool onoff)
{
	if (getVfdType() != 4) vfd_set_icon(id, onoff, false);
}

void evfd::vfd_set_icon(tvfd_icon id, bool onoff, bool force)
{
	if (getVfdType() != 4)
	{
		icon_onoff[id] = onoff;
		if (!blocked || force)
		{
			struct set_icon_s data;

			if (!startloop_running)
			{
				memset(&data, 0, sizeof(struct set_icon_s));
				data.icon_nr=id;
				data.on = onoff;
				file_vfd = open (VFD_DEVICE, O_WRONLY);
				ioctl(file_vfd, VFDICONDISPLAYONOFF, &data);
				close (file_vfd);
			}
		}
	}
	return;
}

void evfd::vfd_clear_icons()
{
	if (getVfdType() != 4)
	{
		for (int id = 1; id <= 45; id++)
		{
			vfd_set_icon((tvfd_icon)id, false);
		}
	}
	return;
}

void evfd::vfd_set_brightness(unsigned char setting)
{
	if (getVfdType() != 4)
	{
	struct vfd_ioctl_data data;

		memset(&data, 0, sizeof(struct vfd_ioctl_data));

		data.start = setting & 0x07;
		data.length = 0;

		file_vfd = open (VFD_DEVICE, O_WRONLY);
		ioctl ( file_vfd, VFDBRIGHTNESS, &data );
		close (file_vfd);
	}
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
