#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wels/codec_api.h>
#include <wels/codec_app_def.h>
#include <wels/codec_def.h>

#define BUFFER_SIZE (1024*1024*64)

static uint8_t buffer[BUFFER_SIZE];

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./decode ./video.h264\n");
        return 1;
    }

    SDecodingParam decoder_params = {
        .sVideoProperty.size = sizeof(decoder_params.sVideoProperty),
        .eEcActiveIdc = ERROR_CON_SLICE_COPY,
        .uiTargetDqLayer = (uint8_t)-1,
        .sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT,
    };

    ISVCDecoder *decoder = NULL;
    if (WelsCreateDecoder(&decoder) < 0) {
        fprintf(stderr, "ERROR: WelsCreateDecoder\n");
        return 1;
    }
    (*decoder)->Initialize(decoder, &decoder_params);

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("fopen");
        return 1;
    }

    SBufferInfo buffer_info;
    int32_t buffer_pos = 0;
    uint64_t timestamp = 0;
    int32_t slice_size;
    uint8_t start_code[4] = {0, 0, 0, 1};
    uint8_t *data[3] = {NULL};
    uint8_t *dst[3] = {NULL};
    int32_t end_of_stream = 0;

    ssize_t nbytes;
    if ((nbytes = fread(buffer, 1, BUFFER_SIZE, file)) < 0) {
        perror("fread");
        return 1;
    }
    memcpy(buffer + nbytes, start_code, 4);

    int frames = 0;
    for (;;) {
        if (buffer_pos >= nbytes) {
            end_of_stream = 1;
            (*decoder)->SetOption(decoder, DECODER_OPTION_END_OF_STREAM, (void *)&end_of_stream);
            break;
        }

        for (slice_size = 0; slice_size < nbytes - buffer_pos; slice_size++) {
            if ((buffer[buffer_pos + slice_size + 0] == 0 && buffer[buffer_pos + slice_size + 1] == 0 &&
                 buffer[buffer_pos + slice_size + 2] == 0 && buffer[buffer_pos + slice_size + 3] == 1 && slice_size > 0) ||
                (buffer[buffer_pos + slice_size + 0] == 0 && buffer[buffer_pos + slice_size + 1] == 0 &&
                 buffer[buffer_pos + slice_size + 2] == 1 && slice_size > 0)) {
                break;
            }
        }

        if (slice_size < 4) {
            buffer_pos += slice_size;
            continue;
        }

        memset(&buffer_info, 0, sizeof(SBufferInfo));
        buffer_info.uiInBsTimeStamp = ++timestamp;
        (*decoder)->DecodeFrameNoDelay(decoder, buffer + buffer_pos, slice_size, data, &buffer_info);

        if (buffer_info.iBufferStatus == 1) {
            dst[0] = buffer_info.pDst[0];
            dst[1] = buffer_info.pDst[1];
            dst[2] = buffer_info.pDst[2];

            int width = buffer_info.UsrData.sSystemBuffer.iWidth;
            int height = buffer_info.UsrData.sSystemBuffer.iHeight;
            frames++;
            printf("Frame %d - Width: %d, Height: %d\n", frames, width, height);
        }

        buffer_pos += slice_size;
    }

    printf("-------------------------------------------------------\n");
    printf("Total frames decoded: %d\n", frames);
    printf("-------------------------------------------------------\n");

    return 0;
}
