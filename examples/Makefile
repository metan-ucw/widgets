CFLAGS+=-W -Wall -Wextra -O2 -I../include/ `gfxprim-config --cflags` -ggdb
LDFLAGS+=-L../src/
LDLIBS=`gfxprim-config --libs --libs-loaders --libs-backends` -lgfxprim-widgets -ldl
BINS=test t0 t3 t4 t5 test_login test_pixmap show_layout imp sysinfo #t1 t2
DEP=$(BINS:=.dep)
SUBDIRS=disk_free login calc

all: $(DEP) $(BINS) $(SUBDIRS)
clean: $(SUBDIRS)

.PHONY: $(SUBDIRS) all clean

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

%.dep: %.c
	$(CC) $(CFLAGS) -M $< -o $@

-include $(DEP)

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

sysinfo: LDFLAGS+=-rdynamic
test: LDFLAGS+=-rdynamic
test_login: LDFLAGS+=-rdynamic
test_pixmap: LDFLAGS+=-rdynamic
show_layout: LDFLAGS+=-rdynamic
t0: LDFLAGS+=-rdynamic
t4: LDFLAGS+=-rdynamic

clean:
	rm -f $(BINS) *.dep *.h *.o
