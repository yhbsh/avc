#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wels/codec_api.h>
#include <wels/codec_app_def.h>
#include <wels/codec_def.h>

#define START_CODE_SIZE 4

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "[USAGE]: ./decode ./video.h264\n");
        return 1;
    }

    // Decoder variables
    int width = 0, height = 0;
    int32_t frame_count = 0;
    ISVCDecoder *decoder = NULL;
    SDecodingParam decoder_params = {0};

    // Buffer variables
    SBufferInfo buff_info;
    uint8_t *buff = NULL;
    int32_t buff_pos = 0;
    int32_t file_size;

    // Decoding variables
    uint64_t timestamp = 0;
    int32_t slice_size;
    uint8_t start_code[START_CODE_SIZE] = {0, 0, 0, 1};
    uint8_t *data[3] = {NULL};
    uint8_t *dst[3] = {NULL};
    int32_t end_of_stream = 0;

    // Initialize decoder
    decoder_params.sVideoProperty.size = sizeof(decoder_params.sVideoProperty);
    decoder_params.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decoder_params.uiTargetDqLayer = (uint8_t)-1;
    decoder_params.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    WelsCreateDecoder(&decoder);
    (*decoder)->Initialize(decoder, &decoder_params);

    // Open and read input file
    FILE *input_file = fopen(argv[1], "rb");
    fseek(input_file, 0L, SEEK_END);
    file_size = (int32_t)ftell(input_file);
    fseek(input_file, 0L, SEEK_SET);
    printf("File size: %d bytes\n", file_size);

    buff = (uint8_t *)malloc(file_size + START_CODE_SIZE);
    fread(buff, 1, file_size, input_file);
    memcpy(buff + file_size, start_code, START_CODE_SIZE);

    // Main decoding loop
    while (1) {
        if (buff_pos >= file_size) {
            end_of_stream = 1;
            (*decoder)->SetOption(decoder, DECODER_OPTION_END_OF_STREAM, (void *)&end_of_stream);
            break;
        }

        // Find next NAL unit
        for (slice_size = 0; slice_size < file_size - buff_pos; slice_size++) {
            if ((buff[buff_pos + slice_size] == 0 && buff[buff_pos + slice_size + 1] == 0 &&
                 buff[buff_pos + slice_size + 2] == 0 && buff[buff_pos + slice_size + 3] == 1 && slice_size > 0) ||
                (buff[buff_pos + slice_size] == 0 && buff[buff_pos + slice_size + 1] == 0 &&
                 buff[buff_pos + slice_size + 2] == 1 && slice_size > 0)) {
                break;
            }
        }

        if (slice_size < 4) {
            buff_pos += slice_size;
            continue;
        }

        // Decode frame
        memset(&buff_info, 0, sizeof(SBufferInfo));
        buff_info.uiInBsTimeStamp = ++timestamp;
        (*decoder)->DecodeFrameNoDelay(decoder, buff + buff_pos, slice_size, data, &buff_info);

        if (buff_info.iBufferStatus == 1) {
            dst[0] = buff_info.pDst[0];
            dst[1] = buff_info.pDst[1];
            dst[2] = buff_info.pDst[2];

            width = buff_info.UsrData.sSystemBuffer.iWidth;
            height = buff_info.UsrData.sSystemBuffer.iHeight;
            frame_count++;
            printf("Frame %d - Width: %d, Height: %d\n", frame_count, width, height);
        }

        buff_pos += slice_size;
    }

    // Print summary
    printf("-------------------------------------------------------\n");
    printf("Final frame size: %d x %d\n", width, height);
    printf("Total frames decoded: %d\n", frame_count);
    printf("-------------------------------------------------------\n");

    // Clean up
    (*decoder)->Uninitialize(decoder);
    WelsDestroyDecoder(decoder);
    free(buff);
    fclose(input_file);

    return 0;
}
