TARGET   = pak
CLASSES  =
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  = -Llibpak -lpak -Llibcc -lcc
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) libcc libpak
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: libcc libpak

libcc:
	$(MAKE) -C libcc

libpak:
	$(MAKE) -C libpak

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C libcc clean
	$(MAKE) -C libpak clean
	rm libcc libpak

$(OBJECTS): $(HFILES)
