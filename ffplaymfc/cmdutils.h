#ifndef FFMPEG_CMDUTILS_H
#define FFMPEG_CMDUTILS_H

#include <stdint.h>

#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

#ifdef __MINGW32__
#undef main /* We don't want SDL to override our main() */
#endif

/**
 * program name, defined by the program for show_version().
 */
extern const char program_name[];

/**
 * this year, defined by the program for show_banner()
 */
extern const int this_year;

extern AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
extern AVFormatContext *avformat_opts;
extern struct SwsContext *sws_opts;
extern struct SwrContext *swr_opts;
extern AVDictionary *format_opts, *codec_opts;

/**
 * Initialize the cmdutils option system, in particular
 * allocate the *_opts contexts.
 */
void init_opts(void);
/**
 * Uninitialize the cmdutils option system, in particular
 * free the *_opts contexts and their contents.
 */
void uninit_opts(void);

/**
 * Trivial log callback.
 * Only suitable for opt_help and similar since it lacks prefix handling.
 */
void log_callback_help(void* ptr, int level, const char* fmt, va_list vl);

/**
 * Fallback for options that are not explicitly handled, these will be
 * parsed through AVOptions.
 */
int opt_default(void *optctx, const char *opt, const char *arg);

/**
 * Set the libav* libraries log level.
 */
int opt_loglevel(void *optctx, const char *opt, const char *arg);

int opt_report(const char *opt);

int opt_max_alloc(void *optctx, const char *opt, const char *arg);

int opt_cpuflags(void *optctx, const char *opt, const char *arg);

int opt_codec_debug(void *optctx, const char *opt, const char *arg);

/**
 * Limit the execution time.
 */
int opt_timelimit(void *optctx, const char *opt, const char *arg);

/**
 * Parse a string and return its corresponding value as a double.
 * Exit from the application if the string cannot be correctly
 * parsed or the corresponding value is invalid.
 *
 * @param context the context of the value to be set (e.g. the
 * corresponding command line option name)
 * @param numstr the string to be parsed
 * @param type the type (OPT_INT64 or OPT_FLOAT) as which the
 * string should be parsed
 * @param min the minimum valid accepted value
 * @param max the maximum valid accepted value
 */
double parse_number_or_die(const char *context, const char *numstr, int type,
                           double min, double max);

/**
 * Parse a string specifying a time and return its corresponding
 * value as a number of microseconds. Exit from the application if
 * the string cannot be correctly parsed.
 *
 * @param context the context of the value to be set (e.g. the
 * corresponding command line option name)
 * @param timestr the string to be parsed
 * @param is_duration a flag which tells how to interpret timestr, if
 * not zero timestr is interpreted as a duration, otherwise as a
 * date
 *
 * @see parse_date()
 */
int64_t parse_time_or_die(const char *context, const char *timestr,
                          int is_duration);

typedef struct SpecifierOpt {
    char *specifier;    /**< stream/chapter/program/... specifier */
    union {
        uint8_t *str;
        int        i;
        int64_t  i64;
        float      f;
        double   dbl;
    } u;
} SpecifierOpt;

typedef struct OptionDef {
    const char *name;
    int flags;
#define HAS_ARG    0x0001
#define OPT_BOOL   0x0002
#define OPT_EXPERT 0x0004
#define OPT_STRING 0x0008
#define OPT_VIDEO  0x0010
#define OPT_AUDIO  0x0020
#define OPT_INT    0x0080
#define OPT_FLOAT  0x0100
#define OPT_SUBTITLE 0x0200
#define OPT_INT64  0x0400
#define OPT_EXIT   0x0800
#define OPT_DATA   0x1000
#define OPT_PERFILE  0x2000     /* the option is per-file (currently ffmpeg-only).
                                   implied by OPT_OFFSET or OPT_SPEC */
#define OPT_OFFSET 0x4000       /* option is specified as an offset in a passed optctx */
#define OPT_SPEC   0x8000       /* option is to be stored in an array of SpecifierOpt.
                                   Implies OPT_OFFSET. Next element after the offset is
                                   an int containing element count in the array. */
#define OPT_TIME  0x10000
#define OPT_DOUBLE 0x20000
     union {
        void *dst_ptr;
        int (*func_arg)(void *, const char *, const char *);
        size_t off;
    } u;
    const char *help;
    const char *argname;
} OptionDef;

/**
 * Print help for all options matching specified flags.
 *
 * @param options a list of options
 * @param msg title of this group. Only printed if at least one option matches.
 * @param req_flags print only options which have all those flags set.
 * @param rej_flags don't print options which have any of those flags set.
 * @param alt_flags print only options that have at least one of those flags set
 */
void show_help_options(const OptionDef *options, const char *msg, int req_flags,
                       int rej_flags, int alt_flags);

/**
 * Show help for all options with given flags in class and all its
 * children.
 */
void show_help_children(const AVClass *class1, int flags);

/**
 * Per-avtool specific help handler. Implemented in each
 * avtool, called by show_help().
 */
void show_help_default(const char *opt, const char *arg);

/**
 * Generic -h handler common to all avtools.
 */
int show_help(void *optctx, const char *opt, const char *arg);

/**
 * Parse one given option.
 *
 * @return on success 1 if arg was consumed, 0 otherwise; negative number on error
 */
int parse_option(void *optctx, const char *opt, const char *arg,
                 const OptionDef *options);

/**
 * Find the '-loglevel' option in the command line args and apply it.
 */
void parse_loglevel(int argc, char **argv, const OptionDef *options);

/**
 * Return index of option opt in argv or 0 if not found.
 */
int locate_option(int argc, char **argv, const OptionDef *options,
                  const char *optname);

/**
 * Check if the given stream matches a stream specifier.
 *
 * @param s  Corresponding format context.
 * @param st Stream from s to be checked.
 * @param spec A stream specifier of the [v|a|s|d]:[\<stream index\>] form.
 *
 * @return 1 if the stream matches, 0 if it doesn't, <0 on error
 */
int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec);

/**
 * Filter out options for given codec.
 *
 * Create a new options dictionary containing only the options from
 * opts which apply to the codec with ID codec_id.
 *
 * @param s Corresponding format context.
 * @param st A stream from s for which the options should be filtered.
 * @param codec The particular codec for which the options should be filtered.
 *              If null, the default one is looked up according to the codec id.
 * @return a pointer to the created dictionary
 */
AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                AVFormatContext *s, AVStream *st, AVCodec *codec);

/**
 * Setup AVCodecContext options for avformat_find_stream_info().
 *
 * Create an array of dictionaries, one dictionary for each stream
 * contained in s.
 * Each dictionary will contain the options from codec_opts which can
 * be applied to the corresponding stream codec context.
 *
 * @return pointer to the created array of dictionaries, NULL if it
 * cannot be created
 */
AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,
                                           AVDictionary *codec_opts);

/**
 * Print an error message to stderr, indicating filename and a human
 * readable description of the error code err.
 *
 * If strerror_r() is not available the use of this function in a
 * multithreaded application may be unsafe.
 *
 * @see av_strerror()
 */
void print_error(const char *filename, int err);

/**
 * Print the program banner to stderr. The banner contents depend on the
 * current version of the repository and of the libav* libraries used by
 * the program.
 */
void show_banner(int argc, char **argv, const OptionDef *options);

/**
 * Print the version of the program to stdout. The version message
 * depends on the current versions of the repository and of the libav*
 * libraries.
 * This option processing function does not utilize the arguments.
 */
int show_version(void *optctx, const char *opt, const char *arg);

/**
 * Print the license of the program to stdout. The license depends on
 * the license of the libraries compiled into the program.
 * This option processing function does not utilize the arguments.
 */
int show_license(void *optctx, const char *opt, const char *arg);

/**
 * Print a listing containing all the formats supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_formats(void *optctx, const char *opt, const char *arg);

/**
 * Print a listing containing all the codecs supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_codecs(void *optctx, const char *opt, const char *arg);

/**
 * Print a listing containing all the decoders supported by the
 * program.
 */
int show_decoders(void *optctx, const char *opt, const char *arg);

/**
 * Print a listing containing all the encoders supported by the
 * program.
 */
int show_encoders(void *optctx, const char *opt, const char *arg);

/**
 * Print a listing containing all the filters supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_filters(void *optctx, const char *opt, const char *arg);

/**
 * Print a listing containing all the bit stream filters supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_bsfs(void *optctx, const char *opt, const char *arg);

/**
 * Print a listing containing all the protocols supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_protocols(void *optctx, const char *opt, const char *arg);

/**
 * Print a listing containing all the pixel formats supported by the
 * program.
 * This option processing function does not utilize the arguments.
 */
int show_pix_fmts(void *optctx, const char *opt, const char *arg);

/**
 * Print a listing containing all the standard channel layouts supported by
 * the program.
 * This option processing function does not utilize the arguments.
 */
int show_layouts(void *optctx, const char *opt, const char *arg);

/**
 * Print a listing containing all the sample formats supported by the
 * program.
 */
int show_sample_fmts(void *optctx, const char *opt, const char *arg);

/**
 * Return a positive value if a line read from standard input
 * starts with [yY], otherwise return 0.
 */
int read_yesno(void);

/**
 * Read the file with name filename, and put its content in a newly
 * allocated 0-terminated buffer.
 *
 * @param bufptr location where pointer to buffer is returned
 * @param size   location where size of buffer is returned
 * @return 0 in case of success, a negative value corresponding to an
 * AVERROR error code in case of failure.
 */
int cmdutils_read_file(const char *filename, char **bufptr, size_t *size);

/**
 * Get a file corresponding to a preset file.
 *
 * If is_path is non-zero, look for the file in the path preset_name.
 * Otherwise search for a file named arg.ffpreset in the directories
 * $FFMPEG_DATADIR (if set), $HOME/.ffmpeg, and in the datadir defined
 * at configuration time or in a "ffpresets" folder along the executable
 * on win32, in that order. If no such file is found and
 * codec_name is defined, then search for a file named
 * codec_name-preset_name.avpreset in the above-mentioned directories.
 *
 * @param filename buffer where the name of the found filename is written
 * @param filename_size size in bytes of the filename buffer
 * @param preset_name name of the preset to search
 * @param is_path tell if preset_name is a filename path
 * @param codec_name name of the codec for which to look for the
 * preset, may be NULL
 */
FILE *get_preset_file(char *filename, size_t filename_size,
                      const char *preset_name, int is_path, const char *codec_name);

/**
 * Realloc array to hold new_size elements of elem_size.
 * Calls exit() on failure.
 *
 * @param elem_size size in bytes of each element
 * @param size new element count will be written here
 * @return reallocated array
 */
void *grow_array(void *array, int elem_size, int *size, int new_size);

typedef struct FrameBuffer {
    uint8_t *base[4];
    uint8_t *data[4];
    int  linesize[4];

    int h, w;
    enum AVPixelFormat pix_fmt;

    int refcount;
    struct FrameBuffer **pool;  ///< head of the buffer pool
    struct FrameBuffer *next;
} FrameBuffer;

/**
 * Get a frame from the pool. This is intended to be used as a callback for
 * AVCodecContext.get_buffer.
 *
 * @param s codec context. s->opaque must be a pointer to the head of the
 *          buffer pool.
 * @param frame frame->opaque will be set to point to the FrameBuffer
 *              containing the frame data.
 */
int codec_get_buffer(AVCodecContext *s, AVFrame *frame);

/**
 * A callback to be used for AVCodecContext.release_buffer along with
 * codec_get_buffer().
 */
void codec_release_buffer(AVCodecContext *s, AVFrame *frame);

/**
 * A callback to be used for AVFilterBuffer.free.
 * @param fb buffer to free. fb->priv must be a pointer to the FrameBuffer
 *           containing the buffer data.
 */
void filter_release_buffer(AVFilterBuffer *fb);

/**
 * Free all the buffers in the pool. This must be called after all the
 * buffers have been released.
 */
void free_buffer_pool(FrameBuffer **pool);

#define GET_PIX_FMT_NAME(pix_fmt)\
    const char *name = av_get_pix_fmt_name(pix_fmt);

#define GET_SAMPLE_FMT_NAME(sample_fmt)\
    const char *name = av_get_sample_fmt_name(sample_fmt)

#define GET_SAMPLE_RATE_NAME(rate)\
    char name[16];\
    snprintf(name, sizeof(name), "%d", rate);

#define GET_CH_LAYOUT_NAME(ch_layout)\
    char name[16];\
    snprintf(name, sizeof(name), "0x%"PRIx64, ch_layout);

#define GET_CH_LAYOUT_DESC(ch_layout)\
    char name[128];\
    av_get_channel_layout_string(name, sizeof(name), 0, ch_layout);

#endif /* CMDUTILS_H */
