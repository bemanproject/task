#-dk: note to self: PATH=/opt/llvm-19.1.6/bin:$PATH LDFLAGS=-fuse-ld=lld

.PHONY: config test default compile clean distclean doc format tidy

BUILDDIR = build
PRESET  = gcc-debug
UNAME = $(shell uname -s)
ifeq ($(UNAME),Darwin)
    PRESET  = appleclang-release
endif
BUILD = $(BUILDDIR)/$(PRESET)

default: doc

doc:
	cd docs; $(MAKE)

compile:
	CMAKE_EXPORT_COMPILE_COMMANDS=1 \
	cmake \
	  --workflow --preset=$(PRESET) \

list:
	cmake --workflow --list-presets

format:
	git clang-format main

$(BUILDDIR)/tidy/compile_commands.json:
	CC=$(CXX) cmake --fresh -G Ninja -B  $(BUILDDIR)/tidy \
	  -D CMAKE_EXPORT_COMPILE_COMMANDS=1 \
          .

tidy: $(BUILDDIR)/tidy/compile_commands.json
	run-clang-tidy -p $(BUILDDIR)/tidy tests examples

test: compile
	cd $(BUILDDIR); ctest

clean:
	cmake --build $(BUILD) --target clean

distclean:
	$(RM) -r $(BUILDDIR)
