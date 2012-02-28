MAKE=make

LINUX=linux
WINDOWS=windows

.PHONY: all $(WINDOWS) $(LINUX) clean
all: $(WINDOWS)

$(LINUX):
	$(MAKE) -f Makefile.$(LINUX)

$(WINDOWS):
	$(MAKE) -f Makefile.$(WINDOWS)

clean:
	$(MAKE) -f Makefile.$(LINUX) clean
