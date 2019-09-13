CFLAGS+=-W -Wall -Wextra -O2 -I../../include/ `gfxprim-config --cflags` -ggdb -rdynamic
LDFLAGS+=-L../../src/
LDLIBS=`gfxprim-config --libs --libs-loaders --libs-backends` -lgfxprim-widgets -ldl
BIN=example_login
DEP=$(BINS:=.dep)

all: $(DEP) $(BIN)

%.dep: %.c
	$(CC) $(CFLAGS) -M $< -o $@

-include $(DEP)

clean:
	rm -f $(BIN) *.dep *.o
