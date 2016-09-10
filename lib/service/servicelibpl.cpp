#include <lib/base/ebase.h>
#include <lib/base/eerror.h>
#include <lib/base/init_num.h>
#include <lib/base/init.h>
#include <lib/base/nconfig.h>
#include <lib/base/object.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/decoder.h>
#include <lib/components/file_eraser.h>
#include <lib/gui/esubtitle.h>
#include <lib/service/servicelibpl.h>
#include <lib/service/service.h>
#include <lib/gdi/gpixmap.h>
#include <string>

#ifdef ENABLE_GSTREAMER
#include <lib/base/eenv.h>
#endif

void ep3Blit()
{
	fbClass *fb = fbClass::getInstance();
	fb->blit();
}

eServiceFactoryLibpl::eServiceFactoryLibpl()
{
	ePtr<eServiceCenter> sc;

#ifdef ENABLE_GSTREAMER
	defaultMP3_Player = (::access(eEnv::resolve("${sysconfdir}/enigma2/mp3player").c_str(), F_OK) >= 0);
#endif

	eServiceCenter::getPrivInstance(sc);
	if (sc)
	{
		std::list<std::string> extensions;
		extensions.push_back("dts");
		extensions.push_back("mp2");
		extensions.push_back("mp3");
		extensions.push_back("ogg");
		extensions.push_back("ogm");
		extensions.push_back("ogv");
		extensions.push_back("mpg");
		extensions.push_back("vob");
		extensions.push_back("wav");
		extensions.push_back("wave");
		extensions.push_back("m4v");
		extensions.push_back("mkv");
		extensions.push_back("avi");
		extensions.push_back("divx");
		extensions.push_back("dat");
		extensions.push_back("flac");
		extensions.push_back("flv");
		extensions.push_back("mp4");
		extensions.push_back("mov");
		extensions.push_back("m4a");
		extensions.push_back("3gp");
		extensions.push_back("3g2");
		extensions.push_back("asf");
		extensions.push_back("mpeg");
		extensions.push_back("m2ts");
		extensions.push_back("trp");
		extensions.push_back("vdr");
		extensions.push_back("mts");
		extensions.push_back("rar");
		extensions.push_back("img");
		extensions.push_back("iso");
		extensions.push_back("ifo");
		extensions.push_back("wmv");
		extensions.push_back("wma");
#ifdef ENABLE_GSTREAMER
		if (!defaultMP3_Player)
		{
			sc->addServiceFactory(eServiceFactoryLibpl::id, this, extensions);
		}
		extensions.clear();
		sc->addServiceFactory(eServiceFactoryLibpl::idServiceLibpl, this, extensions);
#else
		sc->addServiceFactory(eServiceFactoryLibpl::id, this, extensions);
#endif
	}

	m_service_info = new eStaticServiceLibplInfo();
}

eServiceFactoryLibpl::~eServiceFactoryLibpl()
{
	ePtr<eServiceCenter> sc;
	eServiceCenter::getPrivInstance(sc);

	if (sc)
#ifdef ENABLE_GSTREAMER
		sc->removeServiceFactory(eServiceFactoryLibpl::idServiceLibpl);
		if (!defaultMP3_Player)
		{
			sc->removeServiceFactory(eServiceFactoryLibpl::id);
		}
#else
		sc->removeServiceFactory(eServiceFactoryLibpl::id);
#endif
}

DEFINE_REF(eServiceFactoryLibpl)

// iServiceHandler
RESULT eServiceFactoryLibpl::play(const eServiceReference &ref, ePtr<iPlayableService> &ptr)
{
	// check resources...
	ptr = new eServiceLibpl(ref);
	return 0;
}

RESULT eServiceFactoryLibpl::record(const eServiceReference &ref, ePtr<iRecordableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryLibpl::list(const eServiceReference &, ePtr<iListableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryLibpl::info(const eServiceReference &ref, ePtr<iStaticServiceInformation> &ptr)
{
	ptr = m_service_info;
	return 0;
}

class eLibplServiceOfflineOperations: public iServiceOfflineOperations
{
	DECLARE_REF(eLibplServiceOfflineOperations);
	eServiceReference m_ref;
public:
	eLibplServiceOfflineOperations(const eServiceReference &ref);
	RESULT deleteFromDisk(int simulate);
	RESULT getListOfFilenames(std::list<std::string> &);
	RESULT reindex();
};

DEFINE_REF(eLibplServiceOfflineOperations);

eLibplServiceOfflineOperations::eLibplServiceOfflineOperations(const eServiceReference &ref): m_ref((const eServiceReference&)ref)
{
}

RESULT eLibplServiceOfflineOperations::deleteFromDisk(int simulate)
{
	if (!simulate)
	{
		std::list<std::string> res;
		if (getListOfFilenames(res))
			return -1;
		eBackgroundFileEraser *eraser = eBackgroundFileEraser::getInstance();
		if (!eraser)
			eDebug("[eServiceLibpl::%s] FATAL !! can't get background file eraser", __func__);
		for (std::list<std::string>::iterator i(res.begin()); i != res.end(); ++i)
		{
			eDebug("[eServiceLibpl::%s] Removing %s...", __func__, i->c_str());
			if (eraser)
				eraser->erase(i->c_str());
			else
				::unlink(i->c_str());
		}
	}

	return 0;
}

RESULT eLibplServiceOfflineOperations::getListOfFilenames(std::list<std::string> &res)
{
	res.clear();
	res.push_back(m_ref.path);
	return 0;
}

RESULT eLibplServiceOfflineOperations::reindex()
{
	return -1;
}


RESULT eServiceFactoryLibpl::offlineOperations(const eServiceReference &ref, ePtr<iServiceOfflineOperations> &ptr)
{
	ptr = new eLibplServiceOfflineOperations(ref);
	return 0;
}

// eStaticServiceLibplInfo


// eStaticServiceLibplInfo is seperated from eServiceLibpl to give information
// about unopened files.

// probably eServiceLibpl should use this class as well, and eStaticServiceLibplInfo
// should have a database backend where ID3-files etc. are cached.
// this would allow listing the mp3 database based on certain filters.

DEFINE_REF(eStaticServiceLibplInfo)

eStaticServiceLibplInfo::eStaticServiceLibplInfo()
{
}

RESULT eStaticServiceLibplInfo::getName(const eServiceReference &ref, std::string &name)
{
	if ( ref.name.length() )
		name = ref.name;
	else
	{
		size_t last = ref.path.rfind('/');
		if (last != std::string::npos)
			name = ref.path.substr(last+1);
		else
			name = ref.path;
	}

	return 0;
}

int eStaticServiceLibplInfo::getLength(const eServiceReference &ref)
{
	return -1;
}

int eStaticServiceLibplInfo::getInfo(const eServiceReference &ref, int w)
{
	switch (w)
	{
	case iServiceInformation::sTimeCreate:
		{
			struct stat s;
			if (stat(ref.path.c_str(), &s) == 0)
			{
				return s.st_mtime;
			}
		}
		break;
	case iServiceInformation::sFileSize:
		{
			struct stat s;
			if (stat(ref.path.c_str(), &s) == 0)
			{
				return s.st_size;
			}
		}
		break;
	}

	return iServiceInformation::resNA;
}

long long eStaticServiceLibplInfo::getFileSize(const eServiceReference &ref)
{
	struct stat s;
	if (stat(ref.path.c_str(), &s) == 0)
	{
		return s.st_size;
	}

	return 0;
}

RESULT eStaticServiceLibplInfo::getEvent(const eServiceReference &ref, ePtr<eServiceEvent> &evt, time_t start_time)
{
	if (ref.path.find("://") != std::string::npos)
	{
		eServiceReference equivalentref(ref);
		equivalentref.type = eServiceFactoryLibpl::id;
		equivalentref.path.clear();
		return eEPGCache::getInstance()->lookupEventTime(equivalentref, start_time, evt);
	}

	evt = 0;
	return -1;
}

DEFINE_REF(eStreamLibplBufferInfo)

eStreamLibplBufferInfo::eStreamLibplBufferInfo(int percentage, int inputrate, int outputrate, int space, int size)
: bufferPercentage(percentage),
	inputRate(inputrate),
	outputRate(outputrate),
	bufferSpace(space),
	bufferSize(size)
{
}

int eStreamLibplBufferInfo::getBufferPercentage() const
{
	return bufferPercentage;
}

int eStreamLibplBufferInfo::getAverageInputRate() const
{
	return inputRate;
}

int eStreamLibplBufferInfo::getAverageOutputRate() const
{
	return outputRate;
}

int eStreamLibplBufferInfo::getBufferSpace() const
{
	return bufferSpace;
}

int eStreamLibplBufferInfo::getBufferSize() const
{
	return bufferSize;
}

eServiceLibpl *eServiceLibpl::instance;

eServiceLibpl *eServiceLibpl::getInstance()
{
	return instance;
}

eServiceLibpl::eServiceLibpl(eServiceReference ref):
	m_nownext_timer(eTimer::create(eApp)),
	m_cuesheet_changed(0),
	m_cutlist_enabled(1),
	m_ref(ref),
	m_pump(eApp, 1)
{
	eDebug("[eServiceLibpl::%s]", __func__);
	m_currentAudioStream = -1;
	m_currentSubtitleStream = -1;
	m_cachedSubtitleStream = -1; /* report the first subtitle stream to be 'cached'. TODO: use an actual cache. */
	m_subtitle_widget = 0;
	m_buffer_size = 5 * 1024 * 1024;
	m_cuesheet_loaded = false; /* cuesheet CVR */
	inst_m_pump = &m_pump;
	CONNECT(m_nownext_timer->timeout, eServiceLibpl::updateEpgCacheNowNext);
	CONNECT(inst_m_pump->recv_msg, eServiceLibpl::gotThreadMessage);
	m_aspect = m_width = m_height = m_framerate = m_progressive = -1;
	m_state = stIdle;
	instance = this;

	player = (Context_t*) malloc(sizeof(Context_t));

	if (player)
	{
		player->playback  = &PlaybackHandler;
		player->output    = &OutputHandler;
		player->container = &ContainerHandler;
		player->manager   = &ManagerHandler;
		eDebug("[eServiceLibpl::%s] %s", __func__, player->output->Name);
	}

	//Registration of output devices
	if (player && player->output)
	{
		player->output->Command(player,OUTPUT_ADD, (void*)"audio");
		player->output->Command(player,OUTPUT_ADD, (void*)"video");
		player->output->Command(player,OUTPUT_ADD, (void*)"subtitle");
	}

	if (player && player->output && player->output->subtitle)
	{
		fbClass *fb = fbClass::getInstance();
		SubtitleOutputDef_t out;
		out.screen_width = fb->getScreenResX();
		out.screen_height = fb->getScreenResY();
		out.shareFramebuffer = 1;
		out.framebufferFD = fb->getFD();
		out.destination = fb->getLFB_Direct();
		out.destStride = fb->Stride();
		out.framebufferBlit = ep3Blit;
		player->output->subtitle->Command(player, (OutputCmd_t)OUTPUT_SET_SUBTITLE_OUTPUT, (void*) &out);
	}

	//create playback path
	char file[1023] = {""};
	if ((!strncmp("http://", m_ref.path.c_str(), 7))
	|| (!strncmp("https://", m_ref.path.c_str(), 8))
	|| (!strncmp("cache://", m_ref.path.c_str(), 8))
	|| (!strncmp("concat://", m_ref.path.c_str(), 9))
	|| (!strncmp("crypto://", m_ref.path.c_str(), 9))
	|| (!strncmp("gopher://", m_ref.path.c_str(), 9))
	|| (!strncmp("hls://", m_ref.path.c_str(), 6))
	|| (!strncmp("hls+http://", m_ref.path.c_str(), 11))
	|| (!strncmp("httpproxy://", m_ref.path.c_str(), 12))
	|| (!strncmp("mmsh://", m_ref.path.c_str(), 7))
	|| (!strncmp("mmst://", m_ref.path.c_str(), 7))
	|| (!strncmp("rtmp://", m_ref.path.c_str(), 7))
	|| (!strncmp("rtmpe://", m_ref.path.c_str(), 8))
	|| (!strncmp("rtmpt://", m_ref.path.c_str(), 8))
	|| (!strncmp("rtmps://", m_ref.path.c_str(), 8))
	|| (!strncmp("rtmpte://", m_ref.path.c_str(), 9))
	|| (!strncmp("ftp://", m_ref.path.c_str(), 6))
	|| (!strncmp("rtp://", m_ref.path.c_str(), 6))
	|| (!strncmp("srtp://", m_ref.path.c_str(), 7))
	|| (!strncmp("subfile://", m_ref.path.c_str(), 10))
	|| (!strncmp("tcp://", m_ref.path.c_str(), 6))
	|| (!strncmp("tls://", m_ref.path.c_str(), 6))
	|| (!strncmp("udp://", m_ref.path.c_str(), 6))
	|| (!strncmp("udplite://", m_ref.path.c_str(), 10)))
		m_sourceinfo.is_streaming = true;
	else if ((!strncmp("file://", m_ref.path.c_str(), 7))
	|| (!strncmp("bluray://", m_ref.path.c_str(), 9))
	|| (!strncmp("hls+file://", m_ref.path.c_str(), 11))
	|| (!strncmp("myts://", m_ref.path.c_str(), 7)))
		;
	else
		strcat(file, "file://");

	// try parse HLS master playlist to use streams from it
	size_t delim_idx = m_ref.path.rfind(".");
	if(!strncmp("http", m_ref.path.c_str(), 4) && delim_idx != std::string::npos && !m_ref.path.compare(delim_idx, 5, ".m3u8"))
	{
		M3U8VariantsExplorer ve(m_ref.path);
		std::vector<M3U8StreamInfo> m_stream_vec = ve.getStreams();
		if (m_stream_vec.empty())
		{
			eDebug("[eServiceLibpl::%s] failed to retrieve m3u8 streams", __func__);
			strcat(file, m_ref.path.c_str());
		}
		else
		{
			// sort streams from best quality to worst (internally sorted according to bitrate)
			sort(m_stream_vec.rbegin(), m_stream_vec.rend());
			unsigned int bitrate = eConfigManager::getConfigIntValue("config.streaming.connectionSpeedInKb") * 1000L;
			std::vector<M3U8StreamInfo>::const_iterator it(m_stream_vec.begin());
			while(!(it == m_stream_vec.end() || it->bitrate <= bitrate))
			{
				it++;
			}
			eDebug("[eServiceLibpl::%s] play stream (%lu b/s) selected according to connection speed (%d b/s)",
				__func__, it->bitrate, bitrate);
			strcat(file, it->url.c_str());
		}
	}
	else
		strcat(file, m_ref.path.c_str());

	//try to open file
	if (player && player->playback && player->playback->Command(player, PLAYBACK_OPEN, file) >= 0)
	{
		//VIDEO
		//We dont have to register video tracks, or do we ?
		//AUDIO
		if (player && player->manager && player->manager->audio)
		{
			char ** TrackList = NULL;
			player->manager->audio->Command(player, MANAGER_LIST, &TrackList);
			if (TrackList != NULL)
			{
				eDebug("[eServiceLibpl::%s] AudioTrack List:", __func__);
				int i = 0;
				for (i = 0; TrackList[i] != NULL; i+=2)
				{
					eDebug("[eServiceLibpl::%s]\t%s - %s", __func__, TrackList[i], TrackList[i+1]);
					audioStream audio;
					audio.language_code = TrackList[i];

					// atUnknown, atMPEG, atMP3, atAC3, atDTS, atAAC, atPCM, atOGG, atFLAC
					if (    !strncmp("A_MPEG/L3",   TrackList[i+1], 9))
						audio.type = atMP3;
					else if (!strncmp("A_MP3",      TrackList[i+1], 5))
						audio.type = atMP3;
					else if (!strncmp("A_AC3",      TrackList[i+1], 5))
						audio.type = atAC3;
					else if (!strncmp("A_DTS",      TrackList[i+1], 5))
						audio.type = atDTS;
					else if (!strncmp("A_AAC",      TrackList[i+1], 5))
						audio.type = atAAC;
					else if (!strncmp("A_PCM",      TrackList[i+1], 5))
						audio.type = atPCM;
					else if (!strncmp("A_VORBIS",   TrackList[i+1], 8))
						audio.type = atOGG;
					else if (!strncmp("A_FLAC",     TrackList[i+1], 6))
						audio.type = atFLAC;
					else
						audio.type = atUnknown;

					m_audioStreams.push_back(audio);
					free(TrackList[i]);
					TrackList[i] = NULL;
					free(TrackList[i+1]);
					TrackList[i+1] = NULL;
				}
				free(TrackList);
				TrackList = NULL;
			}
		}

		//SUB
		if (player && player->manager && player->manager->subtitle)
		{
			char ** TrackList = NULL;
			player->manager->subtitle->Command(player, MANAGER_LIST, &TrackList);
			if (TrackList != NULL)
			{
				eDebug("[eServiceLibpl::%s] SubtitleTrack List:", __func__);
				int i = 0;
				for (i = 0; TrackList[i] != NULL; i+=2)
				{
					eDebug("[eServiceLibpl::%s]\t%s - %s", __func__, TrackList[i], TrackList[i+1]);
					subtitleStream sub;
					sub.language_code = TrackList[i];

					//  stPlainText, stSSA, stSRT
					if (     !strncmp("S_TEXT/SSA",   TrackList[i+1], 10) ||
							!strncmp("S_SSA", TrackList[i+1], 5))
						sub.type = stSSA;
					else if (!strncmp("S_TEXT/ASS",   TrackList[i+1], 10) ||
							!strncmp("S_AAS", TrackList[i+1], 5))
						sub.type = stSSA;
					else if (!strncmp("S_TEXT/SRT",   TrackList[i+1], 10) ||
							!strncmp("S_SRT", TrackList[i+1], 5))
						sub.type = stSRT;
					else
						sub.type = stPlainText;

					m_subtitleStreams.push_back(sub);
					free(TrackList[i]);
					TrackList[i] = NULL;
					free(TrackList[i+1]);
					TrackList[i+1] = NULL;
				}
				free(TrackList);
				TrackList = NULL;
			}
		}
		loadCuesheet(); /* cuesheet CVR */
		m_event(this, evStart);
	}
	else
	{
		//Creation failed, no playback support for insert file, so send e2 EOF to stop playback
		eDebug("[eServiceLibpl::%s] ERROR! Creation failed! No playback support for insert file!", __func__);
		m_state = stRunning;
		m_event(this, evEOF);
	}
}

eServiceLibpl::~eServiceLibpl()
{
	if (m_subtitle_widget) m_subtitle_widget->destroy();
	m_subtitle_widget = 0;

	if (m_state == stRunning)
		stop();
}

void eServiceLibpl::updateEpgCacheNowNext()
{
	bool update = false;
	ePtr<eServiceEvent> next = 0;
	ePtr<eServiceEvent> ptr = 0;
	eServiceReference ref(m_ref);
	ref.type = eServiceFactoryLibpl::id;
	ref.path.clear();

	if (eEPGCache::getInstance() && eEPGCache::getInstance()->lookupEventTime(ref, -1, ptr) >= 0)
	{
		ePtr<eServiceEvent> current = m_event_now;
		if (!current || !ptr || current->getEventId() != ptr->getEventId())
		{
			update = true;
			m_event_now = ptr;
			time_t next_time = ptr->getBeginTime() + ptr->getDuration();
			if (eEPGCache::getInstance()->lookupEventTime(ref, next_time, ptr) >= 0)
			{
				next = ptr;
				m_event_next = ptr;
			}
		}
	}

	int refreshtime = 60;

	if (!next)
	{
		next = m_event_next;
	}

	if (next)
	{
		time_t now = eDVBLocalTimeHandler::getInstance()->nowTime();
		refreshtime = (int)(next->getBeginTime() - now) + 3;

		if (refreshtime <= 0 || refreshtime > 60)
		{
			refreshtime = 60;
		}
	}

	m_nownext_timer->startLongTimer(refreshtime);

	if (update)
	{
		m_event((iPlayableService*)this, evUpdatedEventInfo);
	}
}

DEFINE_REF(eServiceLibpl);

RESULT eServiceLibpl::connectEvent(const Slot2<void,iPlayableService*,int> &event, ePtr<eConnection> &connection)
{
	connection = new eConnection((iPlayableService*)this, m_event.connect(event));
	m_event(this, evSeekableStatusChanged);
	return 0;
}

RESULT eServiceLibpl::start()
{
	if (m_state != stIdle)
	{
		eDebug("[eServiceLibpl::%s] m_state != stIdle", __func__);
		return -1;
	}

	if (player && player->output && player->playback)
	{
		m_state = stRunning;

		player->output->Command(player, OUTPUT_OPEN, NULL);
		player->playback->Command(player, PLAYBACK_PLAY, NULL);
		m_event(this, evStart);
		m_event(this, evGstreamerPlayStarted);
		updateEpgCacheNowNext();
		eDebug("[eServiceLibpl::%s] start %s", __func__, m_ref.path.c_str());

		return 0;
	}

	eDebug("[eServiceLibpl::%s] ERROR in start %s", __func__, m_ref.path.c_str());
	return -1;
}

RESULT eServiceLibpl::stop()
{
	if (m_state == stIdle)
	{
		eDebug("[eServiceLibpl::%s] m_state == stIdle", __func__);
		return -1;
	}

	if (m_state == stStopped)
		return -1;

	eDebug("[eServiceLibpl::%s] stop %s", __func__, m_ref.path.c_str());

	if (player && player->playback && player->output)
	{
		player->playback->Command(player, PLAYBACK_STOP, NULL);
		player->output->Command(player, OUTPUT_CLOSE, NULL);
	}

	if (player && player->output)
	{
		player->output->Command(player,OUTPUT_DEL, (void*)"audio");
		player->output->Command(player,OUTPUT_DEL, (void*)"video");
		player->output->Command(player,OUTPUT_DEL, (void*)"subtitle");
	}

	if (player && player->playback)
		player->playback->Command(player,PLAYBACK_CLOSE, NULL);

	if (player)
		free(player);

	if (player != NULL)
		player = NULL;

	m_state = stStopped;
	saveCuesheet();
	m_nownext_timer->stop();
	return 0;
}

RESULT eServiceLibpl::setTarget(int target)
{
	return -1;
}

RESULT eServiceLibpl::pause(ePtr<iPauseableService> &ptr)
{
	ptr=this;
	m_event((iPlayableService*)this, evUpdatedInfo);
	return 0;
}

int speed_mapping[] =
{
 /* e2_ratio   speed */
	2,         1,
	4,         3,
	8,         7,
	16,        15,
	32,        31,
	64,        63,
	128,      127,
	-2,       -5,
	-4,      -10,
	-8,      -20,
	-16,      -40,
	-32,      -80,
	-64,     -160,
	-128,     -320,
	-1,       -1
};

int getSpeed(int ratio)
{
	int i = 0;
	while (speed_mapping[i] != -1)
	{
		if (speed_mapping[i] == ratio)
			return speed_mapping[i+1];
		i += 2;
	}

	return -1;
}

RESULT eServiceLibpl::setSlowMotion(int ratio)
{
	// konfetti: in libeplayer3 we changed this because I dont like application specific stuff in a library
	int speed = getSpeed(ratio);

	if (player && player->playback && (speed != -1))
	{
		int result = 0;

		if (ratio > 1)
			result = player->playback->Command(player, PLAYBACK_SLOWMOTION, (void*)&speed);

		if (result != 0)
			return -1;
	}

	return 0;
}

RESULT eServiceLibpl::setFastForward(int ratio)
{
	// konfetti: in libeplayer3 we changed this because I dont like application specific stuff in a library
	int speed = getSpeed(ratio);

	if (player && player->playback && (speed != -1))
	{
		int result = 0;

		if (ratio > 1)
			result = player->playback->Command(player, PLAYBACK_FASTFORWARD, (void*)&speed);
		else if (ratio < -1)
		{
			//speed = speed * -1;
			result = player->playback->Command(player, PLAYBACK_FASTBACKWARD, (void*)&speed);
		}
		else
			result = player->playback->Command(player, PLAYBACK_CONTINUE, NULL);

		if (result != 0)
			return -1;
	}

	return 0;
}

		// iPausableService
RESULT eServiceLibpl::pause()
{
	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_PAUSE, NULL);

	return 0;
}

RESULT eServiceLibpl::unpause()
{
	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_CONTINUE, NULL);

	return 0;
}

	/* iSeekableService */
RESULT eServiceLibpl::seek(ePtr<iSeekableService> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceLibpl::getLength(pts_t &pts)
{
	double length = 0;

	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_LENGTH, &length);

	if (length <= 0)
		return -1;

	pts = length * 90000;
	return 0;
}

RESULT eServiceLibpl::seekTo(pts_t to)
{
	float pos = (to/90000.0)-10;

	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_SEEK, (void*)&pos);

	return 0;
}

RESULT eServiceLibpl::seekRelative(int direction, pts_t to)
{
	float pos = direction*(to/90000.0);

	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_SEEK, (void*)&pos);

	return 0;
}

RESULT eServiceLibpl::getPlayPosition(pts_t &pts)
{
	if (player && player->playback && !player->playback->isPlaying)
	{
		eDebug("[eServiceLibpl::%s] !!!!EOF!!!!", __func__);

		if(m_state == stRunning)
			m_event((iPlayableService*)this, evEOF);

		pts = 0;
		return -1;
	}

	unsigned long long int vpts = 0;

	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_PTS, &vpts);

	if (vpts<=0)
		return -1;

	/* len is in nanoseconds. we have 90 000 pts per second. */
	pts = vpts;
	return 0;
}

RESULT eServiceLibpl::setTrickmode(int trick)
{
	/* trickmode is not yet supported by our dvbmediasinks. */
	return -1;
}

RESULT eServiceLibpl::isCurrentlySeekable()
{
	// Hellmaster1024: 1 for skipping 3 for skipping anf fast forward
	return 3;
}

RESULT eServiceLibpl::info(ePtr<iServiceInformation>&i)
{
	i = this;
	return 0;
}

RESULT eServiceLibpl::getName(std::string &name)
{
	std::string title = m_ref.getName();

	if (title.empty())
	{
		name = m_ref.path;
		size_t n = name.rfind('/');
		if (n != std::string::npos)
			name = name.substr(n + 1);
	}
	else
		name = title;

	return 0;
}

RESULT eServiceLibpl::getEvent(ePtr<eServiceEvent> &evt, int nownext)
{
	evt = nownext ? m_event_next : m_event_now;
	if (!evt)
		return -1;

	return 0;
}

int eServiceLibpl::getInfo(int w)
{
	switch (w)
	{
	case sServiceref: return m_ref;
	case sVideoHeight: return m_height;
	case sVideoWidth: return m_width;
	case sFrameRate: return m_framerate;
	case sProgressive: return m_progressive;
	case sAspect: return m_aspect;
	case sTagTitle:
	case sTagArtist:
	case sTagAlbum:
	case sTagTitleSortname:
	case sTagArtistSortname:
	case sTagAlbumSortname:
	case sTagDate:
	case sTagComposer:
	case sTagGenre:
	case sTagComment:
	case sTagExtendedComment:
	case sTagLocation:
	case sTagHomepage:
	case sTagDescription:
	case sTagVersion:
	case sTagISRC:
	case sTagOrganization:
	case sTagCopyright:
	case sTagCopyrightURI:
	case sTagContact:
	case sTagLicense:
	case sTagLicenseURI:
	case sTagCodec:
	case sTagAudioCodec:
	case sTagVideoCodec:
	case sTagEncoder:
	case sTagLanguageCode:
	case sTagKeywords:
	case sTagChannelMode:
	case sUser+12:
		return resIsString;
	case sTagTrackGain:
	case sTagTrackPeak:
	case sTagAlbumGain:
	case sTagAlbumPeak:
	case sTagReferenceLevel:
	case sTagBeatsPerMinute:
	case sTagImage:
	case sTagPreviewImage:
	case sTagAttachment:
		return resIsPyObject;
	case sBuffer: return m_bufferInfo.bufferPercent;
	default:
		return resNA;
	}

	return 0;
}

std::string eServiceLibpl::getInfoString(int w)
{
	if ( m_sourceinfo.is_streaming )
	{
		switch (w)
		{
		case sProvider:
			return "IPTV";
		case sServiceref:
		{
			eServiceReference ref(m_ref);
			ref.type = eServiceFactoryLibpl::id;
			ref.path.clear();
			return ref.toString();
		}
		default:
			break;
		}
	}

	char * tag = NULL;
	switch (w)
	{
	case sTagTitle:
		tag = strdup("Title");
		break;
	case sTagArtist:
		tag = strdup("Artist");
		break;
	case sTagAlbum:
		tag = strdup("Album");
		break;
	case sTagComment:
		tag = strdup("Comment");
		break;
	case sTagTrackNumber:
		tag = strdup("Track");
		break;
	case sTagGenre:
		tag = strdup("Genre");
		break;
	case sTagDate:
		tag = strdup("Year");
		break;
	case sTagVideoCodec:
		tag = strdup("VideoType");
		break;
	case sTagAudioCodec:
		tag = strdup("AudioType");
		break;
	default:
		return "";
	}

	if (player && player->playback)
	{
		if (player->playback->Command(player, PLAYBACK_INFO, &tag) == 0)
		{
			std::string res (tag);
			free(tag);
			return res;
		}
	}

	return "";
}

RESULT eServiceLibpl::audioChannel(ePtr<iAudioChannelSelection> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceLibpl::audioTracks(ePtr<iAudioTrackSelection> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceLibpl::cueSheet(ePtr<iCueSheet> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceLibpl::subtitle(ePtr<iSubtitleOutput> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceLibpl::audioDelay(ePtr<iAudioDelay> &ptr)
{
	ptr = this;
	return 0;
}

int eServiceLibpl::getNumberOfTracks()
{
 	return m_audioStreams.size();
}

int eServiceLibpl::getCurrentTrack()
{
	return m_currentAudioStream;
}

RESULT eServiceLibpl::selectTrack(unsigned int i)
{
	int ret = selectAudioStream(i);
	return ret;
}

int eServiceLibpl::selectAudioStream(int i)
{
	if (i != m_currentAudioStream)
	{
		if (player && player->playback)
			player->playback->Command(player, PLAYBACK_SWITCH_AUDIO, (void*)&i);
		m_currentAudioStream=i;
		return 0;
	}

	return -1;
}

int eServiceLibpl::getCurrentChannel()
{
	return STEREO;
}

RESULT eServiceLibpl::selectChannel(int i)
{
	eDebug("[eServiceLibpl::%s] %i", __func__,i);
	return 0;
}

RESULT eServiceLibpl::getTrackInfo(struct iAudioTrackInfo &info, unsigned int i)
{
 	if (i >= m_audioStreams.size())
		return -2;
	if (m_audioStreams[i].type == atMPEG)
		info.m_description = "MPEG";
	else if (m_audioStreams[i].type == atMP3)
		info.m_description = "MP3";
	else if (m_audioStreams[i].type == atAC3)
		info.m_description = "AC3";
	else if (m_audioStreams[i].type == atAAC)
		info.m_description = "AAC";
	else if (m_audioStreams[i].type == atDTS)
		info.m_description = "DTS";
	else if (m_audioStreams[i].type == atPCM)
		info.m_description = "PCM";
	else if (m_audioStreams[i].type == atOGG)
		info.m_description = "OGG";
	if (info.m_language.empty())
		info.m_language = m_audioStreams[i].language_code;

	return 0;
}

eAutoInitPtr<eServiceFactoryLibpl> init_eServiceFactoryLibpl(eAutoInitNumbers::service+1, "eServiceFactoryLibpl");

RESULT eServiceLibpl::enableSubtitles(iSubtitleUser *user, struct SubtitleTrack &track)
{
	if (m_currentSubtitleStream != track.pid)
	{
		m_subtitle_pages.clear();
		m_currentSubtitleStream = track.pid;
		m_cachedSubtitleStream = m_currentSubtitleStream;
		m_subtitle_widget = user;
		
		eDebug ("eServiceLibpl::switched to subtitle stream %i", m_currentSubtitleStream);

		if (player && player->playback)
			player->playback->Command(player, PLAYBACK_SWITCH_SUBTITLE, (void*)&track.pid);

		// we have to force a seek, before the new subtitle stream will start
		seekRelative(-1, 90000);
	}

	return 0;
}

RESULT eServiceLibpl::disableSubtitles()
{
	eDebug("[eServiceLibpl::%s]", __func__);
	m_currentSubtitleStream = -1;
	m_cachedSubtitleStream = m_currentSubtitleStream;
	m_subtitle_pages.clear();

	if (m_subtitle_widget) m_subtitle_widget->destroy();

	m_subtitle_widget = 0;

	int pid = -1;

	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_SWITCH_SUBTITLE, (void*)&pid);

	return 0;
}

RESULT eServiceLibpl::getCachedSubtitle(struct SubtitleTrack &track)
{

	bool autoturnon = eConfigManager::getConfigBoolValue("config.subtitles.pango_autoturnon", true);

	if (!autoturnon)
		return -1;

	if (m_cachedSubtitleStream >= 0 && m_cachedSubtitleStream < (int)m_subtitleStreams.size())
	{
		track.type = 2;
		track.pid = m_cachedSubtitleStream;
		track.page_number = int(m_subtitleStreams[m_cachedSubtitleStream].type);
		track.magazine_number = 0;
		return 0;
	}

	return -1;
}

RESULT eServiceLibpl::getSubtitleList(std::vector<struct SubtitleTrack> &subtitlelist)
{
	// 	eDebug("[eServiceLibpl::%s]", __func__);
	int stream_idx = 0;

	for (std::vector<subtitleStream>::iterator IterSubtitleStream(m_subtitleStreams.begin()); IterSubtitleStream != m_subtitleStreams.end(); ++IterSubtitleStream)
	{
		subtype_t type = IterSubtitleStream->type;
		switch(type)
		{
		case stUnknown:
		case stVOB:
		case stPGS:
			break;
		default:
		{
			struct SubtitleTrack track;
			track.type = 2;
			track.pid = stream_idx;
			track.page_number = int(type);
			track.magazine_number = 0;
			track.language_code = IterSubtitleStream->language_code;
			subtitlelist.push_back(track);
		}
		}
		stream_idx++;
	}

	eDebug("[eServiceLibpl::%s] finished", __func__);
	return 0;
}

RESULT eServiceLibpl::streamed(ePtr<iStreamedService> &ptr)
{
	ptr = this;
	return 0;
}

ePtr<iStreamBufferInfo> eServiceLibpl::getBufferCharge()
{
	return new eStreamLibplBufferInfo(m_bufferInfo.bufferPercent, m_bufferInfo.avgInRate, m_bufferInfo.avgOutRate, m_bufferInfo.bufferingLeft, m_buffer_size);
}

/* cuesheet CVR */
PyObject *eServiceLibpl::getCutList()
{
	ePyObject list = PyList_New(0);

	for (std::multiset<struct cueEntry>::iterator i(m_cue_entries.begin()); i != m_cue_entries.end(); ++i)
	{
		ePyObject tuple = PyTuple_New(2);
		PyTuple_SET_ITEM(tuple, 0, PyLong_FromLongLong(i->where));
		PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(i->what));
		PyList_Append(list, tuple);
		Py_DECREF(tuple);
	}

	return list;
}

/* cuesheet CVR */
void eServiceLibpl::setCutList(ePyObject list)
{
	if (!PyList_Check(list))
		return;
	int size = PyList_Size(list);
	int i;

	m_cue_entries.clear();

	for (i=0; i<size; ++i)
	{
		ePyObject tuple = PyList_GET_ITEM(list, i);
		if (!PyTuple_Check(tuple))
		{
			eDebug("[eServiceLibpl::%s] non-tuple in cutlist", __func__);
			continue;
		}
		if (PyTuple_Size(tuple) != 2)
		{
			eDebug("[eServiceLibpl::%s] cutlist entries need to be a 2-tuple", __func__);
			continue;
		}
		ePyObject ppts = PyTuple_GET_ITEM(tuple, 0), ptype = PyTuple_GET_ITEM(tuple, 1);
		if (!(PyLong_Check(ppts) && PyInt_Check(ptype)))
		{
			eDebug("[eServiceLibpl::%s] cutlist entries need to be (pts, type)-tuples (%d %d)", __func__, PyLong_Check(ppts), PyInt_Check(ptype));
			continue;
		}
		pts_t pts = PyLong_AsLongLong(ppts);
		int type = PyInt_AsLong(ptype);
		m_cue_entries.insert(cueEntry(pts, type));
		eDebug("[eServiceLibpl::%s] adding %08llx, %d", __func__, pts, type);
	}
	m_cuesheet_changed = 1;
	m_event((iPlayableService*)this, evCuesheetChanged);
}

void eServiceLibpl::setCutListEnable(int enable)
{
	m_cutlist_enabled = enable;
}

int eServiceLibpl::setBufferSize(int size)
{
	m_buffer_size = size;
	return 0;
}

int eServiceLibpl::getAC3Delay()
{
	return 0;
}

int eServiceLibpl::getPCMDelay()
{
	return 0;
}

void eServiceLibpl::setAC3Delay(int delay)
{
}

void eServiceLibpl::setPCMDelay(int delay)
{
}

void eServiceLibpl::gotThreadMessage(const int &msg)
{
	switch(msg)
	{
	case 1: // thread stopped
		eDebug("[eServiceLibpl::%s] issuing eof...", __func__);
		m_event(this, evEOF);
		break;
	}
}

/* cuesheet CVR */
void eServiceLibpl::loadCuesheet()
{
	if (!m_cuesheet_loaded)
	{
		eDebug("[eServiceLibpl::%s] loading cuesheet", __func__);
		m_cuesheet_loaded = true;
	}

	std::string filename = m_ref.path + ".cuts";

	m_cue_entries.clear();

	FILE *f = fopen(filename.c_str(), "rb");

	if (f)
	{
		while (1)
		{
			unsigned long long where;
			unsigned int what;

			if (!fread(&where, sizeof(where), 1, f))
				break;
			if (!fread(&what, sizeof(what), 1, f))
				break;

			where = be64toh(where);
			what = ntohl(what);

			if (what > 3)
				break;

			m_cue_entries.insert(cueEntry(where, what));
		}
		fclose(f);
		eDebug("[eServiceLibpl::%s] cuts file has %zd entries", __func__, m_cue_entries.size());
	} else
		eDebug("[eServiceLibpl::%s] cutfile not found!", __func__);

	m_cuesheet_changed = 0;
	m_event((iPlayableService*)this, evCuesheetChanged);
}

/* cuesheet CVR */
void eServiceLibpl::saveCuesheet()
{
	std::string filename = m_ref.path;

	/* save cuesheet only when main file is accessible. */
	if (::access(filename.c_str(), R_OK) < 0)
		return;

	filename.append(".cuts");
	/* do not save to file if there are no cuts */
	/* remove the cuts file if cue is empty */
	if(m_cue_entries.begin() == m_cue_entries.end())
	{
		if (::access(filename.c_str(), F_OK) == 0)
			remove(filename.c_str());
		return;
	}

	FILE *f = fopen(filename.c_str(), "wb");

	if (f)
	{
		unsigned long long where;
		int what;

		for (std::multiset<cueEntry>::iterator i(m_cue_entries.begin()); i != m_cue_entries.end(); ++i)
		{
			where = htobe64(i->where);
			what = htonl(i->what);
			fwrite(&where, sizeof(where), 1, f);
			fwrite(&what, sizeof(what), 1, f);

		}
		fclose(f);
	}
	m_cuesheet_changed = 0;
}

void libeplayerThreadStop() // call from libeplayer
{
	eDebug("[eServiceLibpl::%s]", __func__);
	eServiceLibpl *serv = eServiceLibpl::getInstance();
	serv->inst_m_pump->send(1);
}

