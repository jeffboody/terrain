TARGET   = libflt.a
CLASSES  = flt_tile
SOURCE   = $(CLASSES:%=%.c)
OBJECTS  = $(SOURCE:.c=.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Llibxmlstream -lxmlstream -Llibexpat/expat/lib -lexpat -Llibcc -lcc -lm
AR       = ar

all: $(TARGET)

$(TARGET): $(OBJECTS) libcc xmlstream libexpat
	$(AR) rcs $@ $(OBJECTS)

.PHONY: libcc xmlstream libexpat

libcc:
	$(MAKE) -C libcc

xmlstream:
	$(MAKE) -C libxmlstream

libexpat:
	$(MAKE) -C libexpat/expat/lib

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C libcc clean
	$(MAKE) -C libxmlstream clean
	$(MAKE) -C libexpat/expat/lib clean
	rm libcc libxmlstream libexpat

$(OBJECTS): $(HFILES)
