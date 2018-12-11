CC = clang
CXX = clang++
WARNINGS = -Wall -Wformat -Wno-unused-command-line-argument -Wno-deprecated-declarations -Wno-unused
CFLAGS = -I./include -g -O3
CXXLDLIBS = -std=c++17 -O3 -pthread -lc -lgc

objs = $(srcs:.cc=.o)
includes = $(wildcard include/*)

exe = cedar

SRCDIR = src
OBJDIR = build

SOURCEFILES := $(addsuffix /*,$(shell find $(SRCDIR) -type d))
CODEFILES := $(wildcard $(SOURCEFILES))

CXXSRCFILES := $(filter %.cc,$(CODEFILES))
CXXOBJFILES := $(subst $(SRCDIR),$(OBJDIR)/cc,$(CXXSRCFILES:%.cc=%.o))


CSRCFILES := $(filter %.c,$(CODEFILES))
COBJFILES := $(subst $(SRCDIR),$(OBJDIR)/c,$(CSRCFILES:%.c=%.o))


.PHONY: all clean gen lib

default:
	@$(MAKE) -j $(shell getconf _NPROCESSORS_ONLN) all

compile: $(OBJDIR) $(exe)

all: compile

$(OBJDIR):
	@mkdir -p $@

$(OBJDIR)/c/%.o: $(addprefix $(SRCDIR)/,%.c) ${includes}
	@printf " CC\t$@\n"
	@mkdir -p $(dir $@)
	@$(CC) $(WARNINGS) $(CFLAGS) -c $< -o $@


$(OBJDIR)/cc/%.o: $(addprefix $(SRCDIR)/,%.cc) ${includes}
	@printf " CXX\t$@\n"
	@mkdir -p $(dir $@)
	@$(CXX) $(WARNINGS) $(CFLAGS) -c $< -o $@

$(exe): $(CXXOBJFILES) $(COBJFILES)
	@printf " LD\t$@\n"
	@$(CXX) $(CXXLDLIBS) $(WARNINGS) -o $@ $(foreach i,$^,$(i) )

clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(exe)
	@rm -rf lib/stdbind.so*

lib: lib/runtime.wl lib/stdbind.so
	@rm -rf /usr/local/lib/cedar
	@mkdir -p /usr/local/lib/cedar
	@cp -a lib/ /usr/local/lib/cedar
	@mkdir -p /usr/local/include/cedar
	@cp -a include/ /usr/local/include/cedar
