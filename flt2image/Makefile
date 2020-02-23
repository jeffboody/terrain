TARGET   = flt2image
CLASSES  =
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Lterrain -lterrain -Lflt -lflt -Llibxmlstream -lxmlstream -Llibexpat/expat/lib -lexpat -ltiff -Ltexgz -ltexgz -Llibcc -lcc -lpng -lm -lz
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) libcc libexpat libxmlstream texgz terrain flt
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: libcc libexpat libxmlstream texgz terrain flt

libcc:
	$(MAKE) -C libcc

libexpat:
	$(MAKE) -C libexpat/expat/lib

libxmlstream:
	$(MAKE) -C libxmlstream

texgz:
	$(MAKE) -C texgz

terrain:
	$(MAKE) -C terrain

flt:
	$(MAKE) -C flt

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C libcc clean
	$(MAKE) -C libexpat/expat/lib clean
	$(MAKE) -C libxmlstream clean
	$(MAKE) -C texgz clean
	$(MAKE) -C terrain clean
	$(MAKE) -C flt clean
	rm libcc libexpat libxmlstream texgz terrain flt

$(OBJECTS): $(HFILES)
