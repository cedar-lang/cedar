
.PHONY: clean install gen debug

BINDIR = bin



default:
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_DIR=${PWD} ../; $(MAKE) -j --no-print-directory

debug:
	@printf "DEBUG\n"
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_DIR=${PWD} ../; $(MAKE) -j --no-print-directory


gen:
	@python3 tools/scripts/generate_cedar_h.py
	@python3 tools/scripts/generate_src_cmakelists.py
	@python3 tools/scripts/generate_opcode_h.py

install:
	cd $(BINDIR); ninja install
	mkdir -p /usr/local/lib/cedar
	@rm -rf /usr/local/lib/cedar
	cp -r lib /usr/local/lib/cedar/
	cp -r include/ /usr/local/include/

clean:
	rm -rf $(BINDIR)
