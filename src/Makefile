CFLAGS+=-W -Wall -Wextra -O2 -ggdb -fPIC -I../include/ `gfxprim-config --cflags`
LDLIBS=`gfxprim-config --libs --libs-loaders --libs-backends` -ldl -ljson-c
SRC=$(shell echo gp_*.c)
OBJ=$(SRC:.c=.o)
LIBNAME=libgfxprim-widgets.so
SONAME=$(LIBNAME).1
LIBRARY=$(LIBNAME).1.0.0
LDFLAGS=--shared -Wl,-soname -Wl,$(SONAME)

all: $(LIBRARY)

$(LIBRARY): $(OBJ)
	gcc $(LDFLAGS) $^ $(LDLIBS) -o $@
	[ -e $(SONAME) ] || ln -s $(LIBRARY) $(SONAME)
	[ -e $(LIBNAME) ] || ln -s $(LIBRARY) $(LIBNAME)

clean:
	rm -f $(OBJ) $(LIBRARY) $(SONAME) $(LIBNAME)
