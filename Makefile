CC = clang
CXX = clang++
WARNINGS = -Wall -Wextra -Wno-deprecated-declarations -Wno-unused -Wno-ignored-qualifiers
CFLAGS = -flto -I./include -O3
CXXLDLIBS = -flto -std=c++17 -O3 -pthread -lffi -lc

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	SHAREDARGS = -dynamiclib -flat_namespace
endif
ifeq ($(UNAME), Linux)
	SHAREDARGS = -fPIC -shared
	CXXLDLIBS += -ldl
	CFLAGS += -fPIC
	CC = gcc
	CXX = g++
endif

objs = $(srcs:.cc=.o)
includes = $(wildcard include/*)

exe = bin/cedar

SRCDIR = src
OBJDIR = build

SOURCEFILES := $(addsuffix /*,$(shell find $(SRCDIR) -type d))
CODEFILES := $(wildcard $(SOURCEFILES))

CXXSRCFILES := $(filter %.cc,$(CODEFILES))
CXXOBJFILES := $(subst $(SRCDIR),$(OBJDIR)/cc,$(CXXSRCFILES:%.cc=%.o))


CSRCFILES := $(filter %.c,$(CODEFILES))
COBJFILES := $(subst $(SRCDIR),$(OBJDIR)/c,$(CSRCFILES:%.c=%.o))


.PHONY: all clean gen lib install test

default:
	@$(MAKE) -j 8 all

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

build/libcedar.so: $(CXXOBJFILES) $(COBJFILES)
	@printf " SO\t$@\n"
	@$(CXX) $(SHAREDARGS) $(CXXLDLIBS) $(WARNINGS) -o $@ $(CXXOBJFILES) $(COBJFILES)


# the main file must be compiled seperately because a cedar shared
# object file will be compiled in future. Seperating this out makes
# it slightly easier
build/main.o: main.cc
	@printf " CXX\t$@\n"
	@$(CXX) $(WARNINGS) $(CFLAGS) -g -c main.cc -o build/main.o


$(exe): build/main.o $(CXXOBJFILES) $(COBJFILES)
	@printf " LD\t$@\n"
	@mkdir -p bin
	@$(CXX) $(CXXLDLIBS) $(WARNINGS) -g -o $@ build/main.o $(CXXOBJFILES) $(COBJFILES)


clean:
	@rm -rf $(OBJDIR)
	@rm -rf bin
	@rm -rf lib/stdbind.so*
	@rm -rf testbuild
	@rm -rf test/test
	@rm -rf test/build
	@rm -rf test/*.o

lib:
	@rm -rf /usr/local/lib/cedar
	@mkdir -p /usr/local/lib/cedar
	@cp -a lib/ /usr/local/lib/cedar
	@mkdir -p /usr/local/include/cedar
	@cp -a include/ /usr/local/include/cedar


install:
	cp -a ./include/. /usr/local/include/
	cp -a ./build/libcedar.so /usr/local/lib/
	cp -a ./cedar /usr/local/bin/


test: build/libcedar.so
	@cd test; $(MAKE)
	@test/test
