#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libavutil/pixdesc.h>

#include <stdio.h>

static enum AVPixelFormat hw_pix_fmt;
static AVBufferRef *hw_device_ctx = NULL;

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) if (*p == hw_pix_fmt) return *p;

    return AV_PIX_FMT_NONE;
}


int main(int argc, const char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./decode_ffmpeg <url>\n");
        return 1;
    }
    avdevice_register_all();
    AVFormatContext *in_fmt_ctx = avformat_alloc_context();

    int ret = avformat_open_input(&in_fmt_ctx, argv[1], NULL, NULL);

    AVCodec *vcodec = NULL;
    AVCodec *acodec = NULL;

    int vstream = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &vcodec, 0);
    int astream = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &acodec, 0);

    printf("vcodec: %s\n",  vcodec->long_name);
    printf("acodec: %s\n",  acodec->long_name);

    AVCodecContext *vcodec_ctx = avcodec_alloc_context3(vcodec);
    AVCodecContext *acodec_ctx = avcodec_alloc_context3(acodec);

    ret = avcodec_parameters_to_context(vcodec_ctx, in_fmt_ctx->streams[vstream]->codecpar);
    ret = avcodec_parameters_to_context(acodec_ctx, in_fmt_ctx->streams[astream]->codecpar);

    ret = avcodec_open2(vcodec_ctx, vcodec, NULL);
    ret = avcodec_open2(acodec_ctx, acodec, NULL);

    AVPacket *packet = av_packet_alloc();
    AVFrame  *frame  = av_frame_alloc();
    AVFrame  *sw_frame  = av_frame_alloc();

    enum AVHWDeviceType type = AV_HWDEVICE_TYPE_VIDEOTOOLBOX;

    for (int i = 0; ;i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(vcodec, i);
        if (!config) return 1;

        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }

    vcodec_ctx->get_format = get_hw_format;
    av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0);
    vcodec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    int64_t begin = av_gettime_relative();

    do {
        ret = av_read_frame(in_fmt_ctx, packet);
        if (ret == AVERROR_EOF) break;


        int64_t now = av_gettime_relative() - begin;

        if (packet->stream_index == vstream) {
            ret = avcodec_send_packet(vcodec_ctx, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

            while (ret >= 0) {
                ret = avcodec_receive_frame(vcodec_ctx, frame);
                if (ret == AVERROR_EOF) break;

                int64_t pts = av_rescale_q(frame->pts, in_fmt_ctx->streams[vstream]->time_base, AV_TIME_BASE_Q);
                if (pts > now) av_usleep(pts - now);

                if (frame->format == hw_pix_fmt) {
                    ret = av_hwframe_transfer_data(sw_frame, frame, 0);
                    printf("frame: %s - sw_frame: %s\n", av_get_pix_fmt_name(frame->format), av_get_pix_fmt_name(sw_frame->format));
                }
            }
        }

        if (packet->stream_index == astream) {
            ret = avcodec_send_packet(acodec_ctx, packet);

            while (ret >= 0) {
                ret = avcodec_receive_frame(acodec_ctx, frame);
                if (ret == AVERROR_EOF) break;

                int64_t pts = av_rescale_q(frame->pts, in_fmt_ctx->streams[astream]->time_base, AV_TIME_BASE_Q);
                if (pts > now) av_usleep(pts - now);
            }
        }

        av_packet_unref(packet);

    } while (1);

    avformat_close_input(&in_fmt_ctx);
    avcodec_free_context(&vcodec_ctx);
    avcodec_free_context(&acodec_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);

    return 0;
}
