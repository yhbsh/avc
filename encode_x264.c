#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include <x264.h>

#define WIDTH 640
#define HEIGHT 480

void encode_fractal_noise_vertex(int frame, x264_picture_t pic_in);
void encode_polar_coordinate_color_cycling(int frame, x264_picture_t pic_in);
void encode_swirling_vortex(int frame, x264_picture_t pic_in);
void encode_fractal_noise_vertex2(int frame, x264_picture_t pic_in);
void encode_water_effect(int frame, x264_picture_t pic_in);
void encode_neon_glow_effect(int frame, x264_picture_t pic_in);
void encode_game_of_life(int frame, x264_picture_t pic_in);

int main() {
    int fps        = 5;
    int num_frames = 200;

    x264_t *encoder;
    x264_picture_t pic_in, pic_out;
    x264_param_t param;
    FILE *h264_file;

    x264_param_default_preset(&param, "veryfast", "zerolatency");
    param.i_csp            = X264_CSP_I420;
    param.i_width          = WIDTH;
    param.i_height         = HEIGHT;
    param.i_fps_num        = fps;
    param.i_fps_den        = 1;
    param.i_keyint_max     = fps;
    param.b_repeat_headers = 1;
    x264_param_apply_profile(&param, "baseline");

    encoder   = x264_encoder_open(&param);
    h264_file = fopen("video.h264", "wb");

    x264_picture_alloc(&pic_in, X264_CSP_I420, WIDTH, HEIGHT);
    pic_in.i_type = X264_TYPE_AUTO;

    for (int i = 0; i < num_frames; i++) {
        // encode_fractal_noise_vertex(i, pic_in);
        // encode_swirling_vortex(i, pic_in);
        encode_polar_coordinate_color_cycling(i, pic_in);
        // encode_fractal_noise_vertex2(i, pic_in);
        // encode_water_effect(i, pic_in);
        // encode_neon_glow_effect(i, pic_in);
        // encode_game_of_life(i, pic_in);

        x264_nal_t *nals;
        int i_nal;
        x264_encoder_encode(encoder, &nals, &i_nal, &pic_in, &pic_out);
        for (int j = 0; j < i_nal; j++) {
            fwrite(nals[j].p_payload, 1, nals[j].i_payload, h264_file);
        }
    }

    while (x264_encoder_delayed_frames(encoder)) {
        x264_nal_t *nals;
        int i_nal;
        x264_encoder_encode(encoder, &nals, &i_nal, NULL, &pic_out);
        for (int j = 0; j < i_nal; j++) {
            fwrite(nals[j].p_payload, 1, nals[j].i_payload, h264_file);
        }
    }

    x264_picture_clean(&pic_in);
    x264_encoder_close(encoder);
    fclose(h264_file);

    return 0;
}

void encode_polar_coordinate_color_cycling(int frame, x264_picture_t pic_in) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            double angle                       = atan2(y - HEIGHT / 2, x - WIDTH / 2);
            double radius                      = sqrt(pow(x - WIDTH / 2, 2) + pow(y - HEIGHT / 2, 2));
            double intensity                   = 128 + 127 * sin(radius * 0.05 + angle * 3 + frame * 0.1);
            pic_in.img.plane[0][x + y * WIDTH] = (int)intensity;
        }
        if (y < HEIGHT / 2) {
            uint8_t *u_plane = pic_in.img.plane[1] + y * pic_in.img.i_stride[1];
            uint8_t *v_plane = pic_in.img.plane[2] + y * pic_in.img.i_stride[2];
            for (int x = 0; x < WIDTH / 2; x++) {
                double angle  = atan2(y - HEIGHT / 4, x - WIDTH / 4);
                double radius = sqrt(pow(x - WIDTH / 4, 2) + pow(y - HEIGHT / 4, 2));
                u_plane[x]    = 128 + (int)(127 * sin(radius * 0.1 + frame * 0.05));
                v_plane[x]    = 128 + (int)(127 * cos(angle * 5 + frame * 0.03));
            }
        }
    }
}

void encode_fractal_noise_vertex(int frame, x264_picture_t pic_in) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            double angle     = atan2(y - HEIGHT / 2, x - WIDTH / 2);
            double radius    = sqrt(pow(x - WIDTH / 2, 2) + pow(y - HEIGHT / 2, 2));
            double phase     = frame * 0.02;
            double frequency = 0.1 * (radius + 1);
            double intensity = 128 + 127 * sin(frequency * radius + 5 * angle + phase);
            intensity += 127 * sin(3 * frequency * (radius * 0.5) - 3 * angle + phase);
            intensity                          = intensity / 2;
            pic_in.img.plane[0][x + y * WIDTH] = (int)intensity;
        }
        if (y < HEIGHT / 2) {
            uint8_t *u_plane = pic_in.img.plane[1] + y * pic_in.img.i_stride[1];
            uint8_t *v_plane = pic_in.img.plane[2] + y * pic_in.img.i_stride[2];
            for (int x = 0; x < WIDTH / 2; x++) {
                double angle     = atan2(y - HEIGHT / 4, x - WIDTH / 4);
                double radius    = sqrt(pow(x - WIDTH / 4, 2) + pow(y - HEIGHT / 4, 2));
                double phase     = frame * 0.03;
                double frequency = 0.15 * (radius + 1); // Higher frequency for color
                u_plane[x]       = 128 + (int)(127 * cos(frequency * radius - 4 * angle + phase));
                v_plane[x]       = 128 + (int)(127 * cos(frequency * radius + 4 * angle + phase));
            }
        }
    }
}

void encode_swirling_vortex(int frame, x264_picture_t pic_in) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            double angle                       = atan2(y - HEIGHT / 2, x - WIDTH / 2);
            double radius                      = sqrt(pow(x - WIDTH / 2, 2) + pow(y - HEIGHT / 2, 2));
            double wave                        = (radius * 0.05) + (frame * 0.15);
            double swirl                       = angle + wave;
            double intensity                   = 128 + 127 * sin(swirl * 2);
            pic_in.img.plane[0][x + y * WIDTH] = (int)intensity;
        }
        if (y < HEIGHT / 2) {
            uint8_t *u_plane = pic_in.img.plane[1] + y * pic_in.img.i_stride[1];
            uint8_t *v_plane = pic_in.img.plane[2] + y * pic_in.img.i_stride[2];
            for (int x = 0; x < WIDTH / 2; x++) {
                double angle  = atan2(y - HEIGHT / 4, x - WIDTH / 4);
                double radius = sqrt(pow(x - WIDTH / 4, 2) + pow(y - HEIGHT / 4, 2));
                double wave   = (radius * 0.1) + (frame * 0.1);
                double swirl  = angle + wave;
                u_plane[x]    = 128 + (int)(127 * sin(swirl * 3));
                v_plane[x]    = 128 + (int)(127 * cos(swirl * 4));
            }
        }
    }
}

void encode_fractal_noise_vertex2(int frame, x264_picture_t pic_in) {
    const int max_radius_effect = 3;
    const double frequency_base = 0.04;
    const double phase_shift    = frame * 0.02;
    const double color_shift    = frame * 0.03;

    double (^modulate_intensity)(double, double, double) = ^double(double radius, double angle, double phase) {
      return 128 + 127 * sin(radius * frequency_base + max_radius_effect * angle + phase);
    };

    double (^complex_wave)(int, int, double) = ^(int x, int y, double phase_shift) {
      double local_radius = sqrt(x * x + y * y);
      double local_angle  = atan2(y, x);
      return modulate_intensity(local_radius, local_angle, phase_shift);
    };

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int cx                             = x - WIDTH / 2;
            int cy                             = y - HEIGHT / 2;
            pic_in.img.plane[0][x + y * WIDTH] = (int)complex_wave(cx, cy, phase_shift);
        }
    }

    for (int y = 0; y < HEIGHT / 2; y++) {
        uint8_t *u_plane = pic_in.img.plane[1] + y * pic_in.img.i_stride[1];
        uint8_t *v_plane = pic_in.img.plane[2] + y * pic_in.img.i_stride[2];
        for (int x = 0; x < WIDTH / 2; x++) {
            int cx        = x - WIDTH / 4;
            int cy        = y - HEIGHT / 4;
            double u_wave = complex_wave(cx, cy, color_shift);
            double v_wave = complex_wave(-cx, -cy, -color_shift);
            u_plane[x]    = 128 + (int)(127 * cos(u_wave));
            v_plane[x]    = 128 + (int)(127 * sin(v_wave));
        }
    }
}

void encode_water_effect(int frame, x264_picture_t pic_in) {
    const double frequency_base = 0.05;
    const double phase_shift    = frame * 0.03;
    const double color_shift    = frame * 0.02;

    double (^modulate_intensity)(double, double, double) = ^double(double radius, double angle, double phase) {
      return 128 + 127 * sin(radius * frequency_base + angle + phase);
    };

    double (^complex_wave)(int, int, double) = ^(int x, int y, double phase_shift) {
      double local_radius = sqrt(x * x + y * y);
      double local_angle  = atan2(y, x);
      return modulate_intensity(local_radius, local_angle, phase_shift);
    };

    for (int y = 0; y < HEIGHT / 2; y++) {
        uint8_t *u_plane = pic_in.img.plane[1] + y * pic_in.img.i_stride[1];
        uint8_t *v_plane = pic_in.img.plane[2] + y * pic_in.img.i_stride[2];
        for (int x = 0; x < WIDTH / 2; x++) {
            int cx        = x - WIDTH / 4;
            int cy        = y - HEIGHT / 4;
            double u_wave = complex_wave(cx, cy, color_shift);
            double v_wave = complex_wave(-cx, -cy, -color_shift);
            u_plane[x]    = 128 + (int)(127 * cos(u_wave));
            v_plane[x]    = 128 + (int)(127 * sin(v_wave));
        }
    }
}

void encode_neon_glow_effect(int frame, x264_picture_t pic_in) {
    const double frequency_base = 0.08;
    const double phase_shift    = frame * 0.05;
    const double color_shift    = frame * 0.04;

    double (^modulate_intensity)(double, double, double) = ^double(double radius, double angle, double phase) {
      return 128 + 127 * sin(radius * frequency_base + angle + phase);
    };

    double (^complex_wave)(int, int, double) = ^(int x, int y, double phase_shift) {
      double local_radius = sqrt(x * x + y * y);
      double local_angle  = atan2(y, x);
      return modulate_intensity(local_radius, local_angle, phase_shift);
    };

    for (int y = 0; y < HEIGHT / 2; y++) {
        uint8_t *u_plane = pic_in.img.plane[1] + y * pic_in.img.i_stride[1];
        uint8_t *v_plane = pic_in.img.plane[2] + y * pic_in.img.i_stride[2];
        for (int x = 0; x < WIDTH / 2; x++) {
            int cx        = x - WIDTH / 4;
            int cy        = y - HEIGHT / 4;
            double u_wave = complex_wave(cx, cy, color_shift);
            double v_wave = complex_wave(-cx, -cy, -color_shift);
            u_plane[x]    = 128 + (int)(127 * sin(u_wave));
            v_plane[x]    = 128 + (int)(127 * cos(v_wave));
        }
    }
}

void encode_game_of_life(int frame, x264_picture_t pic_in) {
    static uint8_t current[HEIGHT / 2][WIDTH / 2];
    static uint8_t next[HEIGHT / 2][WIDTH / 2];
    int half_width  = WIDTH / 2;
    int half_height = HEIGHT / 2;

    if (frame == 0) {
        for (int y = 0; y < half_height; y++) {
            for (int x = 0; x < half_width; x++) {
                current[y][x] = (rand() % 2) * 255;
            }
        }
    } else {
        for (int y = 0; y < half_height; y++) {
            for (int x = 0; x < half_width; x++) {
                int live_neighbors = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int ny = y + dy, nx = x + dx;
                        if (ny >= 0 && ny < half_height && nx >= 0 && nx < half_width) {
                            live_neighbors += (current[ny][nx] == 255) ? 1 : 0;
                        }
                    }
                }
                next[y][x] = (current[y][x] == 255 && (live_neighbors < 2 || live_neighbors > 3)) ? 0 : ((current[y][x] == 0 && live_neighbors == 3) ? 255 : current[y][x]);
            }
        }
        memcpy(current, next, sizeof(current));
    }

    for (int y = 0; y < half_height; y++) {
        uint8_t *u_plane = pic_in.img.plane[1] + y * pic_in.img.i_stride[1];
        for (int x = 0; x < half_width; x++) {
            u_plane[x] = current[y][x];
        }
    }
}
