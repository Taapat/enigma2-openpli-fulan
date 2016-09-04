#ifndef __servicelibpl_h
#define __servicelibpl_h

#include <lib/base/message.h>
#include <lib/service/iservice.h>
#include <lib/dvb/pmt.h>
#include <lib/dvb/subtitle.h>
#include <lib/dvb/teletext.h>
#include <lib/gui/esubtitle.h>

#include "m3u8.h"

extern "C"
{
#ifndef __STDC_CONSTANT_MACROS
#  define __STDC_CONSTANT_MACROS
#  define UINT64_C
#endif
#include <libeplayer3/include/common.h>
#include <libeplayer3/include/subtitle.h>
}

#define gint int
#define gint64 int64_t
extern OutputHandler_t		OutputHandler;
extern PlaybackHandler_t	PlaybackHandler;
extern ContainerHandler_t	ContainerHandler;
extern ManagerHandler_t	ManagerHandler;

class eStaticServiceMP3Info;

class eServiceFactoryMP3: public iServiceHandler
{
	DECLARE_REF(eServiceFactoryMP3);

public:
	eServiceFactoryMP3();
	virtual ~eServiceFactoryMP3();
	enum { id = 0x1001 };

	// iServiceHandler
	RESULT play(const eServiceReference &, ePtr<iPlayableService> &ptr);
	RESULT record(const eServiceReference &, ePtr<iRecordableService> &ptr);
	RESULT list(const eServiceReference &, ePtr<iListableService> &ptr);
	RESULT info(const eServiceReference &, ePtr<iStaticServiceInformation> &ptr);
	RESULT offlineOperations(const eServiceReference &, ePtr<iServiceOfflineOperations> &ptr);

private:
	ePtr<eStaticServiceMP3Info> m_service_info;
};

class eStaticServiceMP3Info: public iStaticServiceInformation
{
	DECLARE_REF(eStaticServiceMP3Info);
	friend class eServiceFactoryMP3;
	eStaticServiceMP3Info();

public:
	RESULT getName(const eServiceReference &ref, std::string &name);
	int getLength(const eServiceReference &ref);
	int getInfo(const eServiceReference &ref, int w);
	int isPlayable(const eServiceReference &ref, const eServiceReference &ignore, bool simulate) { return 1; }
	long long getFileSize(const eServiceReference &ref);
	RESULT getEvent(const eServiceReference &ref, ePtr<eServiceEvent> &ptr, time_t start_time);
};

class eStreamBufferInfo: public iStreamBufferInfo
{
	DECLARE_REF(eStreamBufferInfo);
	int bufferPercentage;
	int inputRate;
	int outputRate;
	int bufferSpace;
	int bufferSize;

public:
	eStreamBufferInfo(int percentage, int inputrate, int outputrate, int space, int size);
	int getBufferPercentage() const;
	int getAverageInputRate() const;
	int getAverageOutputRate() const;
	int getBufferSpace() const;
	int getBufferSize() const;
};

typedef enum { atUnknown, atMPEG, atMP3, atAC3, atDTS, atAAC, atPCM, atOGG, atFLAC, atWMA } audiotype_t;
typedef enum { stUnknown, stPlainText, stSSA, stASS, stSRT, stVOB, stPGS } subtype_t;
typedef enum { ctNone, ctMPEGTS, ctMPEGPS, ctMKV, ctAVI, ctMP4, ctVCD, ctCDA, ctASF, ctOGG } containertype_t;

class eServiceMP3: public iPlayableService, public iPauseableService,
	public iServiceInformation, public iSeekableService, public iAudioTrackSelection, public iAudioChannelSelection,
	public iSubtitleOutput, public iStreamedService, public iAudioDelay, public Object, public iCueSheet
{
	DECLARE_REF(eServiceMP3);

public:
	virtual ~eServiceMP3();
	static eServiceMP3 *getInstance();
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
		audiotype_t type;
		std::string language_code; /* iso-639, if available. */
		std::string codec; /* clear text codec description */
		audioStream()
			:type(atUnknown)
		{
		}
	};
	struct subtitleStream
	{
		subtype_t type;
		std::string language_code; /* iso-639, if available. */
		int id;
		subtitleStream()
		{
		}
	};
	struct sourceStream
	{
		audiotype_t audiotype;
		containertype_t containertype;
		bool is_video;
		bool is_streaming;
		sourceStream()
			:audiotype(atUnknown), containertype(ctNone), is_video(false), is_streaming(false)
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
			:bufferPercent(0), avgInRate(0), avgOutRate(0), bufferingLeft(-1)
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
	static eServiceMP3 *instance;

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
	std::vector<M3U8StreamInfo> m_stream_vec;
	iSubtitleUser *m_subtitle_widget;
	friend class eServiceFactoryMP3;
	eServiceReference m_ref;
	int m_buffer_size;
	// cuesheet load check
	bool m_cuesheet_loaded;
	bufferInfo m_bufferInfo;
	eServiceMP3(eServiceReference ref);
	Signal2<void, iPlayableService*, int> m_event;
	enum
	{
		stIdle, stRunning, stStopped,
	};

	int m_state;
	Context_t * player;
	eFixedMessagePump<int> m_pump;
	void gotThreadMessage(const int &);

	struct subtitle_page_t
	{
		uint32_t start_ms;
		uint32_t end_ms;
		std::string text;
		subtitle_page_t(uint32_t start_ms_in, uint32_t end_ms_in, const std::string& text_in)
			: start_ms(start_ms_in), end_ms(end_ms_in), text(text_in)
		{
		}
	};

	typedef std::map<uint32_t, subtitle_page_t> subtitle_pages_map_t;
	subtitle_pages_map_t m_subtitle_pages;
	sourceStream m_sourceinfo;
	gint m_aspect, m_width, m_height, m_framerate, m_progressive;
};

#endif
