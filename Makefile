SUBDIRS=src examples

all: $(SUBDIRS)
clean: $(SUBDIRS)

examples: src

.PHONY: $(SUBDIRS) all clean

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
