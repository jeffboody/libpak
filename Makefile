TARGET   = libpak.a
CLASSES  = pak_file
SOURCE   = $(CLASSES:%=%.c)
OBJECTS  = $(SOURCE:.c=.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall
CFLAGS   = $(OPT) -I.
LDFLAGS  =
AR       = ar

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)

$(OBJECTS): $(HFILES)
