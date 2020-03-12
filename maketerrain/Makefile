TARGET   = maketerrain
CLASSES  = mk_state mk_object
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Llibcc -lcc -Lterrain -lterrain -Lflt -lflt -Llibxmlstream -lxmlstream -Llibexpat/expat/lib -lexpat -ltiff -lpthread -lm -lz
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) libcc libexpat libxmlstream terrain flt
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: libcc libexpat libxmlstream terrain flt

libcc:
	$(MAKE) -C libcc

libexpat:
	$(MAKE) -C libexpat/expat/lib

libxmlstream:
	$(MAKE) -C libxmlstream

terrain:
	$(MAKE) -C terrain

flt:
	$(MAKE) -C flt

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C libcc clean
	$(MAKE) -C libexpat/expat/lib clean
	$(MAKE) -C libxmlstream clean
	$(MAKE) -C terrain clean
	$(MAKE) -C flt clean
	rm libcc libexpat libxmlstream terrain flt

$(OBJECTS): $(HFILES)
