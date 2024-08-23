#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>

#include <stdio.h>

void encode(AVCodecContext *codec_ctx, AVFrame *frame, AVPacket *packet, FILE *file) {
    int ret;
    if (frame) printf("Send frame %3" PRId64 "\n", frame->pts);
    ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: avcodec_send_frame(codec_ctx, frame): %s\n", av_err2str(ret));
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            fprintf(stderr, "[ERROR]: avcodec_receive_packet(codec_ctx, packet): %s\n", av_err2str(ret));
            exit(1);
        }

        printf("Write packet %3" PRId64 " (size=%5d)\n", packet->pts, packet->size);
        fwrite(packet->data, 1, packet->size, file);
        av_packet_unref(packet);
    }
}

int main(void) {
    FILE *file = fopen("video", "wb");

    const char *codec_name = "h264_videotoolbox";
    const AVCodec *codec   = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        fprintf(stderr, "[ERROR]: codec: %s not found\n", codec_name);
        return 1;
    }
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    codec_ctx->bit_rate       = 400000;
    codec_ctx->width          = 800;
    codec_ctx->height         = 600;
    codec_ctx->time_base      = (AVRational){1, 25};
    codec_ctx->framerate      = (AVRational){25, 1};
    codec_ctx->gop_size       = 10;
    codec_ctx->max_b_frames   = 1;
    codec_ctx->pix_fmt        = AV_PIX_FMT_YUV420P;
    codec_ctx->color_range    = 2;

    int ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: avcodec_open2(codec_ctx, codec, NULL): %s\n", av_err2str(ret));
        return 1;
    }

    AVFrame *frame = av_frame_alloc();
    frame->format  = codec_ctx->pix_fmt;
    frame->width   = codec_ctx->width;
    frame->height  = codec_ctx->height;

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: av_frame_get_buffer(frame, 0): %s\n", av_err2str(ret));
        return 1;
    }

    AVPacket *packet = av_packet_alloc();

    int nb_frames = 25 * 100;
    for (int i = 0; i < nb_frames; i++) {
        fflush(stdout);

        ret = av_frame_make_writable(frame);
        for (int y = 0; y < codec_ctx->height; y++) {
            for (int x = 0; x < codec_ctx->width; x++) {
                frame->data[0][(y >> 0) * frame->linesize[0] + (x >> 0)] = x + y + i * 3;
                frame->data[1][(y >> 1) * frame->linesize[1] + (x >> 1)] = 128 + (y >> 1) + i * 2;
                frame->data[2][(y >> 1) * frame->linesize[2] + (x >> 1)] = 064 + (x >> 1) + i * 5;
            }
        }
        frame->pts = i;
        encode(codec_ctx, frame, packet, file);
    }

    encode(codec_ctx, NULL, packet, file);

    fclose(file);

    avcodec_free_context(&codec_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);

    return 0;
}
