SRCDIR = src

SOURCEFILES := $(addsuffix /*,$(shell find $(SRCDIR) -type d))
CODEFILES := $(wildcard $(SOURCEFILES))
CXXSRCFILES := $(filter %.cc,$(CODEFILES))

.PHONY: clean install libinc

BINDIR = bin

default: src/lib/std.inc.h
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -DCMAKE_BUILD_TYPE=Release ../; make --no-print-directory


src/lib/std.inc.h: ./src/lib/std.inc.cdr
	xxd -i src/lib/std.inc.cdr > src/lib/std.inc.h

gen:
	@python3 tools/scripts/generate_src_cmakelists.py
	@python3 tools/scripts/generate_opcode_h.py

install:
	@cd $(BINDIR); cmake ../; make install

clean:
	rm -rf $(BINDIR)
