TARGET   = rebuild
CLASSES  =
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Lterrain -lterrain -Ltexgz -ltexgz -lm -lz
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) texgz terrain
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: texgz terrain

texgz:
	$(MAKE) -C texgz

terrain:
	$(MAKE) -C terrain

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C terrain clean
	$(MAKE) -C texgz clean

$(OBJECTS): $(HFILES)
