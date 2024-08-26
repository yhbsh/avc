#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/time.h>

#include <stdio.h>

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./decode_ffmpeg <url>\n");
        return 1;
    }

    int ret;

    AVFormatContext *in_fmt_ctx = avformat_alloc_context();

    if ((ret = avformat_open_input(&in_fmt_ctx, argv[1], NULL, NULL)) < 0) {
        fprintf(stderr, "[ERROR]: cannot open input: %s\n", av_err2str(ret));
        return 1;
    }

    const AVCodec *vcodec = NULL;
    const AVCodec *acodec = NULL;

    int vstream = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &vcodec, 0);
    if (vstream < 0) {
        fprintf(stderr, "[ERROR]: cannot find video stream: %s\n", av_err2str(vstream));
        return 1;
    }
    int astream = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &acodec, 0);
    if (astream < 0) {
        fprintf(stderr, "[ERROR]: cannot find video stream: %s\n", av_err2str(vstream));
        return 1;
    }

    printf("vcodec: %s\n", vcodec->long_name);
    printf("acodec: %s\n", acodec->long_name);

    AVCodecContext *vcodec_ctx = avcodec_alloc_context3(vcodec);
    AVCodecContext *acodec_ctx = avcodec_alloc_context3(acodec);

    if ((ret = avcodec_parameters_to_context(vcodec_ctx, in_fmt_ctx->streams[vstream]->codecpar)) < 0) {
        fprintf(stderr, "[ERROR]: cannot copy video codec params to codec ctx: %s\n", av_err2str(vstream));
        return 1;
    }

    if ((ret = avcodec_parameters_to_context(acodec_ctx, in_fmt_ctx->streams[astream]->codecpar)) < 0) {
        fprintf(stderr, "[ERROR]: cannot copy audio codec params to codec ctx: %s\n", av_err2str(vstream));
        return 1;
    }

    if ((ret = avcodec_open2(vcodec_ctx, vcodec, NULL)) < 0) {
        fprintf(stderr, "[ERROR]: cannot open video codec: %s\n", av_err2str(vstream));
        return 1;
    }

    if ((ret = avcodec_open2(acodec_ctx, acodec, NULL)) < 0) {
        fprintf(stderr, "[ERROR]: cannot open audio codec: %s\n", av_err2str(vstream));
        return 1;
    }

    AVPacket *packet  = av_packet_alloc();
    AVFrame *frame    = av_frame_alloc();

    while (1) {
        ret = av_read_frame(in_fmt_ctx, packet);
        if (ret == AVERROR_EOF) break;

        if (packet->stream_index == vstream) {
            ret = avcodec_send_packet(vcodec_ctx, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

            while (ret >= 0) {
                ret = avcodec_receive_frame(vcodec_ctx, frame);
                if (ret == AVERROR_EOF) break;

                printf("Video Frame: %s\n", av_get_pix_fmt_name(frame->format));
            }
        }

        if (packet->stream_index == astream) {
            ret = avcodec_send_packet(acodec_ctx, packet);

            while (ret >= 0) {
                ret = avcodec_receive_frame(acodec_ctx, frame);
                if (ret == AVERROR_EOF) break;
                printf("Audio Frame: %s\n", av_get_sample_fmt_name(frame->format));
            }
        }

        av_packet_unref(packet);

    }

    avformat_close_input(&in_fmt_ctx);
    avcodec_free_context(&vcodec_ctx);
    avcodec_free_context(&acodec_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);

    return 0;
}
