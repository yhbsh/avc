CFLAGS   := $(shell pkg-config --cflags libavcodec libavformat libavdevice glfw3 openh264 x264 x265)
LDFLAGS  := $(shell pkg-config --libs   libavcodec libavformat libavdevice glfw3 openh264 x264 x265) -framework OpenGL

C_SRCS := $(wildcard *.c)
C_BINS := $(C_SRCS:.c=)
BINS := $(addprefix bin/,$(C_BINS))

all: bin $(BINS)

bin:
	mkdir -p bin

bin/%: %.c | bin
	cc $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf bin

.PHONY: all clean
