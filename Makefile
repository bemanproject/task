#-dk: note to self: PATH=/opt/llvm-19.1.6/bin:$PATH LDFLAGS=-fuse-ld=lld

.PHONY: config test default compile clean distclean doc html pdf format clang-format tidy

BUILDDIR = build
PRESET  = gcc-release
UNAME = $(shell uname -s)
ifeq ($(UNAME),Darwin)
    PRESET  = appleclang-release
endif
BUILD = $(BUILDDIR)/$(PRESET)

default: compile

doc:
	cd docs; $(MAKE)

pdf html:
	cd docs; $(MAKE) doc-$@

compile:
	CMAKE_EXPORT_COMPILE_COMMANDS=1 \
	cmake \
	  --workflow --preset=$(PRESET) \

list:
	cmake --workflow --list-presets

clang-format format:
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
