#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>

#define BUFFER_SIZE 4096

const char *vert_source = "#version 410\n"
                          "layout (location = 0) in vec2 position;\n"
                          "layout (location = 1) in vec2 texCoord;\n"
                          "out vec2 texCoordVarying;\n"
                          "void main() {\n"
                          "    gl_Position = vec4(position, 0.0, 1.0);\n"
                          "    texCoordVarying = texCoord;\n"
                          "}\n";

const char *frag_source = "#version 410\n"
                          "in vec2 texCoordVarying;\n"
                          "uniform sampler2D textureY;\n"
                          "uniform sampler2D textureCb;\n"
                          "uniform sampler2D textureCr;\n"
                          "out vec4 fragColor;\n"
                          "void main() {\n"
                          "    float y  = texture(textureY , texCoordVarying).r;\n"
                          "    float cb = texture(textureCb, texCoordVarying).r - 0.5;\n"
                          "    float cr = texture(textureCr, texCoordVarying).r - 0.5;\n"
                          "    float r = y + (1.40200 * cr);\n"
                          "    float g = y - (0.344 * cb) - (0.714 * cr);\n"
                          "    float b = y + (1.770 * cb);\n"
                          "    fragColor = vec4(r, g, b, 1.0);\n"
                          "}\n";
void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_Q) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        return -1;
    }

    int ret;

    uint8_t  in_buf[BUFFER_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t   data_size;

    const AVCodec        *codec;
    AVCodecParserContext *parser_context;
    AVCodecContext       *codec_context;
    AVFrame              *frame;
    AVPacket             *packet;

    FILE *file;

    codec          = avcodec_find_decoder(AV_CODEC_ID_H264);
    parser_context = av_parser_init(codec->id);
    codec_context  = avcodec_alloc_context3(codec);
    ret            = avcodec_open2(codec_context, codec, NULL);

    frame  = av_frame_alloc();
    packet = av_packet_alloc();

    file = fopen(argv[1], "rb");

    ret = glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "Window", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, glfw_key_callback);

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vert_source, NULL);
    glCompileShader(vert);
    GLint status;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &status);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &frag_source, NULL);
    glCompileShader(frag);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glUseProgram(prog);

    GLuint textureY, textureCb, textureCr;
    glGenTextures(1, &textureY);
    glGenTextures(1, &textureCr);
    glGenTextures(1, &textureCb);

    // clang-format off
    GLfloat vertices[] = {
        // positions    // texture coordinates
        -1.0,  1.0,     0.0, 0.0,
        -1.0, -1.0,     0.0, 1.0,
         1.0,  1.0,     1.0, 0.0,
         1.0, -1.0,     1.0, 1.0,
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    GLuint positionAttrib = glGetAttribLocation(prog, "position");
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);

    GLuint texCoordAttrib = glGetAttribLocation(prog, "texCoord");
    glEnableVertexAttribArray(texCoordAttrib);
    glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));

    glUniform1i(glGetUniformLocation(prog, "textureY" ), 0);
    glUniform1i(glGetUniformLocation(prog, "textureCb"), 1);
    glUniform1i(glGetUniformLocation(prog, "textureCr"), 2);

    do {
        glfwPollEvents();

        data_size = fread(in_buf, 1, sizeof(in_buf) - AV_INPUT_BUFFER_PADDING_SIZE, file);
        if (!data_size) break;

        memset(in_buf + data_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
        data = in_buf;

        while (data_size > 0) {
            ret = av_parser_parse2(parser_context, codec_context, &packet->data, &packet->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            data += ret;
            data_size -= ret;

            if (packet->size) {
                ret = avcodec_send_packet(codec_context, packet);

                while (ret >= 0) {
                    ret = avcodec_receive_frame(codec_context, frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

                    const char *format = av_get_pix_fmt_name(frame->format);
                    printf("number: %d - format = %s\n", codec_context->frame_number, format);

                    glClear(GL_COLOR_BUFFER_BIT);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, textureY);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width, frame->height, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, textureCb);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, textureCr);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, frame->width / 2, frame->height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                    glfwSwapBuffers(window);
                }
            }
        }
    } while (!feof(file) && !glfwWindowShouldClose(window));

    fclose(file);
    av_parser_close(parser_context);
    avcodec_free_context(&codec_context);
    av_frame_free(&frame);
    av_packet_free(&packet);
    glfwTerminate();

    return 0;
}
