#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <stdio.h>

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage ./parse <file>\n");
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    uint8_t buf[1024 * 1024];
    int data_size = fread(buf, 1, sizeof(buf), file);

    AVCodecParserContext *parser = av_parser_init(AV_CODEC_ID_H264);
    AVCodecContext *codec = avcodec_alloc_context3(NULL);

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    uint8_t *data = buf;
    while (data_size > 0) {
        int ret = av_parser_parse2(parser, codec, &pkt->data, &pkt->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
        if (ret < 0) {
            perror("av_parser_parse2");
            return 1;
        }

        data += ret;
        data_size -= ret;

        printf("pkt->size %d\n", pkt->size);
    }

    fclose(file);
}
