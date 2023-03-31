#PYTHON=python

default: install
install:
	$(PYTHON) setup.py install --prefix=$(CROSSBASE)

test_denormals:
	$(CC) src/denormals.c -pedantic -Wall -Werror -O2 -DCOMPILE_DENORMALS_AS_MAIN
	./a.out
	$(CXX) src/denormals.c -pedantic -Wall -Werror -DCOMPILE_DENORMALS_AS_MAIN
	./a.out
	rm a.out

template:
	$(CC) -O2 $(COPTS) tests/code_template.c -c -o template.o
	objdump -d template.o

.PHONY: clean
clean:
	rm -rf build template.o

