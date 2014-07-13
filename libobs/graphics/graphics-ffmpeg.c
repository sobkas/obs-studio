#include "graphics.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "../obs-ffmpeg-compat.h"

struct ffmpeg_image {
	const char         *file;
	AVFormatContext    *fmt_ctx;
	AVCodecContext     *decoder_ctx;
	AVCodec            *decoder;
	AVStream           *stream;
	int                stream_idx;

	int                cx, cy;
	enum AVPixelFormat format;
};

static bool ffmpeg_image_open_decoder_context(struct ffmpeg_image *info)
{
	int ret = av_find_best_stream(info->fmt_ctx, AVMEDIA_TYPE_VIDEO,
			-1, 1, NULL, 0);
	if (ret < 0) {
		return false;
	}

	info->stream_idx  = ret;
	info->stream      = info->fmt_ctx->streams[ret];
	info->decoder_ctx = info->stream->codec;
	info->decoder     = avcodec_find_decoder(info->decoder_ctx->codec_id);

	if (!info->decoder) {
		blog(LOG_WARNING, "Failed to find decoder for file '%s'",
				info->file);
		return false;
	}

	ret = avcodec_open2(info->decoder_ctx, info->decoder, NULL);
	if (ret < 0) {
		return false;
	}

	return true;
}

static void ffmpeg_image_free(struct ffmpeg_image *info)
{
	avcodec_close(info->decoder_ctx);
	avformat_close_input(&info->fmt_ctx);
}

static bool ffmpeg_image_init(struct ffmpeg_image *info, const char *file)
{
	int ret;

	memset(info, 0, sizeof(struct ffmpeg_image));
	info->file       = file;
	info->stream_idx = -1;

	ret = avformat_open_input(&info->fmt_ctx, file, NULL, NULL);
	if (ret < 0) {
		return false;
	}

	ret = avformat_find_stream_info(info->fmt_ctx, NULL);
	if (ret < 0) {
		goto fail;
	}

	if (!ffmpeg_image_open_decoder_context(info))
		goto fail;

	info->cx     = info->decoder_ctx->width;
	info->cy     = info->decoder_ctx->height;
	info->format = info->decoder_ctx->pix_fmt;
	return true;

fail:
	ffmpeg_image_free(info);
	return false;
}

static bool ffmpeg_image_reformat_frame(struct ffmpeg_image *info,
		AVFrame *frame, uint8_t *out, int linesize)
{
	struct SwsContext *sws_ctx = NULL;
	int               ret      = 0;

	if (info->format == AV_PIX_FMT_RGBA ||
	    info->format == AV_PIX_FMT_BGRA) {

		if (linesize != frame->linesize[0]) {
			int min_line = linesize < frame->linesize[0] ?
				linesize : frame->linesize[0];

			for (int y = 0; y < info->cy; y++)
				memcpy(out + y * linesize,
				       frame->data[0] + y * frame->linesize[0],
				       min_line);
		} else {
			memcpy(out, frame->data[0], linesize * info->cy);
		}

	} else {
		sws_ctx = sws_getContext(info->cx, info->cy, info->format,
				info->cx, info->cy, AV_PIX_FMT_BGRA,
				SWS_POINT, NULL, NULL, NULL);
		if (!sws_ctx) {
			blog(LOG_WARNING, "Failed to create scale context "
			                  "for '%s'", info->file);
			return false;
		}

		ret = sws_scale(sws_ctx, (const uint8_t *const*)frame->data,
				frame->linesize, 0, info->cy, &out, &linesize);
		sws_freeContext(sws_ctx);

		if (ret < 0) {
			return false;
		}

		info->format = AV_PIX_FMT_BGRA;
	}

	return true;
}

static bool ffmpeg_image_decode(struct ffmpeg_image *info, uint8_t *out,
		int linesize)
{
	AVPacket          packet    = {0};
	bool              success   = false;
	AVFrame           *frame    = av_frame_alloc();
	int               got_frame = 0;
	int               ret;

	if (!frame) {
		blog(LOG_WARNING, "Failed to create frame data for '%s'",
				info->file);
		return false;
	}

	ret = av_read_frame(info->fmt_ctx, &packet);
	if (ret < 0) {
		goto fail;
	}

	while (!got_frame) {
		ret = avcodec_decode_video2(info->decoder_ctx, frame,
				&got_frame, &packet);
		if (ret < 0) {
			goto fail;
		}
	}

	success = ffmpeg_image_reformat_frame(info, frame, out, linesize);

fail:
	av_free_packet(&packet);
	av_frame_free(&frame);
	return success;
}

void gs_init_image_deps(void)
{
	av_register_all();
}

void gs_free_image_deps(void)
{
}

static inline enum gs_color_format convert_format(enum AVPixelFormat format)
{
	switch ((int)format) {
	case AV_PIX_FMT_RGBA: return GS_RGBA;
	case AV_PIX_FMT_BGRA: return GS_BGRA;
	}

	return GS_BGRX;
}

texture_t gs_create_texture_from_file(const char *file)
{
	struct ffmpeg_image image;
	texture_t           tex = NULL;

	if (ffmpeg_image_init(&image, file)) {
		uint8_t *data = malloc(image.cx * image.cy * 4);
		if (ffmpeg_image_decode(&image, data, image.cx * 4)) {
			tex = gs_create_texture(image.cx, image.cy,
					convert_format(image.format),
					1, (const uint8_t**)&data, 0);
		}

		ffmpeg_image_free(&image);
		free(data);
	}
	return tex;
}
