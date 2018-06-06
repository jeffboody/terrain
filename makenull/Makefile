TARGET   = makenull
CLASSES  =
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
#OPT      = -g -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Lterrain -lterrain -lm -lz
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) terrain
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: terrain

terrain:
	$(MAKE) -C terrain

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C terrain clean
	rm terrain

$(OBJECTS): $(HFILES)
