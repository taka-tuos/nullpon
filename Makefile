TARGET		= nullpon
OBJS_TARGET	= main.o ximage.o nvbdflib.o

CFLAGS = -g `pkg-config sdl --cflags`
LDFLAGS =
LIBS = `pkg-config sdl --libs` -lm

include Makefile.in
