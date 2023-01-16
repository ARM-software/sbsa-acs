
default: install

install:
	$(PYTHON) setup.py install  --prefix=$(CROSSBASE)

sysinstall:
	sudo $(PYTHON) setup.py install

.PHONY: clean
clean:
	rm -rf build
