#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t *data;
    size_t size;
    size_t pos;
} BitStream;

typedef struct {
    uint8_t nal_unit_type;
    uint8_t nal_ref_idc;
    uint8_t forbidden_zero_bit;
    uint8_t *rbsp_data;
    size_t rbsp_size;
} NALUnit;

typedef struct {
    uint8_t profile_idc;
    uint8_t constraint_set_flags;
    uint8_t level_idc;
    uint8_t seq_parameter_set_id;
    uint8_t log2_max_frame_num_minus4;
    uint8_t pic_order_cnt_type;
    uint8_t log2_max_pic_order_cnt_lsb_minus4;
    uint8_t num_ref_frames;
    uint8_t gaps_in_frame_num_value_allowed_flag;
    uint16_t pic_width_in_mbs_minus1;
    uint16_t pic_height_in_map_units_minus1;
    uint8_t frame_mbs_only_flag;
    uint8_t direct_8x8_inference_flag;
    uint8_t frame_cropping_flag;
    uint32_t frame_crop_left_offset;
    uint32_t frame_crop_right_offset;
    uint32_t frame_crop_top_offset;
    uint32_t frame_crop_bottom_offset;
} SPS;

typedef struct {
    uint8_t pic_parameter_set_id;
    uint8_t seq_parameter_set_id;
    uint8_t entropy_coding_mode_flag;
    uint8_t pic_order_present_flag;
    uint8_t num_slice_groups_minus1;
    uint8_t num_ref_idx_l0_active_minus1;
    uint8_t num_ref_idx_l1_active_minus1;
    uint8_t weighted_pred_flag;
    uint8_t weighted_bipred_idc;
    int8_t pic_init_qp_minus26;
    int8_t pic_init_qs_minus26;
    int8_t chroma_qp_index_offset;
    uint8_t deblocking_filter_control_present_flag;
    uint8_t constrained_intra_pred_flag;
    uint8_t redundant_pic_cnt_present_flag;
} PPS;

typedef struct {
    uint8_t *buffer;
    size_t size;
    size_t capacity;
} NALBuffer;

BitStream* create_bit_stream(uint8_t *data, size_t size) {
    BitStream *bs = malloc(sizeof(BitStream));
    bs->data = data;
    bs->size = size;
    bs->pos = 0;
    return bs;
}

void free_bit_stream(BitStream *bs) {
    free(bs);
}

uint8_t read_bit(BitStream *bs) {
    if (bs->pos >= bs->size * 8) return 0;

    size_t byte_pos = bs->pos / 8;
    size_t bit_pos = 7 - (bs->pos % 8);

    uint8_t bit = (bs->data[byte_pos] >> bit_pos) & 0x01;
    bs->pos++;

    return bit;
}

uint32_t read_bits(BitStream *bs, uint8_t n) {
    uint32_t val = 0;

    for (uint8_t i = 0; i < n; i++) {
        val = (val << 1) | read_bit(bs);
    }

    return val;
}

uint32_t read_ue(BitStream *bs) {
    int leading_zero_bits = -1;
    uint8_t bit;

    do {
        bit = read_bit(bs);
        leading_zero_bits++;
    } while (bit == 0);

    if (leading_zero_bits == 0) {
        return 0;
    }

    uint32_t suffix = read_bits(bs, leading_zero_bits);
    return (1 << leading_zero_bits) - 1 + suffix;
}

int32_t read_se(BitStream *bs) {
    uint32_t val = read_ue(bs);

    if (val & 0x01) {
        return (val + 1) / 2;
    } else {
        return -(val / 2);
    }
}

uint8_t* find_next_start_code(uint8_t *data, size_t size, size_t *offset) {
    if (size < 3) return NULL;

    for (size_t i = 0; i < size - 3; i++) {
        if (data[i] == 0 && data[i + 1] == 0 &&
            ((data[i + 2] == 1) || (data[i + 2] == 0 && data[i + 3] == 1))) {

            *offset = (data[i + 2] == 0) ? 4 : 3;
            return &data[i];
        }
    }

    return NULL;
}

void remove_emulation_prevention_bytes(uint8_t *src, size_t src_size, uint8_t *dst, size_t *dst_size) {
    size_t i = 0, j = 0;

    while (i < src_size) {
        if (i + 2 < src_size && src[i] == 0 && src[i + 1] == 0 && src[i + 2] == 3) {
            dst[j++] = 0;
            dst[j++] = 0;
            i += 3;
        } else {
            dst[j++] = src[i++];
        }
    }

    *dst_size = j;
}

NALUnit* parse_nal_unit(uint8_t *data, size_t size) {
    if (size < 1) return NULL;

    NALUnit *nal = malloc(sizeof(NALUnit));

    nal->forbidden_zero_bit = (data[0] >> 7) & 0x01;
    nal->nal_ref_idc = (data[0] >> 5) & 0x03;
    nal->nal_unit_type = data[0] & 0x1F;

    uint8_t *rbsp = malloc(size);
    size_t rbsp_size;

    remove_emulation_prevention_bytes(data + 1, size - 1, rbsp, &rbsp_size);

    nal->rbsp_data = rbsp;
    nal->rbsp_size = rbsp_size;

    return nal;
}

void free_nal_unit(NALUnit *nal) {
    if (nal) {
        free(nal->rbsp_data);
        free(nal);
    }
}

SPS* parse_sps(NALUnit *nal) {
    if (nal->nal_unit_type != 7) return NULL;

    BitStream *bs = create_bit_stream(nal->rbsp_data, nal->rbsp_size);
    SPS *sps = malloc(sizeof(SPS));

    sps->profile_idc = read_bits(bs, 8);
    sps->constraint_set_flags = read_bits(bs, 8);
    sps->level_idc = read_bits(bs, 8);
    sps->seq_parameter_set_id = read_ue(bs);

    if (sps->profile_idc == 100 || sps->profile_idc == 110 ||
        sps->profile_idc == 122 || sps->profile_idc == 244 ||
        sps->profile_idc == 44  || sps->profile_idc == 83  ||
        sps->profile_idc == 86  || sps->profile_idc == 118 ||
        sps->profile_idc == 128 || sps->profile_idc == 138) {

        uint32_t chroma_format_idc = read_ue(bs);

        if (chroma_format_idc == 3) {
            read_bit(bs); // separate_colour_plane_flag
        }

        read_ue(bs); // bit_depth_luma_minus8
        read_ue(bs); // bit_depth_chroma_minus8
        read_bit(bs); // qpprime_y_zero_transform_bypass_flag

        uint8_t seq_scaling_matrix_present_flag = read_bit(bs);

        if (seq_scaling_matrix_present_flag) {
            int size = (chroma_format_idc != 3) ? 8 : 12;
            for (int i = 0; i < size; i++) {
                uint8_t seq_scaling_list_present_flag = read_bit(bs);
                if (seq_scaling_list_present_flag) {
                    if (i < 6) {
                        // scaling_list(ScalingList4x4[i], 16)
                        // Skip the scaling list
                        for (int j = 0; j < 16; j++) {
                            read_se(bs);
                        }
                    } else {
                        // scaling_list(ScalingList8x8[i - 6], 64)
                        // Skip the scaling list
                        for (int j = 0; j < 64; j++) {
                            read_se(bs);
                        }
                    }
                }
            }
        }
    }

    sps->log2_max_frame_num_minus4 = read_ue(bs);
    sps->pic_order_cnt_type = read_ue(bs);

    if (sps->pic_order_cnt_type == 0) {
        sps->log2_max_pic_order_cnt_lsb_minus4 = read_ue(bs);
    } else if (sps->pic_order_cnt_type == 1) {
        // Skip
        read_bit(bs); // delta_pic_order_always_zero_flag
        read_se(bs);  // offset_for_non_ref_pic
        read_se(bs);  // offset_for_top_to_bottom_field

        uint32_t num_ref_frames_in_pic_order_cnt_cycle = read_ue(bs);

        for (uint32_t i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            read_se(bs); // offset_for_ref_frame[i]
        }
    }

    sps->num_ref_frames = read_ue(bs);
    sps->gaps_in_frame_num_value_allowed_flag = read_bit(bs);
    sps->pic_width_in_mbs_minus1 = read_ue(bs);
    sps->pic_height_in_map_units_minus1 = read_ue(bs);
    sps->frame_mbs_only_flag = read_bit(bs);

    if (!sps->frame_mbs_only_flag) {
        read_bit(bs); // mb_adaptive_frame_field_flag
    }

    sps->direct_8x8_inference_flag = read_bit(bs);
    sps->frame_cropping_flag = read_bit(bs);

    if (sps->frame_cropping_flag) {
        sps->frame_crop_left_offset = read_ue(bs);
        sps->frame_crop_right_offset = read_ue(bs);
        sps->frame_crop_top_offset = read_ue(bs);
        sps->frame_crop_bottom_offset = read_ue(bs);
    }

    free_bit_stream(bs);
    return sps;
}

void free_sps(SPS *sps) {
    free(sps);
}

PPS* parse_pps(NALUnit *nal) {
    if (nal->nal_unit_type != 8) return NULL;

    BitStream *bs = create_bit_stream(nal->rbsp_data, nal->rbsp_size);
    PPS *pps = malloc(sizeof(PPS));

    pps->pic_parameter_set_id = read_ue(bs);
    pps->seq_parameter_set_id = read_ue(bs);
    pps->entropy_coding_mode_flag = read_bit(bs);
    pps->pic_order_present_flag = read_bit(bs);
    pps->num_slice_groups_minus1 = read_ue(bs);

    if (pps->num_slice_groups_minus1 > 0) {
        uint32_t slice_group_map_type = read_ue(bs);

        if (slice_group_map_type == 0) {
            for (uint32_t i = 0; i <= pps->num_slice_groups_minus1; i++) {
                read_ue(bs); // run_length_minus1[i]
            }
        } else if (slice_group_map_type == 2) {
            for (uint32_t i = 0; i < pps->num_slice_groups_minus1; i++) {
                read_ue(bs); // top_left[i]
                read_ue(bs); // bottom_right[i]
            }
        } else if (slice_group_map_type == 3 || slice_group_map_type == 4 || slice_group_map_type == 5) {
            read_bit(bs); // slice_group_change_direction_flag
            read_ue(bs);  // slice_group_change_rate_minus1
        } else if (slice_group_map_type == 6) {
            uint32_t pic_size_in_map_units_minus1 = read_ue(bs);

            uint32_t bits_needed = 0;
            uint32_t temp = pps->num_slice_groups_minus1 + 1;

            while (temp > 0) {
                bits_needed++;
                temp >>= 1;
            }

            for (uint32_t i = 0; i <= pic_size_in_map_units_minus1; i++) {
                read_bits(bs, bits_needed); // slice_group_id[i]
            }
        }
    }

    pps->num_ref_idx_l0_active_minus1 = read_ue(bs);
    pps->num_ref_idx_l1_active_minus1 = read_ue(bs);
    pps->weighted_pred_flag = read_bit(bs);
    pps->weighted_bipred_idc = read_bits(bs, 2);
    pps->pic_init_qp_minus26 = read_se(bs);
    pps->pic_init_qs_minus26 = read_se(bs);
    pps->chroma_qp_index_offset = read_se(bs);
    pps->deblocking_filter_control_present_flag = read_bit(bs);
    pps->constrained_intra_pred_flag = read_bit(bs);
    pps->redundant_pic_cnt_present_flag = read_bit(bs);

    free_bit_stream(bs);
    return pps;
}

void free_pps(PPS *pps) {
    free(pps);
}

void init_nal_buffer(NALBuffer *buffer) {
    buffer->capacity = 1024;
    buffer->size = 0;
    buffer->buffer = malloc(buffer->capacity);
}

void free_nal_buffer(NALBuffer *buffer) {
    free(buffer->buffer);
    buffer->buffer = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}

void add_to_nal_buffer(NALBuffer *buffer, uint8_t *data, size_t size) {
    if (buffer->size + size > buffer->capacity) {
        buffer->capacity = buffer->size + size + 1024;
        buffer->buffer = realloc(buffer->buffer, buffer->capacity);
    }

    memcpy(buffer->buffer + buffer->size, data, size);
    buffer->size += size;
}

void parse_nal_units(uint8_t *data, size_t size) {
    size_t offset;
    uint8_t *start = data;
    size_t remaining = size;
    NALBuffer nal_buffer;
    init_nal_buffer(&nal_buffer);

    while ((start = find_next_start_code(start, remaining, &offset)) != NULL) {
        start += offset;
        remaining = size - (start - data);

        if (remaining <= 0) break;

        uint8_t *next_start = find_next_start_code(start, remaining, &offset);
        size_t nal_size;

        if (next_start) {
            nal_size = next_start - start;
        } else {
            nal_size = remaining;
        }

        add_to_nal_buffer(&nal_buffer, start, nal_size);

        NALUnit *nal = parse_nal_unit(nal_buffer.buffer, nal_buffer.size);

        if (nal) {
            printf("NAL Unit Type: %d\n", nal->nal_unit_type);

            if (nal->nal_unit_type == 7) {
                SPS *sps = parse_sps(nal);

                if (sps) {
                    printf("SPS: Profile: %d, Level: %d, Resolution: %dx%d\n",
                           sps->profile_idc, sps->level_idc,
                           (sps->pic_width_in_mbs_minus1 + 1) * 16,
                           (sps->pic_height_in_map_units_minus1 + 1) * 16 * (2 - sps->frame_mbs_only_flag));

                    free_sps(sps);
                }
            } else if (nal->nal_unit_type == 8) {
                PPS *pps = parse_pps(nal);

                if (pps) {
                    printf("PPS: PPS ID: %d, SPS ID: %d\n",
                           pps->pic_parameter_set_id, pps->seq_parameter_set_id);

                    free_pps(pps);
                }
            }

            free_nal_unit(nal);
        }

        nal_buffer.size = 0;

        if (!next_start) break;

        start = next_start;
        remaining = size - (start - data);
    }

    free_nal_buffer(&nal_buffer);
}

int parse_h264_file(const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *buffer = malloc(file_size);

    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory\n");
        fclose(file);
        return -1;
    }

    if (fread(buffer, 1, file_size, file) != file_size) {
        fprintf(stderr, "Failed to read file\n");
        free(buffer);
        fclose(file);
        return -1;
    }

    parse_nal_units(buffer, file_size);

    free(buffer);
    fclose(file);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <h264_file>\n", argv[0]);
        return 1;
    }

    return parse_h264_file(argv[1]);
}
