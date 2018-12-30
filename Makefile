SRCDIR = src

SOURCEFILES := $(addsuffix /*,$(shell find $(SRCDIR) -type d))
CODEFILES := $(wildcard $(SOURCEFILES))
CXXSRCFILES := $(filter %.cc,$(CODEFILES))

.PHONY: clean install

BINDIR = bin

default:
	@# tell cmake what files to compile with by generating
	@# the CMakeLists.txt in the src/cedar folder
	@python3 tools/scripts/generate_src_cmakelists.py
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake ../; make -j 8


install:
	@cd $(BINDIR); cmake ../; make install

clean:
	rm -rf $(BINDIR)
