
.PHONY: clean install libinc

BUILDMODE=Release

BINDIR = bin

default: src/lib/std.inc.h
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Debug ../; ninja


release: src/lib/std.inc.h
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ../; ninja

src/lib/std.inc.h: ./src/lib/std.inc.cdr
	xxd -i src/lib/std.inc.cdr > src/lib/std.inc.h

gen:
	@python3 tools/scripts/generate_cedar_h.py
	@python3 tools/scripts/generate_src_cmakelists.py
	@python3 tools/scripts/generate_opcode_h.py

install:
	@cd $(BINDIR); ninja install

clean:
	rm -rf $(BINDIR)

