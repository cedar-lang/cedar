
.PHONY: clean install gen debug

BINDIR = bin



default:
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -G "Ninja"  -DCMAKE_BUILD_TYPE=Release -DBUILD_DIR=${PWD} ../; ninja

debug:
	@printf "DEBUG\n"
	@mkdir -p $(BINDIR)
	@cd $(BINDIR); cmake -G "Ninja" -DBUILD_DIR=${PWD} -DCMAKE_BUILD_TYPE=Debug ../; ninja


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
