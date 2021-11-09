all:
	python setup.py build

install:
	python setup.py install

devel: all kapow.so

kapow.so:
	ln -s build/lib.linux-`uname -m`-`python -V 2>&1 | cut -d ' ' -f 2 | cut -d . -f 1-2`/kapow.so kapow.so

clean:
	python setup.py clean

test:
	python -m doctest kapow.test && echo Passed.

.PHONY: all install devel clean test
