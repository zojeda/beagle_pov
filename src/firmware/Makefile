CROSS_COMPILE?=

LIBDIR_APP_LOADER?=../../app_loader/lib
INCDIR_APP_LOADER?=../../app_loader/include
BINDIR?=./bin

CFLAGS+= -Wall -I$(INCDIR_APP_LOADER) -D__DEBUG -O2 -mtune=cortex-a8 -march=armv7-a
LDFLAGS+=-L$(LIBDIR_APP_LOADER) -lprussdrv
OBJDIR=obj
TARGET=beagle_pov

PASM?=pasm

ASSEM_FILES=$(wildcard *.p)

all: firmware $(TARGET)


firmware:
# Pass PRU assembly code for each example through assembler
	for a_file in ${ASSEM_FILES} ; \
	do \
          ${PASM} -V3 -b $$a_file ; \
	done ; \
	mv *.bin ${BINDIR}

_DEPS =
DEPS = $(patsubst %,$(INCDIR_APP_LOADER)/%,$(_DEPS))

_OBJ = beagle_pov.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))


$(OBJDIR)/%.o: %.c $(DEPS)
	@mkdir -p obj
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CROSS_COMPILE)gcc $(CFLAGS) -o $(BINDIR)/$@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf $(OBJDIR)/ *~  $(INCDIR_APP_LOADER)/*~  $(BINDIR)
