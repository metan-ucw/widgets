CFLAGS+=-W -Wall -Wextra -O2 -I../../include/ `gfxprim-config --cflags` -ggdb
LDFLAGS+=-L../../src/ -rdynamic
LDLIBS=`gfxprim-config --libs --libs-loaders --libs-backends` -lgfxprim-widgets -lgps -ldl
BIN=gpsd-test
DEP=$(BINS:=.dep)

all: $(DEP) $(BIN)

%.dep: %.c
	$(CC) $(CFLAGS) -M $< -o $@

-include $(DEP)

clean:
	rm -f $(BIN) *.dep *.o
