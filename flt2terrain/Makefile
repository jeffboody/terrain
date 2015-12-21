TARGET   = flt2terrain
CLASSES  =
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
#OPT      = -g -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Lterrain -lterrain -Lflt -lflt -Ltexgz -ltexgz -lm -lz
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) texgz terrain flt
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: texgz terrain flt

texgz:
	$(MAKE) -C texgz

terrain:
	$(MAKE) -C terrain

flt:
	$(MAKE) -C flt

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C texgz clean
	$(MAKE) -C terrain clean
	$(MAKE) -C flt clean
	rm texgz terrain flt

$(OBJECTS): $(HFILES)
