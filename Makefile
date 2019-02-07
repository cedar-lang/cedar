
.PHONY: clean install libinc

BUILDMODE=Release

BINDIR = bin



debug:
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -G "Ninja"  -DCORE_DIR=$(shell pwd)/core -DCMAKE_BUILD_TYPE=Debug ../; ninja

release:
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -G "Ninja" -DCORE_DIR=$(shell pwd)/core -DCMAKE_BUILD_TYPE=Release ../; ninja

default:
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -G "Ninja" -DCORE_DIR=/usr/local/lib/cedar/core -DCMAKE_BUILD_TYPE=Release ../; ninja

gen:
	@python3 tools/scripts/generate_cedar_h.py
	@python3 tools/scripts/generate_src_cmakelists.py
	@python3 tools/scripts/generate_opcode_h.py

install:
	cd $(BINDIR); ninja install
	mkdir -p /usr/local/lib/cedar
	@rm -rf /usr/local/lib/cedar/core
	cp -r core /usr/local/lib/cedar/

clean:
	rm -rf $(BINDIR)

