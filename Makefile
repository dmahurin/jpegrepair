PNAME=jpegrepair
PROGS=$(PNAME)
SRCS=.
CC=gcc
CFLAGS=-Wall -I$(SRCS)
ifeq ($(OS),Windows_NT)
  LIBS=libjpeg-8.dll
else
  LIBS=-ljpeg
endif
PREFIX?=/usr/local
DATAPREFIX=$(PREFIX)/share
INSTALL=install
RM=rm -fv

.PHONY: all clean install wasm wasm-clean

all: $(PROGS)

$(PNAME): $(SRCS)/$(PNAME).c $(SRCS)/transupp.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -s

# WebAssembly build. libjpeg-turbo is built as a static WebAssembly library so
# no system libjpeg or prebuilt WebAssembly dependency is required.
WASM_BUILD_DIR?=build/wasm
WASM_DIST_DIR?=dist/wasm
LIBJPEG_TURBO_VERSION?=3.1.1
LIBJPEG_TURBO_ARCHIVE=$(WASM_BUILD_DIR)/libjpeg-turbo-$(LIBJPEG_TURBO_VERSION).tar.gz
LIBJPEG_TURBO_SOURCE=$(WASM_BUILD_DIR)/libjpeg-turbo-$(LIBJPEG_TURBO_VERSION)
LIBJPEG_TURBO_BUILD=$(WASM_BUILD_DIR)/libjpeg-turbo-build
LIBJPEG_TURBO_PREFIX=$(WASM_BUILD_DIR)/libjpeg-turbo-install
LIBJPEG_TURBO_URL?=https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/$(LIBJPEG_TURBO_VERSION)/libjpeg-turbo-$(LIBJPEG_TURBO_VERSION).tar.gz
EMCC?=emcc
EMCMAKE?=emcmake
WASM_CFLAGS?=-O2 -Wall
WASM_LDFLAGS?=-sALLOW_MEMORY_GROWTH=1 -sFORCE_FILESYSTEM=1 -sEXIT_RUNTIME=1 \
	-sINVOKE_RUN=0 -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createJpegRepairModule \
	-sEXPORTED_RUNTIME_METHODS=FS,callMain -sENVIRONMENT=web,worker,node

wasm: $(WASM_DIST_DIR)/$(PNAME).js

$(LIBJPEG_TURBO_ARCHIVE):
	mkdir -p $(WASM_BUILD_DIR)
	curl -fL --retry 3 -o $@ $(LIBJPEG_TURBO_URL)

$(LIBJPEG_TURBO_SOURCE)/CMakeLists.txt: $(LIBJPEG_TURBO_ARCHIVE)
	tar -xzf $< -C $(WASM_BUILD_DIR)
	touch $@

$(LIBJPEG_TURBO_PREFIX)/lib/libjpeg.a: $(LIBJPEG_TURBO_SOURCE)/CMakeLists.txt
	mkdir -p $(LIBJPEG_TURBO_BUILD) $(LIBJPEG_TURBO_PREFIX)
	cd $(LIBJPEG_TURBO_BUILD) && $(EMCMAKE) cmake $(abspath $(LIBJPEG_TURBO_SOURCE)) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=$(abspath $(LIBJPEG_TURBO_PREFIX)) \
		-DENABLE_SHARED=FALSE \
		-DENABLE_STATIC=TRUE \
		-DWITH_TOOLS=FALSE \
		-DWITH_TESTS=FALSE \
		-DWITH_SIMD=FALSE
	cmake --build $(LIBJPEG_TURBO_BUILD) --target install --parallel

$(WASM_DIST_DIR)/$(PNAME).js: $(SRCS)/$(PNAME).c $(SRCS)/transupp.c $(LIBJPEG_TURBO_PREFIX)/lib/libjpeg.a
	mkdir -p $(WASM_DIST_DIR)
	$(EMCC) $(WASM_CFLAGS) -I$(SRCS) -I$(LIBJPEG_TURBO_PREFIX)/include \
		-o $@ $(SRCS)/$(PNAME).c $(SRCS)/transupp.c \
		$(LIBJPEG_TURBO_PREFIX)/lib/libjpeg.a $(WASM_LDFLAGS)

clean:
	$(RM) $(PROGS)

wasm-clean:
	$(RM) -r $(WASM_BUILD_DIR) $(WASM_DIST_DIR)

install: $(PROGS)
	$(INSTALL) -d $(PREFIX)/bin
	$(INSTALL) -m 0755 $(PROGS) $(PREFIX)/bin/
	$(INSTALL) -d $(DATAPREFIX)
	$(INSTALL) -d $(DATAPREFIX)/man/man1
	$(INSTALL) -m 0644 man/man1/$(PNAME).1 $(DATAPREFIX)/man/man1
	$(INSTALL) -d $(DATAPREFIX)/doc/$(PNAME)
	$(INSTALL) -m 0644 LICENSE README.md README.ijg $(DATAPREFIX)/doc/$(PNAME)
