TARGET   = subterrain
CLASSES  =
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
#OPT      = -g -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Lterrain -lterrain -Ltexgz -ltexgz -lm -lz
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) terrain texgz
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: terrain texgz

terrain:
	$(MAKE) -C terrain

texgz:
	$(MAKE) -C texgz

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C terrain clean
	$(MAKE) -C texgz clean
	rm terrain texgz

$(OBJECTS): $(HFILES)
