#include "ffplaymfc.h"
#include "ffplaymfcDlg.h"
#include "afxdialogex.h"
#include "config.h"
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"
#include <assert.h>

extern "C"
{
#include "libavutil/avstring.h"
#include "libavutil/colorspace.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"
}

#if CONFIG_AVFILTER
# include "libavfilter/avcodec.h"
# include "libavfilter/avfilter.h"
# include "libavfilter/avfiltergraph.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#endif

#include "cmdutils.h"

enum ShowMode {
	SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
};

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 5

/* SDL audio buffer size, in samples. Should be small to have precise
A/V sync as SDL does not have hardware buffer fullness info. */
#define SDL_AUDIO_BUFFER_SIZE 1024

/* no AV sync correction is done if below the AV sync threshold */
#define AV_SYNC_THRESHOLD 0.01
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

static int sws_flags = SWS_BICUBIC;

//读取输入文件协议的时候使用；来自ffmpeg源码
typedef struct URLContext {
	const AVClass* av_class; ///< information for av_log(). Set by url_open().
	struct URLProtocol* prot;
	int flags;
	int is_streamed;  /**< true if streamed (no seek possible), default = false */
	int max_packet_size;  /**< if non zero, the stream is packetized with this max packet size */
	void* priv_data;
	char* filename; /**< specified URL */
	int is_connected;
	AVIOInterruptCB interrupt_callback;
} URLContext;
typedef struct URLProtocol {
	const char* name;
	int (*url_open)(URLContext* h, const char* url, int flags);
	int (*url_read)(URLContext* h, unsigned char* buf, int size);
	int (*url_write)(URLContext* h, const unsigned char* buf, int size);
	int64_t(*url_seek)(URLContext* h, int64_t pos, int whence);
	int (*url_close)(URLContext* h);
	struct URLProtocol* next;
	int (*url_read_pause)(URLContext* h, int pause);
	int64_t(*url_read_seek)(URLContext* h, int stream_index,
		int64_t timestamp, int flags);
	int (*url_get_file_handle)(URLContext* h);
	int priv_data_size;
	const AVClass* priv_data_class;
	int flags;
	int (*url_check)(URLContext* h, int mask);
} URLProtocol;
//---------------------

typedef struct PacketQueue {
	AVPacketList* first_pkt, * last_pkt;
	int nb_packets;
	int size;
	int abort_request;
	SDL_mutex* mutex;
	SDL_cond* cond;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 4
#define SUBPICTURE_QUEUE_SIZE 4

typedef struct VideoPicture {
	double pts;                                  ///< presentation time stamp for this picture
	int64_t pos;                                 ///< byte position in file
	int skip;
	SDL_Overlay* bmp;
	int width, height; /* source height & width */
	AVRational sample_aspect_ratio;
	int allocated;
	int reallocate;

#if CONFIG_AVFILTER
	AVFilterBufferRef* picref;
#endif
} VideoPicture;

typedef struct SubPicture {
	double pts; /* presentation time stamp for this picture */
	AVSubtitle sub;
} SubPicture;

typedef struct AudioParams {
	int freq;
	int channels;
	int channel_layout;
	enum AVSampleFormat fmt;
} AudioParams;

enum {
	AV_SYNC_AUDIO_MASTER, /* default choice */
	AV_SYNC_VIDEO_MASTER,
	AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};
//视频显示方式
enum V_Show_Mode {
	SHOW_MODE_YUV = 0, SHOW_MODE_Y, SHOW_MODE_U, SHOW_MODE_V
};

typedef struct VideoState {
	SDL_Thread* read_tid;
	SDL_Thread* video_tid;
	SDL_Thread* refresh_tid;
	AVInputFormat* iformat;
	int no_background;
	int abort_request;
	int force_refresh;
	int paused;
	int last_paused;
	int que_attachments_req;
	int seek_req;
	int seek_flags;
	int64_t seek_pos;
	int64_t seek_rel;
	int read_pause_return;
	AVFormatContext* ic;
	int audio_stream;
	int av_sync_type;
	double external_clock; /* external clock base */
	int64_t external_clock_time;
	double audio_clock;
	double audio_diff_cum; /* used for AV difference average computation */
	double audio_diff_avg_coef;
	double audio_diff_threshold;
	int audio_diff_avg_count;
	AVStream* audio_st;
	PacketQueue audioq;
	int audio_hw_buf_size;
	DECLARE_ALIGNED(16, uint8_t, audio_buf2)[AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];
	uint8_t silence_buf[SDL_AUDIO_BUFFER_SIZE];
	uint8_t* audio_buf;
	uint8_t* audio_buf1;
	unsigned int audio_buf_size; /* in bytes */
	int audio_buf_index; /* in bytes */
	int audio_write_buf_size;
	AVPacket audio_pkt_temp;
	AVPacket audio_pkt;
	struct AudioParams audio_src;
	struct AudioParams audio_tgt;
	struct SwrContext* swr_ctx;
	double audio_current_pts;
	double audio_current_pts_drift;
	int frame_drops_early;
	int frame_drops_late;
	AVFrame* frame;

	enum ShowMode {
		SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
	} show_mode;
	int16_t sample_array[SAMPLE_ARRAY_SIZE];
	int sample_array_index;
	int last_i_start;
	RDFTContext* rdft;
	int rdft_bits;
	FFTSample* rdft_data;
	int xpos;

	SDL_Thread* subtitle_tid;
	int subtitle_stream;
	int subtitle_stream_changed;
	AVStream* subtitle_st;
	PacketQueue subtitleq;
	SubPicture subpq[SUBPICTURE_QUEUE_SIZE];
	int subpq_size, subpq_rindex, subpq_windex;
	SDL_mutex* subpq_mutex;
	SDL_cond* subpq_cond;

	double frame_timer;
	double frame_last_pts;
	double frame_last_duration;
	double frame_last_dropped_pts;
	double frame_last_returned_time;
	double frame_last_filter_delay;
	int64_t frame_last_dropped_pos;
	double video_clock;                          ///< pts of last decoded frame / predicted pts of next decoded frame
	int video_stream;
	AVStream* video_st;
	PacketQueue videoq;
	double video_current_pts;                    ///< current displayed pts (different from video_clock if frame fifos are used)
	double video_current_pts_drift;              ///< video_current_pts - time (av_gettime) at which we updated video_current_pts - used to have running video pts
	int64_t video_current_pos;                   ///< current displayed file pos
	VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
	int pictq_size, pictq_rindex, pictq_windex;
	SDL_mutex* pictq_mutex;
	SDL_cond* pictq_cond;
#if !CONFIG_AVFILTER
	struct SwsContext* img_convert_ctx;
#endif

	char filename[1024];
	int width, height, xleft, ytop;
	int step;

#if CONFIG_AVFILTER
	AVFilterContext* in_video_filter;           ///< the first filter in the video chain
	AVFilterContext* out_video_filter;          ///< the last filter in the video chain
	int use_dr1;
	FrameBuffer* buffer_pool;
#endif

	int refresh;
	int last_video_stream, last_audio_stream, last_subtitle_stream;

	SDL_cond* continue_read_thread;
	//视频显示方式------------
	enum V_Show_Mode v_show_mode;
} VideoState;

static bool m_exit = false;//专门设置的标记，在程序将要退出的时候会置1
static int is_full_screen;
static int64_t audio_callback_time;
static AVPacket flush_pkt;
static SDL_Surface* screen;

static AVInputFormat* file_iformat;
static const char* input_filename;
static const char* window_title;
static int fs_screen_width;
static int fs_screen_height;
static int screen_width = 0;
static int screen_height = 0;
static int audio_disable;
static int video_disable;
static int wanted_stream[AVMEDIA_TYPE_NB] = { -1,-1,0,-1,0 };
static int seek_by_bytes = -1;
static int display_disable;
static int show_status = 1;
static int av_sync_type = AV_SYNC_AUDIO_MASTER;
static int64_t start_time = AV_NOPTS_VALUE;
static int64_t duration = AV_NOPTS_VALUE;
static int workaround_bugs = 1;
static int fast = 0;
static int genpts = 0;
static int lowres = 0;
static int idct = FF_IDCT_AUTO;
static enum AVDiscard skip_frame = AVDISCARD_DEFAULT;
static enum AVDiscard skip_idct = AVDISCARD_DEFAULT;
static enum AVDiscard skip_loop_filter = AVDISCARD_DEFAULT;
static int error_concealment = 3;
static int decoder_reorder_pts = -1;
static int autoexit;
static int exit_on_keydown;
static int exit_on_mousedown;
static int loop = 1;
static int framedrop = -1;
static int infinite_buffer = -1;
static enum ShowMode show_mode = SHOW_MODE_NONE;
static const char* audio_codec_name;
static const char* subtitle_codec_name;
static const char* video_codec_name;
static int rdftspeed = 20;

#define FF_ALLOC_EVENT   (SDL_USEREVENT)
#define FF_REFRESH_EVENT (SDL_USEREVENT + 1)
#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)
#define FFMFC_SEEK_BAR_EVENT    (SDL_USEREVENT + 4)//自定义一个事件，用于调整播放进度
#define FFMFC_STRETCH_EVENT (SDL_USEREVENT + 5) //是否拉伸-------------------------


#define MAX_FRAME_NUM 10000 //最多存储的帧信息
#define MAX_PACKET_NUM 10000//最多存储的Packet信息
#define MAX_URL_LENGTH 500//URL长度

void ffmfc_play_pause(); //发送“暂停”命令
void ffmfc_seek_step();//发送“逐帧”命令
void ffmfc_play_fullcreen();//发送“全屏”命令
void ffmfc_seek(int time);//发送“前进/后退”命令
void ffmfc_aspectratio(int num,int den);//发送“宽高比”命令
void ffmfc_size(int percentage);//发送“大小”命令
void ffmfc_audio_display(int mode);//发送“窗口画面内容”命令
void ffmfc_quit();//发送“退出”命令
int  ffmfc_play(LPVOID lpParam);//解码主函数
int  ffmfc_reset_index();//复位
void ffmfc_seek_bar(int pos);//播放进度
void ffmfc_stretch(int stretch);//Stretch

