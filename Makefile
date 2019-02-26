
.PHONY: clean install libinc gen

BUILDMODE=Release

BINDIR = bin



debug:
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Debug ../; ninja

default:
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ../; ninja

gen:
	@python3 tools/scripts/generate_cedar_h.py
	@python3 tools/scripts/generate_src_cmakelists.py
	@python3 tools/scripts/generate_opcode_h.py

install:
	cd $(BINDIR); ninja install
	mkdir -p /usr/local/lib/cedar
	@rm -rf /usr/local/lib/cedar/core
	cp -r core /usr/local/lib/cedar/
	cp -r include/ /usr/local/include/

clean:
	rm -rf $(BINDIR)

