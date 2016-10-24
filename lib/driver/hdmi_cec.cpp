#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#include <lib/base/init.h>
#include <lib/base/init_num.h>
#include <lib/base/eerror.h>
#include <lib/base/ebase.h>
#include <lib/base/nconfig.h>
#include <lib/driver/input_fake.h>
#include <lib/driver/hdmi_cec.h>
#include <lib/driver/avswitch.h>
/* NOTE: this header will move to linux uapi, once the cec framework is out of staging */
#include <lib/driver/linux-uapi-cec.h>

eHdmiCEC *eHdmiCEC::instance = NULL;

DEFINE_REF(eHdmiCEC::eCECMessage);

eHdmiCEC::eCECMessage::eCECMessage(int addr, int cmd, char *data, int length)
{
	address = addr;
	command = cmd;
	if (length > (int)sizeof(messageData)) length = sizeof(messageData);
	if (length && data) memcpy(messageData, data, length);
	dataLength = length;
}

int eHdmiCEC::eCECMessage::getAddress()
{
	return address;
}

int eHdmiCEC::eCECMessage::getCommand()
{
	return command;
}

int eHdmiCEC::eCECMessage::getData(char *data, int length)
{
	if (length > (int)dataLength) length = dataLength;
	memcpy(data, messageData, length);
	return length;
}

eHdmiCEC::eHdmiCEC()
: eRCDriver(eRCInput::getInstance())
{
	ASSERT(!instance);
	instance = this;
	linuxCEC = false;
	fixedAddress = false;
	physicalAddress[0] = 0x10;
	physicalAddress[1] = 0x00;
	logicalAddress = 1;
	deviceType = 1; /* default: recorder */

	hdmiFd = ::open("/dev/cec0", O_RDWR | O_CLOEXEC);
	if (hdmiFd >= 0)
	{
		__u32 monitor = CEC_MODE_INITIATOR | CEC_MODE_FOLLOWER;
		struct cec_caps caps = {};

		::ioctl(hdmiFd, CEC_ADAP_G_CAPS, &caps);

		if (caps.capabilities & CEC_CAP_LOG_ADDRS)
		{
			struct cec_log_addrs laddrs = {};

			::ioctl(hdmiFd, CEC_ADAP_S_LOG_ADDRS, &laddrs);
			memset(&laddrs, 0, sizeof(laddrs));

			/*
			 * NOTE: cec_version, osd_name and deviceType should be made configurable,
			 * CEC_ADAP_S_LOG_ADDRS delayed till the desired values are available
			 * (saves us some startup speed as well, polling for a free logical address
			 * takes some time)
			 */
			laddrs.cec_version = CEC_OP_CEC_VERSION_2_0;
			strcpy(laddrs.osd_name, "enigma2");
			laddrs.vendor_id = CEC_VENDOR_ID_NONE;

			switch (deviceType)
			{
			case CEC_LOG_ADDR_TV:
				laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_TV;
				laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_TV;
				laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_TV;
				break;
			case CEC_LOG_ADDR_RECORD_1:
				laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_RECORD;
				laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_RECORD;
				laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_RECORD;
				break;
			case CEC_LOG_ADDR_TUNER_1:
				laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_TUNER;
				laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_TUNER;
				laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_TUNER;
				break;
			case CEC_LOG_ADDR_PLAYBACK_1:
				laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_PLAYBACK;
				laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_PLAYBACK;
				laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_PLAYBACK;
				break;
			case CEC_LOG_ADDR_AUDIOSYSTEM:
				laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_AUDIOSYSTEM;
				laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_AUDIOSYSTEM;
				laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_AUDIOSYSTEM;
				break;
			default:
				laddrs.log_addr_type[laddrs.num_log_addrs] = CEC_LOG_ADDR_TYPE_UNREGISTERED;
				laddrs.all_device_types[laddrs.num_log_addrs] = CEC_OP_ALL_DEVTYPE_SWITCH;
				laddrs.primary_device_type[laddrs.num_log_addrs] = CEC_OP_PRIM_DEVTYPE_SWITCH;
				break;
			}
			laddrs.num_log_addrs++;

			::ioctl(hdmiFd, CEC_ADAP_S_LOG_ADDRS, &laddrs);
		}

		::ioctl(hdmiFd, CEC_S_MODE, &monitor);

		linuxCEC = true;
	}

	if (!linuxCEC)
	{
#ifdef DREAMBOX
#define HDMIDEV "/dev/misc/hdmi_cec0"
#else
#define HDMIDEV "/dev/hdmi_cec"
#endif

		hdmiFd = ::open(HDMIDEV, O_RDWR | O_NONBLOCK | O_CLOEXEC);
		if (hdmiFd >= 0)
		{

#ifdef DREAMBOX
			unsigned int val = 0;
			::ioctl(hdmiFd, 4, &val);
#else
			::ioctl(hdmiFd, 0); /* flush old messages */
#endif
		}
	}

	if (hdmiFd >= 0)
	{
		messageNotifier = eSocketNotifier::create(eApp, hdmiFd, eSocketNotifier::Read | eSocketNotifier::Priority);
		CONNECT(messageNotifier->activated, eHdmiCEC::hdmiEvent);
	}
	else
	{
		eDebug("[eHdmiCEC] cannot open %s: %m", HDMIDEV);
	}

	getAddressInfo();
}

eHdmiCEC::~eHdmiCEC()
{
	if (hdmiFd >= 0) ::close(hdmiFd);
}

eHdmiCEC *eHdmiCEC::getInstance()
{
	return instance;
}

void eHdmiCEC::reportPhysicalAddress()
{
	struct cec_message txmessage;
	txmessage.address = (logicalAddress << 4) + (0xf); /* broadcast */
	txmessage.data[0] = 0x84; /* report address */
	txmessage.data[1] = physicalAddress[0];
	txmessage.data[2] = physicalAddress[1];
	txmessage.data[3] = deviceType;
	txmessage.length = 4;
	sendMessage(txmessage);
}

void eHdmiCEC::getAddressInfo()
{
	if (hdmiFd >= 0)
	{
		bool hasdata = false;
#if DREAMBOX
		struct
		{
			unsigned char physical[2];
			unsigned char logical;
			unsigned char type;
		} addressinfo;

#else
		struct
		{
			unsigned char logical;
			unsigned char physical[2];
			unsigned char type;
		} addressinfo;
#endif 
		if (::ioctl(hdmiFd, 1, &addressinfo) >= 0)
		{
			hasdata = true;
			/* we do not get the device type, check the logical address to determine the type */
			switch (addressinfo.logical)
			{
			case 0x1:
			case 0x2:
			case 0x9:
				addressinfo.type = 1; /* recorder */
				break;
			case 0x3:
			case 0x6:
			case 0x7:
			case 0xa:
				addressinfo.type = 3; /* tuner */
				break;
			case 0x4:
			case 0x8:
			case 0xb:
				addressinfo.type = 4; /* playback */
				break;
			}
		}
		if (hasdata)
		{
			deviceType = addressinfo.type;
			logicalAddress = addressinfo.logical;
			if (!fixedAddress)
			{
				if (memcmp(physicalAddress, addressinfo.physical, sizeof(physicalAddress)))
				{
					eDebug("[eHdmiCEC] detected physical address change: %02X%02X --> %02X%02X", physicalAddress[0], physicalAddress[1], addressinfo.physical[0], addressinfo.physical[1]);
					memcpy(physicalAddress, addressinfo.physical, sizeof(physicalAddress));
					reportPhysicalAddress();
					/* emit */ addressChanged((physicalAddress[0] << 8) | physicalAddress[1]);
				}
			}
		}
	}
}

int eHdmiCEC::getLogicalAddress()
{
	return logicalAddress;
}

int eHdmiCEC::getPhysicalAddress()
{
	return (physicalAddress[0] << 8) | physicalAddress[1];
}

void eHdmiCEC::setFixedPhysicalAddress(int address)
{
	if (address)
	{
		fixedAddress = true;
		physicalAddress[0] = (address >> 8) & 0xff;
		physicalAddress[1] = address & 0xff;
		if (linuxCEC)
		{
			__u16 phys_addr = address & 0xffff;
			::ioctl(hdmiFd, CEC_ADAP_S_PHYS_ADDR, &phys_addr);
		}
		/* report our (possibly new) address */
		reportPhysicalAddress();
	}
	else
	{
		fixedAddress = false;
		/* get our current address */
		getAddressInfo();
	}
}

int eHdmiCEC::getDeviceType()
{
	return deviceType;
}

bool eHdmiCEC::getActiveStatus()
{
	bool active = true;
	eAVSwitch *avswitch = eAVSwitch::getInstance();
	if (avswitch) active = avswitch->isActive();
	return active;
}

void eHdmiCEC::hdmiEvent(int what)
{
	if (what & eSocketNotifier::Priority)
	{
		if (linuxCEC)
		{
			struct cec_event cecevent;
			::ioctl(hdmiFd, CEC_DQEVENT, &cecevent);
			if (cecevent.event == CEC_EVENT_STATE_CHANGE)
			{
				/* do not bother decoding the new address, just get the address in getAddressInfo */
			}
		}
		getAddressInfo();
	}

	if (what & eSocketNotifier::Read)
	{
		bool hasdata = false;
		struct cec_rx_message rxmessage;
		if (linuxCEC)
		{
			struct cec_msg msg;
			if (::ioctl(hdmiFd, CEC_RECEIVE, &msg) >= 0)
			{
				rxmessage.length = msg.len - 1;
				memcpy(&rxmessage.data, &msg.msg[1], rxmessage.length);
				hasdata = true;
			}
		}
		else
		{
#ifdef DREAMBOX
			if (::ioctl(hdmiFd, 2, &rxmessage) >= 0)
			{
				hasdata = true;
			}
			unsigned int val = 0;
			::ioctl(hdmiFd, 4, &val);
#else
			if (::read(hdmiFd, &rxmessage, 2) == 2)
			{
				if (::read(hdmiFd, &rxmessage.data, rxmessage.length) == rxmessage.length)
				{
					hasdata = true;
				}
			}
#endif
		}
		bool hdmicec_enabled = eConfigManager::getConfigBoolValue("config.hdmicec.enabled", false);
		if (hasdata && hdmicec_enabled)
		{
			bool keypressed = false;
			static unsigned char pressedkey = 0;

			eDebugNoNewLineStart("[eHdmiCEC] received message");
			for (int i = 0; i < rxmessage.length; i++)
			{
				eDebugNoNewLine(" %02X", rxmessage.data[i]);
			}
			eDebugNoNewLine("\n");
			bool hdmicec_report_active_menu = eConfigManager::getConfigBoolValue("config.hdmicec.report_active_menu", false);
			if (hdmicec_report_active_menu)
			{
				switch (rxmessage.data[0])
				{
					case 0x44: /* key pressed */
						keypressed = true;
						pressedkey = rxmessage.data[1];
					case 0x45: /* key released */
					{
						long code = translateKey(pressedkey);
						if (keypressed) code |= 0x80000000;
						for (std::list<eRCDevice*>::iterator i(listeners.begin()); i != listeners.end(); ++i)
						{
							(*i)->handleCode(code);
						}
						break;
					}
				}
			}
			ePtr<iCECMessage> msg = new eCECMessage(rxmessage.address, rxmessage.data[0], (char*)&rxmessage.data[1], rxmessage.length);
			messageReceived(msg);
		}
	}
}

long eHdmiCEC::translateKey(unsigned char code)
{
	long key = 0;
	switch (code)
	{
		case 0x32:
			key = 0x8b; //KEY_MENU
			break;
		case 0x20:
			key = 0x0b; //KEY_0
			break;
		case 0x21:
			key = 0x02; //KEY_1
			break;
		case 0x22:
			key = 0x03; //KEY_2
			break;
		case 0x23:
			key = 0x04; //KEY_3
			break;
		case 0x24:
			key = 0x05; //KEY_4
			break;
		case 0x25:
			key = 0x06; //KEY_5
			break;
		case 0x26:
			key = 0x07; //KEY_6
			break;
		case 0x27:
			key = 0x08; //KEY_7
			break;
		case 0x28:
			key = 0x09; //KEY_8
			break;
		case 0x29:
			key = 0x0a; //KEY_9
			break;
		case 0x30:
			key = 0x192; //KEY_CHANNELUP
			break;
		case 0x31:
			key = 0x193; //KEY_CHANNELDOWN
			break;
		case 0x40:
			key = 0x74; //KEY_POWER
			break;
		case 0x44:
		case 0x60:
			key = 0xcf; //KEY_PLAY
			break;
		case 0x45:
			key = 0x80; //KEY_STOP
			break;
		case 0x46:
			key = 0x77; //KEY_PAUSE
			break;
		case 0x47:
			key = 0xa7; //KEY_RECORD
			break;
		case 0x48:
			key = 0xa8; //KEY_REWIND
			break;
		case 0x49:
		case 0x4B:
			key = 0xd0; //KEY_FASTFORWARD
			break;
		case 0x4C:
			key = 0xa8; //KEY_REWIND
			break;
		case 0x53:
			key = 0x166; //KEY_INFO
			break;
		case 0x54:
			key = 0x16a; //KEY_PROGRAM
			break;
		case 0x61:
			key = 0xa4; //KEY_PLAYPAUSE
			break;
		case 0x62:
			key = 0xa7; //KEY_RECORD
			break;
		case 0x64:
			key = 0x80; //KEY_STOP
			break;
		case 0x00:
			key = 0x160; //KEY_OK
			break;
		case 0x03:
			key = 0x69; //KEY_LEFT
			break;
		case 0x04:
			key = 0x6a; //KEY_RIGHT
			break;
		case 0x01:
			key = 0x67; //KEY_UP
			break;
		case 0x02:
			key = 0x6c; //KEY_DOWN
			break;
		case 0x0d:
			key = 0xae; //KEY_EXIT
			break;
		case 0x72:
			key = 0x18e; //KEY_RED
			break;
		case 0x71:
			key = 0x191; //KEY_BLUE
			break;
		case 0x73:
			key = 0x18f; //KEY_GREEN
			break;
		case 0x74:
			key = 0x190; //KEY_YELLOW
			break;
		default:
			eDebug("eHdmiCEC: unknown code 0x%02X", (unsigned int)(code & 0xFF));
			break;
	}
	return key;
}

void eHdmiCEC::sendMessage(struct cec_message &message)
{
	if (hdmiFd >= 0)
	{
		eDebugNoNewLineStart("[eHdmiCEC] send message");
		for (int i = 0; i < message.length; i++)
		{
			eDebugNoNewLine(" %02X", message.data[i]);
		}
		eDebugNoNewLine(" -> %02X\n", message.address);
		if (linuxCEC)
		{
			struct cec_msg msg;
			cec_msg_init(&msg, logicalAddress, message.address);
			memcpy(&msg.msg[1], message.data, message.length);
			msg.len = message.length + 1;
			::ioctl(hdmiFd, CEC_TRANSMIT, &msg);
		}
		else
		{
#ifdef DREAMBOX
			message.flag = 1;
			::ioctl(hdmiFd, 3, &message);
#else
			::write(hdmiFd, &message, 2 + message.length);
#endif
		}
	}
}

void eHdmiCEC::sendMessage(unsigned char address, unsigned char cmd, char *data, int length)
{
	struct cec_message message;
	message.address = address;
	if (length > (int)(sizeof(message.data) - 1)) length = sizeof(message.data) - 1;
	message.length = length + 1;
	message.data[0] = cmd;
	memcpy(&message.data[1], data, length);
	sendMessage(message);
}

void eHdmiCECDevice::handleCode(long code)
{
	if (code & 0x80000000)
	{
		/*emit*/ input->keyPressed(eRCKey(this, code & 0xffff, 0));
	}
	else
	{
		/*emit*/ input->keyPressed(eRCKey(this, code & 0xffff, eRCKey::flagBreak));
	}
}

eHdmiCECDevice::eHdmiCECDevice(eRCDriver *driver)
 : eRCDevice("Hdmi-CEC", driver)
{
}

const char *eHdmiCECDevice::getDescription() const
{
	return "Hdmi-CEC device";
}

class eHdmiCECInit
{
	eHdmiCEC driver;
	eHdmiCECDevice device;

public:
	eHdmiCECInit(): driver(), device(&driver)
	{
	}
};

eAutoInitP0<eHdmiCECInit> init_hdmicec(eAutoInitNumbers::rc + 2, "Hdmi CEC driver");
