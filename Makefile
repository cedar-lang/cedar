SRCDIR = src

SOURCEFILES := $(addsuffix /*,$(shell find $(SRCDIR) -type d))
CODEFILES := $(wildcard $(SOURCEFILES))
CXXSRCFILES := $(filter %.cc,$(CODEFILES))

.PHONY: clean install

BINDIR = bin

default:
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -DCMAKE_BUILD_TYPE=Debug ../; make -j 4 --no-print-directory


gen:
	@python3 tools/scripts/generate_src_cmakelists.py
	@python3 tools/scripts/generate_opcode_h.py

install:
	@cd $(BINDIR); cmake ../; make install

clean:
	rm -rf $(BINDIR)
