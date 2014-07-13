/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "../util/bmem.h"
#include "audio-resampler.h"
#include "audio-io.h"
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>

struct audio_resampler {
        AVAudioResampleContext   *context;
	bool                opened;
	uint8_t             *output_buffer[MAX_AV_PLANES];
        uint32_t            output_planes;
        uint32_t            input_freq;
        uint32_t            output_freq;
        enum AVSampleFormat output_format;
        int                 output_size;
        uint32_t            output_ch;
};

static inline enum AVSampleFormat convert_audio_format(enum audio_format format)
{
	switch (format) {
	case AUDIO_FORMAT_UNKNOWN:      return AV_SAMPLE_FMT_S16;
	case AUDIO_FORMAT_U8BIT:        return AV_SAMPLE_FMT_U8;
	case AUDIO_FORMAT_16BIT:        return AV_SAMPLE_FMT_S16;
	case AUDIO_FORMAT_32BIT:        return AV_SAMPLE_FMT_S32;
	case AUDIO_FORMAT_FLOAT:        return AV_SAMPLE_FMT_FLT;
	case AUDIO_FORMAT_U8BIT_PLANAR: return AV_SAMPLE_FMT_U8P;
	case AUDIO_FORMAT_16BIT_PLANAR: return AV_SAMPLE_FMT_S16P;
	case AUDIO_FORMAT_32BIT_PLANAR: return AV_SAMPLE_FMT_S32P;
	case AUDIO_FORMAT_FLOAT_PLANAR: return AV_SAMPLE_FMT_FLTP;
	}

	/* shouldn't get here */
	return AV_SAMPLE_FMT_S16;
}

static inline uint64_t convert_speaker_layout(enum speaker_layout layout)
{
	switch (layout) {
	case SPEAKERS_UNKNOWN:          return 0;
	case SPEAKERS_MONO:             return AV_CH_LAYOUT_MONO;
	case SPEAKERS_STEREO:           return AV_CH_LAYOUT_STEREO;
	case SPEAKERS_2POINT1:          return AV_CH_LAYOUT_2_1;
	case SPEAKERS_QUAD:             return AV_CH_LAYOUT_QUAD;
	case SPEAKERS_4POINT1:          return AV_CH_LAYOUT_4POINT1;
	case SPEAKERS_5POINT1:          return AV_CH_LAYOUT_5POINT1;
	case SPEAKERS_5POINT1_SURROUND: return AV_CH_LAYOUT_5POINT1_BACK;
	case SPEAKERS_7POINT1:          return AV_CH_LAYOUT_7POINT1;
	case SPEAKERS_7POINT1_SURROUND: return AV_CH_LAYOUT_7POINT1_WIDE_BACK;
	case SPEAKERS_SURROUND:         return AV_CH_LAYOUT_SURROUND;
	}

	/* shouldn't get here */
	return 0;
}

static char *const get_error_text(const int error)
{
       static char error_buffer[255];
       av_strerror(error, error_buffer, sizeof(error_buffer));
       return error_buffer;
}

audio_resampler_t audio_resampler_create(const struct resample_info *dst,
		const struct resample_info *src)
{
	struct audio_resampler *rs = bzalloc(sizeof(struct audio_resampler));
        AVAudioResampleContext *avr = avresample_alloc_context();
        rs->context = avr;
	int errcode;

        errcode = av_opt_set_int(avr, "in_channel_layout",  av_get_channel_layout_nb_channels(src->speakers), 0);
        blog(LOG_ERROR, "code %d", errcode);
        errcode = av_opt_set_int(avr, "out_channel_layout", av_get_channel_layout_nb_channels(dst->speakers), 0);
        blog(LOG_ERROR, "code %d", errcode);
        errcode = av_opt_set_int(avr, "in_sample_rate",     src->samples_per_sec, 0);
        blog(LOG_ERROR, "code %d", errcode);
        errcode = av_opt_set_int(avr, "out_sample_rate",    dst->samples_per_sec, 0);
        blog(LOG_ERROR, "code %d", errcode);
        errcode = av_opt_set_int(avr, "in_sample_fmt",      convert_audio_format(src->format), 0);
        blog(LOG_ERROR, "code %d", errcode);
        errcode = av_opt_set_int(avr, "out_sample_fmt",     convert_audio_format(dst->format), 0);
        blog(LOG_ERROR, "code %d", errcode);

        rs->input_freq    = src->samples_per_sec;
        rs->output_freq   = dst->samples_per_sec;
        rs->output_size   = 0;
        rs->output_format = convert_audio_format(dst->format);
        rs->output_ch     = get_audio_channels(dst->speakers);
        
	errcode = avresample_open(avr);
	if (errcode != 0) {
		blog(LOG_ERROR, "avresample_open failed: error %s",
                     get_error_text(errcode));
		audio_resampler_destroy(rs);
                avresample_free(&avr);
                avr = NULL;
		return NULL;
	}

	return rs;
}

void audio_resampler_destroy(audio_resampler_t rs)
{
	if (rs) {
		if (rs->context)
			avresample_close(&rs->context);
		if (rs->output_buffer)
			av_freep(&rs->output_buffer[0]);

		bfree(rs);
	}
}

bool audio_resampler_resample(audio_resampler_t rs,
		 uint8_t *output[], uint32_t *out_frames, uint64_t *ts_offset,
		 const uint8_t *const input[], uint32_t in_frames)
{
	if (!rs) return false;
        
        AVAudioResampleContext *context = rs->context;
	int ret;
        int out_linesize;
	int estimated = avresample_available(rs->context) + (int)av_rescale_rnd(avresample_get_delay(rs->context) + (int64_t)in_frames, (int64_t)rs->output_freq, (int64_t)rs->input_freq, AV_ROUND_UP);
	*ts_offset = (uint64_t)avresample_get_delay(context);
        //*ts_offset = 0;
	/* resize the buffer if bigger */
	if (estimated > rs->output_size) {
		if (rs->output_buffer[0])
			av_freep(&rs->output_buffer[0]);

		av_samples_alloc(rs->output_buffer, out_linesize, rs->output_ch,
				estimated, rs->output_format, 0);

		rs->output_size = estimated;
	}

	ret = avresample_convert(context, rs->output_buffer, out_linesize, rs->output_size, (const uint8_t**)input, out_linesize, in_frames);

	if (ret < 0) {
		blog(LOG_ERROR, "swr_convert failed: %d", ret);
		return false;
	}

	for (uint32_t i = 0; i < rs->output_planes; i++)
		output[i] = rs->output_buffer[i];

	*out_frames = (uint32_t)ret;
	return true;
}
