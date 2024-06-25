#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wels/codec_api.h>
#include <wels/codec_app_def.h>
#include <wels/codec_def.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./decode ./video.h264");
        return 1;
    }

    int width = 0;
    int height = 0;

    SBufferInfo buff_info;
    uint64_t ui_ts = 0;
    int32_t slice_size;
    uint8_t *buff = NULL;
    uint8_t ui_start_code[4] = {0, 0, 0, 1};
    uint8_t *data[3] = {NULL};
    uint8_t *dst[3] = {NULL};
    int32_t buff_pos = 0;
    int32_t file_size;
    int32_t frame_count = 0;
    int32_t eof = 0;

    ISVCDecoder *decoder = NULL;
    SDecodingParam decoder_params = {0};
    decoder_params.sVideoProperty.size = sizeof(decoder_params.sVideoProperty);
    decoder_params.eEcActiveIdc = ERROR_CON_SLICE_MV_COPY_CROSS_IDR_FREEZE_RES_CHANGE;
    decoder_params.uiTargetDqLayer = (uint8_t)-1;
    decoder_params.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decoder_params.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    WelsCreateDecoder(&decoder);
    decoder->SetOption(DECODER_OPTION_ERROR_CON_IDC, &decoder_params.eEcActiveIdc);
    decoder->Initialize(&decoder_params);

    FILE *f = fopen(argv[1], "rb");
    fseek(f, 0L, SEEK_END);
    file_size = (int32_t)ftell(f);
    fseek(f, 0L, SEEK_SET);
    printf("file size: %d\n", file_size);

    buff = (uint8_t *)malloc(file_size + 4);
    fread(buff, 1, file_size, f);
    memcpy(buff + file_size, &ui_start_code[0], 4);

    while (true) {
        if (buff_pos >= file_size) {
            eof = true;
            if (eof) {
                decoder->SetOption(DECODER_OPTION_END_OF_STREAM, (void *)&eof);
            }
            break;
        }
        int i;
        for (i = 0; i < file_size; i++) {
            if ((buff[buff_pos + i] == 0 && buff[buff_pos + i + 1] == 0 && buff[buff_pos + i + 2] == 0 && buff[buff_pos + i + 3] == 1 && i > 0) ||
                (buff[buff_pos + i] == 0 && buff[buff_pos + i + 1] == 0 && buff[buff_pos + i + 2] == 1 && i > 0)) {
                break;
            }
        }
        slice_size = i;
        if (slice_size < 4) {
            buff_pos += slice_size;
            continue;
        }

        data[0] = NULL;
        data[1] = NULL;
        data[2] = NULL;

        ui_ts++;
        memset(&buff_info, 0, sizeof(SBufferInfo));

        buff_info.uiInBsTimeStamp = ui_ts;
        decoder->DecodeFrameNoDelay(buff + buff_pos, slice_size, data, &buff_info);

        if (buff_info.iBufferStatus == 1) {
            dst[0] = buff_info.pDst[0];
            dst[1] = buff_info.pDst[1];
            dst[2] = buff_info.pDst[2];
        }

        if (buff_info.iBufferStatus == 1) {
            width = buff_info.UsrData.sSystemBuffer.iWidth;
            height = buff_info.UsrData.sSystemBuffer.iHeight;
            ++frame_count;

            printf("width: %d - height: %d\n", width, height);
        }

        buff_pos += slice_size;
    }

    printf("-------------------------------------------------------\n");
    printf("width:\t\t%d\nheight:\t\t%d\nframes:\t\t%d\n", width, height, frame_count);
    printf("-------------------------------------------------------\n");

    return 0;
}
