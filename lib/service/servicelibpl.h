#ifndef __servicelibpl_h
#define __servicelibpl_h

#include <lib/base/message.h>
#include <lib/service/iservice.h>
#include <lib/dvb/pmt.h>
#include <lib/dvb/subtitle.h>
#include <lib/dvb/teletext.h>
#include <lib/gui/esubtitle.h>

#include <libeplayer3/player.h>

#include "m3u8.h"

#define gint int
#define gint64 int64_t

class eStaticServiceLibplInfo;

class eServiceFactoryLibpl: public iServiceHandler
{
	DECLARE_REF(eServiceFactoryLibpl);

public:
	eServiceFactoryLibpl();
	virtual ~eServiceFactoryLibpl();
#ifdef ENABLE_GSTREAMER
	enum {
		id = 4097,
		idServiceLibpl = 5003
	};
#else
	enum { id = 0x1001 };
#endif

	// iServiceHandler
	RESULT play(const eServiceReference &, ePtr<iPlayableService> &ptr);
	RESULT record(const eServiceReference &, ePtr<iRecordableService> &ptr);
	RESULT list(const eServiceReference &, ePtr<iListableService> &ptr);
	RESULT info(const eServiceReference &, ePtr<iStaticServiceInformation> &ptr);
	RESULT offlineOperations(const eServiceReference &, ePtr<iServiceOfflineOperations> &ptr);

private:
	ePtr<eStaticServiceLibplInfo> m_service_info;
#ifdef ENABLE_GSTREAMER
	bool defaultMP3_Player;
#endif
};

class eStaticServiceLibplInfo: public iStaticServiceInformation
{
	DECLARE_REF(eStaticServiceLibplInfo);
	friend class eServiceFactoryLibpl;
	eStaticServiceLibplInfo();

public:
	RESULT getName(const eServiceReference &ref, std::string &name);
	int getLength(const eServiceReference &ref);
	int getInfo(const eServiceReference &ref, int w);
	int isPlayable(const eServiceReference &ref, const eServiceReference &ignore, bool simulate) { return 1; }
	long long getFileSize(const eServiceReference &ref);
	RESULT getEvent(const eServiceReference &ref, ePtr<eServiceEvent> &ptr, time_t start_time);
};

class eStreamLibplBufferInfo: public iStreamBufferInfo
{
	DECLARE_REF(eStreamLibplBufferInfo);
	int bufferPercentage;
	int inputRate;
	int outputRate;
	int bufferSpace;
	int bufferSize;

public:
	eStreamLibplBufferInfo(int percentage, int inputrate, int outputrate, int space, int size);
	int getBufferPercentage() const;
	int getAverageInputRate() const;
	int getAverageOutputRate() const;
	int getBufferSpace() const;
	int getBufferSize() const;
};

class eServiceLibpl: public iPlayableService, public iPauseableService,
	public iServiceInformation, public iSeekableService, public iAudioTrackSelection, public iAudioChannelSelection,
	public iSubtitleOutput, public iStreamedService, public iAudioDelay, public Object, public iCueSheet
{
	DECLARE_REF(eServiceLibpl);

public:
	virtual ~eServiceLibpl();
	static eServiceLibpl *getInstance();
	eFixedMessagePump<int> (*inst_m_pump);

	// iPlayableService
	RESULT connectEvent(const Slot2<void,iPlayableService*,int> &event, ePtr<eConnection> &connection);
	RESULT start();
	RESULT stop();
	RESULT setTarget(int target);
	RESULT pause(ePtr<iPauseableService> &ptr);
	RESULT setSlowMotion(int ratio);
	RESULT setFastForward(int ratio);
	RESULT seek(ePtr<iSeekableService> &ptr);
	RESULT audioTracks(ePtr<iAudioTrackSelection> &ptr);
	RESULT audioChannel(ePtr<iAudioChannelSelection> &ptr);
	RESULT subtitle(ePtr<iSubtitleOutput> &ptr);
	RESULT audioDelay(ePtr<iAudioDelay> &ptr);
	RESULT cueSheet(ePtr<iCueSheet> &ptr);

	// not implemented (yet)
	RESULT frontendInfo(ePtr<iFrontendInformation> &ptr) { ptr = 0; return -1; }
	RESULT subServices(ePtr<iSubserviceList> &ptr) { ptr = 0; return -1; }
	RESULT timeshift(ePtr<iTimeshiftService> &ptr) { ptr = 0; return -1; }

	// iCueSheet
	PyObject *getCutList();
	void setCutList(SWIG_PYOBJECT(ePyObject));
	void setCutListEnable(int enable);

	RESULT rdsDecoder(ePtr<iRdsDecoder> &ptr) { ptr = 0; return -1; }
	RESULT keys(ePtr<iServiceKeys> &ptr) { ptr = 0; return -1; }
	RESULT stream(ePtr<iStreamableService> &ptr) { ptr = 0; return -1; }

	// iPausableService
	RESULT pause();
	RESULT unpause();
	RESULT info(ePtr<iServiceInformation>&);

	// iSeekableService
	RESULT getLength(pts_t &SWIG_OUTPUT);
	RESULT seekTo(pts_t to);
	RESULT seekRelative(int direction, pts_t to);
	RESULT getPlayPosition(pts_t &SWIG_OUTPUT);
	RESULT setTrickmode(int trick);
	RESULT isCurrentlySeekable();

	// iServiceInformation
	RESULT getName(std::string &name);
	RESULT getEvent(ePtr<eServiceEvent> &evt, int nownext);
	int getInfo(int w);
	std::string getInfoString(int w);

	// iAudioTrackSelection
	int getNumberOfTracks();
	RESULT selectTrack(unsigned int i);
	RESULT getTrackInfo(struct iAudioTrackInfo &, unsigned int n);
	int getCurrentTrack();

	// iAudioChannelSelection
	int getCurrentChannel();
	RESULT selectChannel(int i);

	// iSubtitleOutput
	RESULT enableSubtitles(iSubtitleUser *user, SubtitleTrack &track);
	RESULT disableSubtitles();
	RESULT getSubtitleList(std::vector<SubtitleTrack> &sublist);
	RESULT getCachedSubtitle(SubtitleTrack &track);

	// iStreamedService
	RESULT streamed(ePtr<iStreamedService> &ptr);
	ePtr<iStreamBufferInfo> getBufferCharge();
	int setBufferSize(int size);

	// iAudioDelay
	int getAC3Delay();
	int getPCMDelay();
	void setAC3Delay(int);
	void setPCMDelay(int);

	struct audioStream
	{
		int type;
		int pid;
		std::string language_code; /* iso-639, if available. */
		audioStream()
		{
		}
	};
	struct subtitleStream
	{
		int type;
		std::string language_code; /* iso-639, if available. */
		int id;
		subtitleStream()
		{
		}
	};
	struct bufferInfo
	{
		gint bufferPercent;
		gint avgInRate;
		gint avgOutRate;
		gint64 bufferingLeft;
		bufferInfo()
			:bufferPercent(100), avgInRate(0), avgOutRate(0), bufferingLeft(-1)
		{
		}
	};
	struct errorInfo
	{
		std::string error_message;
		std::string missing_codec;
	};

protected:
	ePtr<eTimer> m_nownext_timer;
	ePtr<eServiceEvent> m_event_now, m_event_next;
	void updateEpgCacheNowNext();
	static eServiceLibpl *instance;

	// cuesheet
	struct cueEntry
	{
		pts_t where;
		unsigned int what;

		bool operator < (const struct cueEntry &o) const
		{
			return where < o.where;
		}
		cueEntry(const pts_t &where, unsigned int what) :
			where(where), what(what)
		{
		}
	};
	std::multiset<cueEntry> m_cue_entries;
	int m_cuesheet_changed, m_cutlist_enabled;
	void loadCuesheet();
	void saveCuesheet();
private:
	int m_currentAudioStream;
	int m_currentSubtitleStream;
	int m_cachedSubtitleStream;
	int selectAudioStream(int i);
	std::vector<audioStream> m_audioStreams;
	std::vector<subtitleStream> m_subtitleStreams;
	iSubtitleUser *m_subtitle_widget;
	friend class eServiceFactoryLibpl;
	eServiceReference m_ref;
	int m_buffer_size;
	bool m_paused;
	bool is_streaming;
	// cuesheet load check
	bool m_cuesheet_loaded;
	bool m_use_chapter_entries;
	bufferInfo m_bufferInfo;
	eServiceLibpl(eServiceReference ref);
	Signal2<void, iPlayableService*, int> m_event;
	enum
	{
		stIdle, stRunning, stStopped,
	};

	int m_state;
	Player *player;
	eFixedMessagePump<int> m_pump;
	void gotThreadMessage(const int &);

	std::vector<std::string> m_metaKeys;
	std::vector<std::string> m_metaValues;
	size_t m_metaCount;

	typedef std::map<uint32_t, subtitleData> subtitle_pages_map;
	typedef std::pair<uint32_t, subtitleData> subtitle_pages_map_pair;
	subtitle_pages_map const *m_subtitle_pages;
	subtitle_pages_map m_emb_subtitle_pages;
	subtitle_pages_map m_srt_subtitle_pages;
	subtitle_pages_map m_ass_subtitle_pages;
	subtitle_pages_map m_ssa_subtitle_pages;
	ePtr<eTimer> m_subtitle_sync_timer;

	void ReadSrtSubtitle(const char *subfile, int delay, double convert_fps);
	void ReadSsaSubtitle(const char *subfile, int isASS, int delay, double convert_fps);
	void ReadTextSubtitles(const char *filename);
	void pullTextSubtitles(int type);
	void pushSubtitles();
	void pullSubtitle();
	void videoSizeChanged();
	void videoFramerateChanged();
	void videoProgressiveChanged();
	void getChapters();
	gint m_aspect, m_width, m_height, m_framerate, m_progressive;
};

#endif
