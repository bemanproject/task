.PHONY: default doc

default: doc

doc: P3552-task.html issues.html

wg21/Makefile:
	git clone https://github.com/mpark/wg21.git

distclean: clean
	$(RM) -r generated
	$(RM) mkerr olderr

include wg21/Makefile
