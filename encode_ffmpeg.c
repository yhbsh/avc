#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

#include <stdio.h>

void encode(AVCodecContext *encoder_ctx, AVFrame *frame, AVPacket *packet, FILE *file) {
    int ret;

    ret = avcodec_send_frame(encoder_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "[ERROR]: avcodec_send_frame(encoder_ctx, frame): %s\n", av_err2str(ret));
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(encoder_ctx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            fprintf(stderr, "[ERROR]: avcodec_receive_packet(encoder_ctx, packet): %s\n", av_err2str(ret));
            exit(1);
        }

        printf("Write packet %3" PRId64 " (size=%5d)\n", packet->pts, packet->size);
        fwrite(packet->data, 1, packet->size, file);
        av_packet_unref(packet);
    }
}

int main(int argc, const char *argv[1]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./encoder_name_ffmpeg <file>\n");
        return 1;
    }

    int ret;

    FILE *file = fopen(argv[1], "wb");

    const char *encoder_name = "h264_videotoolbox";
    const AVCodec *encoder   = avcodec_find_encoder_by_name(encoder_name);
    if (!encoder) {
        fprintf(stderr, "[ERROR]: encoder: %s not found\n", encoder_name);
        return 1;
    }

    AVCodecContext *encoder_ctx = avcodec_alloc_context3(encoder);
    encoder_ctx->bit_rate       = 400000;
    encoder_ctx->width          = 800;
    encoder_ctx->height         = 600;
    encoder_ctx->time_base      = (AVRational){1, 25};
    encoder_ctx->framerate      = (AVRational){25, 1};
    encoder_ctx->gop_size       = 10;
    encoder_ctx->max_b_frames   = 1;
    encoder_ctx->pix_fmt         = AV_PIX_FMT_YUV420P;
    encoder_ctx->color_range     = AVCOL_RANGE_JPEG;
    encoder_ctx->colorspace      = AVCOL_SPC_BT709;
    encoder_ctx->color_primaries = AVCOL_PRI_BT709;
    encoder_ctx->color_trc       = AVCOL_TRC_BT709;

    if ((ret = avcodec_open2(encoder_ctx, encoder, NULL)) < 0) {
        fprintf(stderr, "[ERROR]: cannot open encoder: %s\n", av_err2str(ret));
        return 1;
    }

    AVFrame *frame     = av_frame_alloc();
    frame->format      = encoder_ctx->pix_fmt;
    frame->width       = encoder_ctx->width;
    frame->height      = encoder_ctx->height;
    frame->color_range = AVCOL_RANGE_JPEG;

    if ((ret = av_frame_get_buffer(frame, 0)) < 0) {
        fprintf(stderr, "[ERROR]: cannot allocate frame buffer: %s\n", av_err2str(ret));
        return 1;
    }

    AVPacket *packet = av_packet_alloc();

    int nb_frames = 100;

    for (int i = 0; i < nb_frames; i++) {
        fflush(stdout);
        ret = av_frame_make_writable(frame);

        for (int y = 0; y < frame->height; y++) {
            for (int x = 0; x < frame->width; x++) {
                float nx = (float)x / frame->width;
                float ny = (float)y / frame->height;

                // Simple moving gradient for Y plane
                frame->data[0][y * frame->linesize[0] + x] = 128 + 127 * sin((nx + ny + i * 0.001) * M_PI);

                // Simple color pattern for U and V planes
                if (y % 2 == 0 && x % 2 == 0) {
                    int uvx = x / 2;
                    int uvy = y / 2;

                    frame->data[1][uvy * frame->linesize[1] + uvx] = 128 + 127 * sin(nx * M_PI + i * 0.001);
                    frame->data[2][uvy * frame->linesize[2] + uvx] = 128 + 127 * sin(ny * M_PI + i * 0.001);
                }
            }
        }

        frame->pts = i;
        encode(encoder_ctx, frame, packet, file);
    }

    encode(encoder_ctx, NULL, packet, file);

    fclose(file);

    avcodec_free_context(&encoder_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);

    return 0;
}
