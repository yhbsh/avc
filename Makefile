CFLAGS   := $(shell pkg-config --cflags libavcodec libavformat libavdevice glfw3 openh264 x264)
LDFLAGS  := $(shell pkg-config --libs   libavcodec libavformat libavdevice glfw3 openh264 x264) -framework OpenGL

C_SRCS := $(wildcard *.c)
CPP_SRCS := $(wildcard *.cpp)
C_BINS := $(C_SRCS:.c=)
CPP_BINS := $(CPP_SRCS:.cpp=)
BINS := $(addprefix bin/,$(C_BINS) $(CPP_BINS))

all: bin $(BINS)

bin:
	mkdir -p bin

bin/%: %.c | bin
	cc $(CFLAGS) $< -o $@ $(LDFLAGS)

bin/%: %.cpp | bin
	c++ $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf bin

.PHONY: all clean
