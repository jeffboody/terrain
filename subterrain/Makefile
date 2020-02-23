TARGET   = subterrain
CLASSES  =
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
#OPT      = -g -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Lterrain -lterrain -Ltexgz -ltexgz -Llibcc -lcc -lm -lz
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) libcc terrain texgz
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: libcc terrain texgz

libcc:
	$(MAKE) -C libcc

terrain:
	$(MAKE) -C terrain

texgz:
	$(MAKE) -C texgz

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C libcc clean
	$(MAKE) -C terrain clean
	$(MAKE) -C texgz clean
	rm libcc terrain texgz

$(OBJECTS): $(HFILES)
