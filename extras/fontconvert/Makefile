all: fontconvert

CC     = gcc
# We need to add the path to the adafruit-gfx library here, conveniently
# that is also available in a subdirectory.
CFLAGS = -Wall -I/usr/local/include/freetype2 -I/usr/include/freetype2 -I/usr/include -I../../src/libs/Adafruit-GFX-Library
LIBS   = -lfreetype

fontconvert: fontconvert.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@
	strip $@

clean:
	rm -f fontconvert
