#
exe = bgg_read.exe

RM        = rm -f 
F77=f77

FFLAGS= -O2  
# CERNLIBS= -L $(CERN_ROOT)/lib -lpawlib -lgraflib -lgrafX11  -lpacklib -lkernlib -lmathlib

none: $(exe)

all: $(exe)

$(exe): bgg_read.f
	$(F77) $(FFLAGS) -o  $@ $^ #$(CERNLIBS)

clean: 
	$(RM) bgg_read.o $(exe)


