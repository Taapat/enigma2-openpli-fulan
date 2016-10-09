#include <lib/base/cfile.h>
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
	m_subtitle_pages = NULL;
	m_currentAudioStream = -1;
	m_currentSubtitleStream = -1;
	m_cachedSubtitleStream = -2; /* report subtitle stream to be 'cached'. TODO: use an actual cache. */
	m_subtitle_widget = 0;
	m_buffer_size = 5 * 1024 * 1024;
	m_paused = false;
	is_streaming = false;
	m_cuesheet_loaded = false; /* cuesheet CVR */
	m_use_chapter_entries = false; /* chapter support CVR */
	m_subtitle_sync_timer = eTimer::create(eApp);
	CONNECT(m_subtitle_sync_timer->timeout, eServiceLibpl::pushSubtitles);
	inst_m_pump = &m_pump;
	CONNECT(m_nownext_timer->timeout, eServiceLibpl::updateEpgCacheNowNext);
	CONNECT(inst_m_pump->recv_msg, eServiceLibpl::gotThreadMessage);
	m_width = m_height = m_aspect = m_framerate = m_progressive = -1;
	m_state = stIdle;
	instance = this;

	player = new Player();

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
	|| (!strncmp("mms://", m_ref.path.c_str(), 6))
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
		is_streaming = true;
	else if ((!strncmp("file://", m_ref.path.c_str(), 7))
	|| (!strncmp("bluray://", m_ref.path.c_str(), 9))
	|| (!strncmp("hls+file://", m_ref.path.c_str(), 11))
	|| (!strncmp("myts://", m_ref.path.c_str(), 7)))
		is_streaming = false;
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
	if (player->Open(file, is_streaming, ""))
	{
		eDebug("[eServiceLibpl::%s] Open file!", __func__);

		std::vector<Track> tracks = player->manager.getAudioTracks();
		if(!tracks.empty())
		{
			eDebug("[eServiceLibpl::%s] Audio track list:", __func__);
			for (std::vector<Track>::iterator it = tracks.begin(); it != tracks.end(); ++it) 
			{
				eDebug("[eServiceLibpl::%s]    Id:%i type:%i language:%s", __func__, it->pid, it->type, it->title.c_str());
				audioStream audio;
				audio.language_code = it->title;
				audio.pid = it->pid;
				audio.type = it->type;

				m_audioStreams.push_back(audio);
			}
			m_currentAudioStream = 0;
		}

		tracks = player->manager.getSubtitleTracks();
		if(!tracks.empty())
		{
			eDebug("[eServiceLibpl::%s] Subtitle track list:", __func__);
			for (std::vector<Track>::iterator it = tracks.begin(); it != tracks.end(); ++it) 
			{
				eDebug("[eServiceLibpl::%s]    Id:%i type:%i language:%s", __func__, it->pid, it->type, it->title.c_str());
				subtitleStream subtitle;
				subtitle.language_code = it->title;
				subtitle.id = it->pid;
				subtitle.type = it->type;

				m_subtitleStreams.push_back(subtitle);
			}
		}

		loadCuesheet(); /* cuesheet CVR */

		if (!strncmp(file, "file://", 7)) /* text subtitles */
			ReadTextSubtitles(file);
	}
	else
	{
		//Creation failed, no playback support for insert file, so send e2 EOF to stop playback
		eDebug("[eServiceLibpl::%s] ERROR! Creation failed! No playback support for insert file!", __func__);
		m_state = stStopped;
		m_event((iPlayableService*)this, evEOF);
		m_event((iPlayableService*)this, evUser+12);
	}
}

eServiceLibpl::~eServiceLibpl()
{
	if (m_subtitle_widget) m_subtitle_widget->destroy();
	m_subtitle_widget = 0;

	if (m_state == stRunning)
		stop();

	delete player;
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

void eServiceLibpl::ReadSrtSubtitle(const char *subfile, int delay, double convert_fps)
{
	int horIni, minIni, secIni, milIni, horFim, minFim, secFim, milFim;
	char *Text = NULL;

	CFile f(subfile, "rt");
	if (f)
	{
		int pos = 0;
		int64_t start_ms = 0;
		int64_t end_ms = 0;
		size_t bufsize = 256;
		char *line = (char*) malloc(bufsize);
		while (getline(&line, &bufsize, f) != -1)
		{
			/*
			00:02:17,440 --> 00:02:20,375
			Senator, we're making
			our final approach into Coruscant.
			*/
			if(pos == 0)
			{
				if(line[0] == '\n' || line[0] == '\0' || line[0] == 13 /* ^M */)
					continue; /* Empty line not allowed here */
				pos++;
			}
			else if(pos == 1)
			{
				if (sscanf(line, "%d:%d:%d%*1[,.]%d --> %d:%d:%d%*1[,.]%d",
					&horIni, &minIni, &secIni, &milIni, &horFim, &minFim, &secFim, &milFim) != 8)
				{
					continue; /* Data is not in correct format */
				}

				start_ms = ((horIni * 3600 + minIni * 60 + secIni) * 1000 + milIni) * convert_fps + delay;
				end_ms = ((horFim * 3600 + minFim * 60 + secFim) * 1000  + milFim) * convert_fps + delay;
				pos++;
			}
			else if(pos == 2)
			{
				if(line[0] == '\n' || line[0] == '\0' || line[0] == 13 /* ^M */)
				{
					if(Text != NULL)
					{
						int sl = strlen(Text)-1;
						Text[sl]='\0'; /* Set last to \0, to replace \n or \r if exist */

						subtitleData sub;
						sub.start_ms = start_ms;
						sub.duration_ms = end_ms - start_ms;
						sub.end_ms = end_ms;
						sub.text = (const char *)Text;

						m_srt_subtitle_pages.insert(subtitle_pages_map_pair(sub.end_ms, sub));
						free(Text);
						Text = NULL;
					}
					pos = 0;
					continue;
				}

				if(!Text)
				{
					Text = strdup(line);
				}
				else
				{
					int length = strlen(Text) /* \0 -> \n */ + strlen(line) + 2 /* \0 */;
					char *tmpText = Text;
					Text = (char *)malloc(length);

					strcpy(Text, tmpText);
					strcat(Text, line);
					free(tmpText);
				}
			}
		} /* while */
		if(Text != NULL)
		{
			free(Text);
			Text = NULL;
		}
	}

	if(!m_srt_subtitle_pages.empty())
	{
		subtitleStream sub;
		sub.language_code = "SRT";
		sub.type = 4;
		m_subtitleStreams.push_back(sub);
	}
}

void eServiceLibpl::ReadSsaSubtitle(const char *subfile, int isASS, int delay, double convert_fps)
{
	int horIni, minIni, secIni, milIni, horFim, minFim, secFim, milFim;
	char *Text = NULL;

	CFile f(subfile, "rt");
	if (f)
	{
		size_t bufsize = 256;
		char *line = (char*) malloc(bufsize);
		while (getline(&line, &bufsize, f) != -1)
		{
			/*
			Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
			Dialogue: Marked=0,0:02:40.65,0:02:41.79,Wolf main,Cher,0000,0000,0000,,Hello world!
			*/
			if(line[0]  != 'D')
				continue; /* Skip line without Dialogue */

			int i = 0;
			int ret = 0;
			char *p_newline = NULL;
			char *ptr = line;

			while(i < 10 && *ptr != '\0')
			{
				if (*ptr == ',')
				{
					i++;
				}
				ptr++;
				if (i == 1)
				{
					ret = sscanf(ptr, "%d:%d:%d.%d,%d:%d:%d.%d,", &horIni, &minIni, &secIni, &milIni, &horFim, &minFim, &secFim, &milFim);
					i++;
				}
			}

			if (ret != 8)
			{
				continue; /* Data is not in correct format */
			}

			int64_t start_ms = ((horIni * 3600 + minIni * 60 + secIni) * 1000 + milIni) * convert_fps + delay;
			int64_t end_ms = ((horFim * 3600 + minFim * 60 + secFim) * 1000  + milFim) * convert_fps + delay;

			/* standardize hard break: '\N'->'\n' http://docs.aegisub.org/3.2/ASS_Tags/ */
			while((p_newline = strstr(ptr, "\\N")) != NULL)
			{
				*(p_newline + 1) = 'n';
			}

			Text = strdup(ptr);
			int sl = strlen(Text)-1;
			Text[sl]='\0'; /* Set last to \0, to replace \n or \r if exist */

			if(Text != NULL)
			{
				subtitleData sub;
				sub.start_ms = start_ms;
				sub.duration_ms = end_ms - start_ms;
				sub.end_ms = end_ms;
				sub.text = (const char *)Text;

				if (isASS)
				{
					m_ass_subtitle_pages.insert(subtitle_pages_map_pair(sub.end_ms, sub));
				}
				else
				{
					m_ssa_subtitle_pages.insert(subtitle_pages_map_pair(sub.end_ms, sub));
				}
				free(Text);
				Text = NULL;
			}
		} /* while */
	}

	if(!m_ass_subtitle_pages.empty() || !m_ssa_subtitle_pages.empty())
	{
		subtitleStream sub;
		if (isASS)
		{
			sub.language_code = "ASS";
			sub.type = 3;
		}
		else
		{
			sub.language_code = "SSA";
			sub.type = 2;
		}
		m_subtitleStreams.push_back(sub);
	}
}

void eServiceLibpl::ReadTextSubtitles(const char *filename)
{
	int delay = eConfigManager::getConfigIntValue("config.subtitles.pango_subtitles_delay") / 90;
	int subtitle_fps = eConfigManager::getConfigIntValue("config.subtitles.pango_subtitles_fps");

	double convert_fps = 1.0;
	if (subtitle_fps > 1 && m_framerate > 0)
		convert_fps = subtitle_fps / (double)m_framerate;

	filename += 7; // remove 'file://'
	const char *lastDot = strrchr(filename, '.');
	if (!lastDot)
		return;
	char subfile[strlen(filename) + 3];

	strcpy(subfile, filename);
	strcpy(subfile + (lastDot + 1 - filename), "srt");
	if (::access(subfile, R_OK) == 0)
	{
		eDebug("[eServiceLibpl::%s] add %s", __func__, subfile);
		ReadSrtSubtitle(subfile, delay, convert_fps);
	}

	strcpy(subfile, filename);
	strcpy(subfile + (lastDot + 1 - filename), "ass");
	if (::access(subfile, R_OK) == 0)
	{
		eDebug("[eServiceLibpl::%s] add %s", __func__, subfile);
		ReadSsaSubtitle(subfile, 0, delay, convert_fps);
	}

	strcpy(subfile, filename);
	strcpy(subfile + (lastDot + 1 - filename), "ssa");
	if (::access(subfile, R_OK) == 0)
	{
		eDebug("[eServiceLibpl::%s] add %s", __func__, subfile);
		ReadSsaSubtitle(subfile, 1, delay, convert_fps);
	}
}

void eServiceLibpl::pullTextSubtitles(int type)
{
	eDebug("[eServiceLibpl::%s] type %d", __func__, type);

	if (type == 4)
	{
		m_subtitle_pages = &m_srt_subtitle_pages;
	}
	else if (type == 3)
	{
		m_subtitle_pages = &m_ass_subtitle_pages;
	}
	else
	{
		m_subtitle_pages = &m_ssa_subtitle_pages;
	}

	m_subtitle_sync_timer->start(1, true);
}

void eServiceLibpl::pullSubtitle()
{
	if(m_state != stRunning || player->output.embedded_subtitle.empty())
		return;

	int delay = eConfigManager::getConfigIntValue("config.subtitles.pango_subtitles_delay") / 90;
	int subtitle_fps = eConfigManager::getConfigIntValue("config.subtitles.pango_subtitles_fps");

	double convert_fps = 1.0;
		if (subtitle_fps > 1 && m_framerate > 0)
			convert_fps = subtitle_fps / (double)m_framerate;

	eSingleLocker lock(m_subtitle_lock);
	subtitle_pages_map::iterator current;
	for (current = player->output.embedded_subtitle.begin(); current != player->output.embedded_subtitle.end(); current++)
	{
		subtitleData sub = current->second;
		sub.start_ms = sub.start_ms * convert_fps + delay;
		sub.end_ms = sub.start_ms + sub.duration_ms;
		m_emb_subtitle_pages.insert(subtitle_pages_map_pair(sub.end_ms, sub));
	}
	player->output.embedded_subtitle.clear();
	m_subtitle_sync_timer->start(1, true);
}

void eServiceLibpl::pushSubtitles()
{
	if (!m_subtitle_pages)
		return;

	pts_t running_pts = 0;
	int32_t next_timer = 0, decoder_ms, start_ms, end_ms, diff_start_ms, diff_end_ms;
	subtitle_pages_map::const_iterator current;

	if (getPlayPosition(running_pts) < 0)
	{
		next_timer = 50;
		goto exit;
	}

	decoder_ms = running_pts / 90;

	for (current = m_subtitle_pages->lower_bound(decoder_ms); current != m_subtitle_pages->end(); current++)
	{
		start_ms = current->second.start_ms;
		end_ms = current->second.end_ms;
		diff_start_ms = start_ms - decoder_ms;
		diff_end_ms = end_ms - decoder_ms;

		// eDebug("[eServiceLibpl::%s] *** next subtitle: decoder: %d, start: %d, end: %d, duration_ms: %d, diff_start: %d, diff_end: %d : %s", __func__, decoder_ms, start_ms, end_ms, end_ms - start_ms, diff_start_ms, diff_end_ms, current->second.text.c_str());

		if (diff_end_ms < 0)
		{
			// eDebug("[eServiceLibpl::%s] *** current sub has already ended, skip: %d", __func__, diff_end_ms);
			continue;
		}
		if (diff_start_ms > 50)
		{
			// eDebug("[eServiceLibpl::%s] *** current sub in the future, start timer, %d", __func__, diff_start_ms);
			next_timer = diff_start_ms;
			goto exit;
		}
		if (m_subtitle_widget && !m_paused)
		{
			// eDebug("[eServiceLibpl::%s] current sub actual, show!", __func__);

			ePangoSubtitlePage pango_page;
			gRGB rgbcol(0xD0,0xD0,0xD0);

			pango_page.m_elements.push_back(ePangoSubtitlePageElement(rgbcol, current->second.text.c_str()));
			pango_page.m_show_pts = start_ms * 90; // actually completely unused by widget!
			pango_page.m_timeout = end_ms - decoder_ms; // take late start into account

			m_subtitle_widget->setPage(pango_page);
		}
	}

exit:
	if (next_timer == 0)
	{
		// eDebug("[eServiceLibpl::%s] *** next timer = 0, set default timer!", __func__);
		next_timer = 1000;
	}

	m_subtitle_sync_timer->start(next_timer, true);
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
		eDebug("[eServiceLibpl::%s] state is not idle", __func__);
		return -1;
	}

	if (player && player->output.Open() && player->Play())
	{
		m_state = stRunning;

		int autoaudio = 0;
		int autoaudio_level = 5;
		std::string configvalue;
		std::vector<std::string> autoaudio_languages;
		configvalue = eConfigManager::getConfigValue("config.autolanguage.audio_autoselect1");
		if (configvalue != "" && configvalue != "None")
			autoaudio_languages.push_back(configvalue);
		configvalue = eConfigManager::getConfigValue("config.autolanguage.audio_autoselect2");
		if (configvalue != "" && configvalue != "None")
			autoaudio_languages.push_back(configvalue);
		configvalue = eConfigManager::getConfigValue("config.autolanguage.audio_autoselect3");
		if (configvalue != "" && configvalue != "None")
			autoaudio_languages.push_back(configvalue);
		configvalue = eConfigManager::getConfigValue("config.autolanguage.audio_autoselect4");
		if (configvalue != "" && configvalue != "None")
			autoaudio_languages.push_back(configvalue);
		for (int i = 0; i < m_audioStreams.size() && !autoaudio; i++)
		{
			if (!m_audioStreams[i].language_code.empty())
			{
				int x = 1;
				for (std::vector<std::string>::iterator it = autoaudio_languages.begin(); x < autoaudio_level && it != autoaudio_languages.end(); x++, it++)
				{
					if ((*it).find(m_audioStreams[i].language_code) != std::string::npos)
					{
						autoaudio = i;
						autoaudio_level = x;
						break;
					}
				}
			}
		}

		if (autoaudio)
		{
			selectAudioStream(autoaudio);
			m_currentAudioStream = autoaudio;
		}

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
		eDebug("[eServiceLibpl::%s] state is idle", __func__);
		return -1;
	}

	if (m_state == stStopped)
	{
		eDebug("[eServiceLibpl::%s] state is stoped", __func__);
		return -1;
	}

	eDebug("[eServiceLibpl::%s] stop %s", __func__, m_ref.path.c_str());

	player->RequestAbort();
	player->Stop();
	player->output.Close();
	player->Close();

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

	if (m_state == stRunning && speed != -1 && ratio > 1)
	{
		if (player->SlowMotion(speed))
		{
			eDebug("[eServiceLibpl::%s] ERROR!", __func__);
			return -1;
		}
	}

	return 0;
}

RESULT eServiceLibpl::setFastForward(int ratio)
{
	int speed = getSpeed(ratio);

	int res = 0;

	if (m_state == stRunning && speed != -1)
	{
		if (ratio > 1)
			res = player->FastForward(speed);
		else if (ratio < -1)
			res = player->FastBackward(speed);
		else /* speed == 1 */
			res = player->Continue();

		if (res)
			eDebug("[eServiceLibpl::%s] ERROR!", __func__);
	}

	return 0;
}

		// iPausableService
RESULT eServiceLibpl::pause()
{
	if (m_state != stRunning)
		return 0;

	return player->Pause();
	m_paused = true;
}

RESULT eServiceLibpl::unpause()
{
	if (m_state != stRunning)
		return 0;

	return player->Continue();
	m_paused = false;
}

	/* iSeekableService */
RESULT eServiceLibpl::seek(ePtr<iSeekableService> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceLibpl::getLength(pts_t &pts)
{
	if (m_state != stRunning)
		return 0;

	int64_t length = 0;
	player->GetDuration(length);

	if (length > 0)
	{
		pts = length * 90000 / AV_TIME_BASE;
	}
	else
	{
		length = 0;
		player->GetPts(length);
		if (length > 0)
			pts = length + AV_TIME_BASE / 90000;
		else
			return -1;
	}
	return 0;
}

RESULT eServiceLibpl::seekTo(pts_t to)
{
	if (m_state != stRunning)
		return 0;

	player->Seek((int64_t)to * AV_TIME_BASE / 90000, true);

	if(m_currentSubtitleStream >= 0 && m_emb_subtitle_pages.size())
		m_emb_subtitle_pages.clear();

	return 0;
}

RESULT eServiceLibpl::seekRelative(int direction, pts_t to)
{
	if (m_state != stRunning)
		return 0;

	player->Seek((int64_t)to * direction * AV_TIME_BASE / 90000, false);

	if(m_currentSubtitleStream >= 0 && m_emb_subtitle_pages.size())
		m_emb_subtitle_pages.clear();

	return 0;
}

RESULT eServiceLibpl::getPlayPosition(pts_t &pts)
{
	pts = 0;

	if(m_state != stRunning)
		return -1;

	if (!player->isPlaying)
	{
		eDebug("[eServiceLibpl::%s] !!!!EOF!!!!", __func__);
		m_event((iPlayableService*)this, evEOF);
		return -1;
	}

	int64_t vpts = 0;
	player->GetPts(vpts);

	if (vpts <= 0)
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
	if (is_streaming)
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

	if(m_state != stRunning)
		return "";

	char tag[16] = {""};
	switch (w)
	{
	case sTagTitle:
		strcat(tag, "title");
		break;
	case sTagArtist:
		strcat(tag, "artist");
		break;
	case sTagAlbum:
		strcat(tag, "album");
		break;
	case sTagComment:
		strcat(tag, "comment");
		break;
	case sTagTrackNumber:
		strcat(tag, "track");
		break;
	case sTagGenre:
		strcat(tag, "genre");
		break;
	case sTagDate:
		strcat(tag, "date");
		break;
	default:
		return "";
	}

	if (m_metaCount == 0)
	{
		player->input.GetMetadata(m_metaKeys, m_metaValues);
		m_metaCount = m_metaKeys.size();
	}

	if (m_metaCount > 0)
	{
		for (size_t i = 0; i < m_metaCount; i++)
		{
			if (strcmp(tag, m_metaKeys[i].c_str()) == 0)
				return m_metaValues[i].c_str();
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
	if (m_state == stRunning && i != m_currentAudioStream)
	{
		player->SwitchAudio(m_audioStreams[i].pid);
		m_currentAudioStream = i;
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

	switch(m_audioStreams[i].type)
	{
		case 1:
			info.m_description = "MPEG";
			break;
		case 2:
			info.m_description = "MP3";
			break;
		case 3:
			info.m_description = "AC3";
			break;
		case 4:
			info.m_description = "DTS";
			break;
		case 5:
			info.m_description = "AAC";
			break;
		case 0:
		case 6:
			info.m_description = "PCM";
			break;
		case 8:
			info.m_description = "FLAC";
			break;
		case 9:
			info.m_description = "WMA";
			break;
		default:
			break;
	}

	if (info.m_language.empty())
		info.m_language = m_audioStreams[i].language_code;

	return 0;
}

eAutoInitPtr<eServiceFactoryLibpl> init_eServiceFactoryLibpl(eAutoInitNumbers::service+1, "eServiceFactoryLibpl");

RESULT eServiceLibpl::enableSubtitles(iSubtitleUser *user, struct SubtitleTrack &track)
{
	if (m_state == stRunning && m_currentSubtitleStream != track.pid)
	{
		m_currentSubtitleStream = track.pid;
		m_cachedSubtitleStream = m_currentSubtitleStream;
		m_subtitle_widget = user;
		
		eDebug ("[eServiceLibpl::%s] switched to subtitle stream %i, type %d", __func__, m_currentSubtitleStream, track.page_number);

		if (track.page_number > 1)
		{
			pullTextSubtitles(track.page_number);
		}
		else
		{
			m_emb_subtitle_pages.clear();
			m_subtitle_pages = &m_emb_subtitle_pages;
			player->SwitchSubtitle(track.magazine_number);
		}
	}

	return 0;
}

RESULT eServiceLibpl::disableSubtitles()
{
	eDebug("[eServiceLibpl::%s]", __func__);

	if(m_state != stRunning)
		return 0;

	player->SwitchSubtitle(-1);
	m_currentSubtitleStream = -1;
	m_cachedSubtitleStream = m_currentSubtitleStream;
	m_subtitle_pages = NULL;
	m_emb_subtitle_pages.clear();

	if (m_subtitle_widget)
		m_subtitle_widget->destroy();

	m_subtitle_widget = 0;
	return 0;
}

RESULT eServiceLibpl::getCachedSubtitle(struct SubtitleTrack &track)
{
	bool autoturnon = eConfigManager::getConfigBoolValue("config.subtitles.pango_autoturnon", true);
	int m_subtitleStreams_size = (int)m_subtitleStreams.size();
	if (!autoturnon)
		return -1;

	if (m_cachedSubtitleStream == -2 && m_subtitleStreams_size)
	{
		m_cachedSubtitleStream = m_subtitleStreams_size - 1;
		if (m_subtitleStreams[m_cachedSubtitleStream].type < 2)
		{
			int autosub_level = 5;
			std::string configvalue;
			std::vector<std::string> autosub_languages;
			configvalue = eConfigManager::getConfigValue("config.autolanguage.subtitle_autoselect1");
			if (configvalue != "" && configvalue != "None")
				autosub_languages.push_back(configvalue);
			configvalue = eConfigManager::getConfigValue("config.autolanguage.subtitle_autoselect2");
			if (configvalue != "" && configvalue != "None")
				autosub_languages.push_back(configvalue);
			configvalue = eConfigManager::getConfigValue("config.autolanguage.subtitle_autoselect3");
			if (configvalue != "" && configvalue != "None")
				autosub_languages.push_back(configvalue);
			configvalue = eConfigManager::getConfigValue("config.autolanguage.subtitle_autoselect4");
			if (configvalue != "" && configvalue != "None")
				autosub_languages.push_back(configvalue);
			for (int i = 0; i < m_subtitleStreams_size; i++)
			{
				if (!m_subtitleStreams[i].language_code.empty())
				{
					int x = 1;
					for (std::vector<std::string>::iterator it2 = autosub_languages.begin(); x < autosub_level && it2 != autosub_languages.end(); x++, it2++)
					{
						if ((*it2).find(m_subtitleStreams[i].language_code) != std::string::npos)
						{
							autosub_level = x;
							m_cachedSubtitleStream = i;
							break;
						}
					}
				}
			}
		}
	}

	if (m_cachedSubtitleStream >= 0 && m_cachedSubtitleStream < m_subtitleStreams_size)
	{
		track.type = 2;
		track.pid = m_cachedSubtitleStream;
		track.page_number = m_subtitleStreams[m_cachedSubtitleStream].type;
		track.magazine_number = m_subtitleStreams[m_cachedSubtitleStream].id;
		return 0;
	}
	return -1;
}

RESULT eServiceLibpl::getSubtitleList(std::vector<struct SubtitleTrack> &subtitlelist)
{
	int stream_idx = 0;

	for (std::vector<subtitleStream>::iterator IterSubtitleStream(m_subtitleStreams.begin()); IterSubtitleStream != m_subtitleStreams.end(); ++IterSubtitleStream)
	{
		struct SubtitleTrack track;
		track.type = 2;
		track.pid = stream_idx;
		track.page_number = IterSubtitleStream->type;
		track.magazine_number = IterSubtitleStream->id;
		track.language_code = IterSubtitleStream->language_code;
		subtitlelist.push_back(track);

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

void eServiceLibpl::videoSizeChanged()
{
	if (m_state != stRunning)
		return;

	m_width = player->output.videoInfo.width;
	m_height = player->output.videoInfo.height;
	m_aspect = player->output.videoInfo.aspect;
	eDebug("[eServiceLibpl::%s] width:%d height:%d aspect:%d", __func__, m_width, m_height, m_aspect);
	m_event((iPlayableService*)this, evVideoSizeChanged);
}

void eServiceLibpl::videoFramerateChanged()
{
	if (m_state != stRunning)
		return;

	m_framerate = player->output.videoInfo.frame_rate;
	eDebug("[eServiceLibpl::%s] framerate:%d", __func__, m_framerate);
	m_event((iPlayableService*)this, evVideoFramerateChanged);
}

void eServiceLibpl::videoProgressiveChanged()
{
	if (m_state != stRunning)
		return;

	m_progressive = player->output.videoInfo.progressive;
	eDebug("[eServiceLibpl::%s] progressive:%d", __func__, m_framerate);
	m_event((iPlayableService*)this, evVideoProgressiveChanged);
}

void eServiceLibpl::getChapters()
{
	eDebug("[eServiceLibpl::%s]", __func__);

	if (m_state != stRunning)
		return;

	std::vector<int> positions;
	player->GetChapters(positions);

	if (!positions.empty())
	{
		m_use_chapter_entries = true;
		if (m_cuesheet_loaded)
			m_cue_entries.clear();

		for (unsigned int i = 0; i < positions.size(); i++)
		{
			/* first chapter is movie start no cut needed */
			if (i > 0)
			{
				if (positions[i] > 0)
					m_cue_entries.insert(cueEntry(positions[i], 2));
			}
		}

		m_cuesheet_changed = 1;
		m_event((iPlayableService*)this, evCuesheetChanged);
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

	/* save cuesheet only when main file is accessible and no libeplayer chapters avbl*/
	if ((::access(filename.c_str(), R_OK) < 0) || m_use_chapter_entries)
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

void eServiceLibpl::gotThreadMessage(const int &msg)
{
	switch(msg)
	{
	case 0:
		pullSubtitle();
		break;
	case 1: // thread stopped
		eDebug("[eServiceLibpl::%s] issuing eof...", __func__);
		m_event(this, evEOF);
		break;
	case 2:
		videoSizeChanged();
		break;
	case 3:
		videoFramerateChanged();
		break;
	case 4:
		videoProgressiveChanged();
		break;
	case 5:
		getChapters();
		break;
	}
}

void libeplayerMessage(int message) // call from libeplayer
{
	// eDebug("[eServiceLibpl::%s] %d", __func__, message);
	eServiceLibpl *serv = eServiceLibpl::getInstance();
	serv->inst_m_pump->send(message);
}

