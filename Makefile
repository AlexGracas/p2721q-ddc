CC ?= clang
CFLAGS ?= -O2 -Wall
PREFIX ?= /usr/local

p2721q: p2721q.c
	$(CC) $(CFLAGS) -o $@ $< -framework IOKit -framework CoreFoundation

install: p2721q
	install -d $(PREFIX)/bin
	install -m 755 p2721q $(PREFIX)/bin/p2721q

pkg:
	./scripts/build-pkg.sh

clean:
	rm -rf p2721q build

.PHONY: install pkg clean
