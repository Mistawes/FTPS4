LIBPS4	:= $(PS4SDK)/libPS4

TEXT	:=	0x926200000
DATA	:=	0x926300000

CC		:= gcc
AS		:=	gcc
OBJCOPY	:= objcopy
ODIR	:= build
SDIR	:= source
IDIRS	:= -I$(LIBPS4)/include -I. -Iinclude
LDIRS	:= -L$(LIBPS4) -L. -Llib
MAPFILE := $(shell basename $(CURDIR)).map
CFLAGS	:= $(IDIRS) -O2 -Os -std=gnu11 -ffunction-sections -fdata-sections -fno-builtin -nostartfiles -nostdlib -Wall -masm=intel -march=btver2 -mtune=btver2 -m64 -mabi=sysv -mcmodel=small -fpie -DTEXT_ADDRESS=$(TEXT) -DDATA_ADDRESS=$(DATA)
SFLAGS	:=	-nostartfiles -nostdlib -march=btver2 -mtune=btver2
LFLAGS	:= $(LDIRS) -Xlinker -T $(LIBPS4)/linker.x -Xlinker -Map=$(MAPFILE) -Wl,--build-id=none -Wl,--build-id=none -Ttext=$(TEXT) -Tdata=$(DATA) -Wl,--gc-sections
CFILES	:= $(wildcard $(SDIR)/*.c)
SFILES	:= $(wildcard $(SDIR)/*.s)
OBJS	:= $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES)) $(patsubst $(SDIR)/%.s, $(ODIR)/%.o, $(SFILES))

LIBS	:= -lPS4

#TARGET = $(shell basename $(CURDIR)).bin
TARGET = payload.bin

$(TARGET): $(ODIR) $(OBJS)
	$(CC) $(LIBPS4)/crt0.s $(ODIR)/*.o -o temp.t $(CFLAGS) $(LFLAGS) $(LIBS)
	$(OBJCOPY) -O binary temp.t $(TARGET)
	rm -f temp.t

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(SDIR)/%.s
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR):
	@mkdir $@

.PHONY: clean

clean:
	rm -rf $(TARGET) $(MAPFILE) $(ODIR)
