PROGRAM = mandelbrot
all:    $(PROGRAM)
.PHONY: all

#DEBUG_INFO = YES

CFLAGS = -lm `sdl2-config --cflags --libs`

ifdef DEBUG_INFO
 CFLAGS += -g
else
 CFLAGS += -O3
endif

mandelbrot: mandelbrot.c
	$(CC) -o mandelbrot mandelbrot.c $(CFLAGS)

clean:
	find . -name '*~' -delete
	-rm -f $(PROGRAM)
