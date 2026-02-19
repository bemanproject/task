#-dk: note to self: PATH=/opt/llvm-19.1.6/bin:$PATH LDFLAGS=-fuse-ld=lld

.PHONY: config test default install clean distclean doc docs html pdf format clang-format tidy

BUILDDIR = build
PRESET = gcc-release
CXX := g++
UNAME = $(shell uname -s)
ifeq ($(UNAME),Darwin)
    # PRESET = appleclang-release
    PRESET = gcc-release
endif
BUILD = $(BUILDDIR)/$(PRESET)

default: $(BUILD)/compile_commands.json
	cmake --workflow --preset=$(PRESET)
	cmake --install $(BUILD) --prefix=$(BUILDDIR)/statedir

docs doc:
	cd docs; $(MAKE)

pdf html:
	cd docs; $(MAKE) doc-$@

$(BUILD)/compile_commands.json: Makefile CMakePresets.json
	cmake --preset=$(PRESET) --log-level=VERBOSE
	ln -sf $(@) .

list:
	cmake --workflow --list-presets

format:
	pre-commit run --all

clang-format:
	git clang-format main

$(BUILDDIR)/tidy/compile_commands.json: Makefile CMakeLists.txt
	CC=$(CXX) cmake -G Ninja -S . -B $(BUILDDIR)/tidy \
	  -D CMAKE_EXPORT_COMPILE_COMMANDS=1 \
      --fresh --log-level=VERBOSE
	ln -sf $(@) .

tidy: $(BUILDDIR)/tidy/compile_commands.json
	run-clang-tidy -p $(BUILDDIR)/tidy tests examples

test: $(BUILDDIR)
	cd $(BUILDDIR); ctest

clean:
	cmake --build $(BUILD) --target clean

distclean:
	$(RM) -r $(BUILD) $(BUILDDIR)
