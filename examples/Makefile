CFLAGS+=-W -Wall -I../include/ `gfxprim-config --cflags` -ggdb
LDFLAGS+=-L../src/
LDLIBS=`gfxprim-config --libs --libs-loaders --libs-backends` -lgfxprim-widgets -ldl -ljson-c
BINS=test t0 t3 t4 t5 test_login test_pixmap show_layout #sysinfo t1 t2 imp
DEP=$(BINS:=.dep)

all: $(DEP) $(BINS)

%.dep: %.c
	$(CC) $(CFLAGS) -M $< -o $@

-include $(DEP)

test.dep: test.h
sysinfo.dep: sysinfo.h
t0.dep: t0.h
t4.dep: t4.h
t5.dep: t5.h
imp.dep: imp.h

test: test.o
sysinfo: sysinfo.o
t0: t0.o
t1: t1.o
t2: t2.o
t3: t3.o
t4: t4.o
t5: t5.o
imp: imp.o
test_login: test_login.o
test_pixmap: test_pixmap.o
show_layout: show_layout.o

test: LDFLAGS+=-rdynamic
test_login: LDFLAGS+=-rdynamic
test_pixmap: LDFLAGS+=-rdynamic
show_layout: LDFLAGS+=-rdynamic
t0: LDFLAGS+=-rdynamic
t4: LDFLAGS+=-rdynamic

%.h: %.wl
	./gpwl $^ > $@

clean:
	rm -f $(BINS) *.dep *.h *.o
