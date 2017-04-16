SYSTEMC_HOME = /usr/local/systemc-2.3.1
SYSTEMC_INC_DIR = $(SYSTEMC_HOME)/include
SYSTEMC_LIB_DIR = $(SYSTEMC_HOME)/lib/x86_64-linux-gnu

SRCDIR := src
OBJDIR := obj
MAIN := $(SRCDIR)/Main.cpp
SRCS := $(filter-out $(MAIN) $(SRCDIR)/Gem5Wrapper.cpp, $(wildcard $(SRCDIR)/*.cpp))
OBJS := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))


# Ramulator currently supports g++ 5.1+ or clang++ 3.4+.  It will NOT work with
#   g++ 4.x due to an internal compiler error when processing lambda functions.
#CXX := clang++
CXX := g++-5
CFLAGS = -std=c++11 -g -Wall -pedantic -Wno-long-long \
		 -DSC_INCLUDE_DYNAMIC_PROCESSES -fpermissive \
		 -I$(SYSTEMC_INC_DIR) 

LDFLAGS =-L$(SYSTEMC_LIB_DIR) -lsystemc -lm

#CXXFLAGS := -O3 -std=c++11 -g -Wall
#CXXFLAGS := --std=c++11 -g -Wall
.PHONY: all clean depend

all: depend ramulator

clean:
	rm -f ramulator
	rm -rf $(OBJDIR)

depend: $(OBJDIR)/.depend

exe:
	./ramulator configs/DDR3-config.cfg --mode=acc dram.trace

gdb:
	gdb --args ./ramulator configs/DDR3-config.cfg --mode=acc dram.trace

$(OBJDIR)/.depend: $(SRCS)
	@mkdir -p $(OBJDIR)
	@rm -f $(OBJDIR)/.depend
	@$(foreach SRC, $(SRCS), $(CXX) $(CFLAGS) -DRAMULATOR -MM -MT $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC)) $(SRC) >> $(OBJDIR)/.depend ;)

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJDIR)/.depend
endif


ramulator: $(MAIN) $(OBJS) $(SRCDIR)/*.h | depend
	$(CXX) $(CFLAGS) -DRAMULATOR -o $@ $(MAIN) $(OBJS) $(LDFLAGS)

libramulator.a: $(OBJS) $(OBJDIR)/Gem5Wrapper.o
	libtool -static -o $@ $(OBJS) $(OBJDIR)/Gem5Wrapper.o

$(OBJS): | $(OBJDIR)

$(OBJDIR): 
	@mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CFLAGS) -DRAMULATOR -c -o $@ $<
