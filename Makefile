OS := $(shell uname)
CFLAGS = -I. -Wall 
ifeq ($(OS), Darwin)
CFLAGS += -framework Security
endif
ifeq ($(OS), Linux)
CFLAGS = -pthread -Wl,--no-as-needed -ldl -lm
endif

release: 
	cargo build --release
	cbindgen --config cbindgen.toml --crate c2pa-c --output include/c2pa.h --lang c

build-musl: SHELL := /bin/bash
build-musl:
	# compile musl dylib for ARM
	rustup target add aarch64-unknown-linux-musl
	TARGET_CC=clang TARGET_AR=llvm-ar RUSTFLAGS="-Ctarget-feature=-crt-static -Clink-self-contained=yes" cargo build --release --target aarch64-unknown-linux-musl

	# compile dylib for x86
	rustup target add x86_64-unknown-linux-gnu
	RUSTFLAGS="-Clinker=x86_64-linux-gnu-gcc" cargo build --release --target x86_64-unknown-linux-gnu
	
	cbindgen --config cbindgen.toml --crate c2pa-c --output include/c2pa.h --lang c; \
	
build-docker:
	mkdir -p dist
	docker run \
		-v dist:/home/runner/dist \
		--rm \
		-it $$(docker build -q .) \
		/bin/bash

test-c: release
	$(CC) $(CFLAGS) tests/test.c -o target/ctest -lc2pa_c -L./target/release
	target/ctest

test-cpp: release
	g++ $(CFLAGS) -std=c++11 tests/test.cpp -o target/cpptest -lc2pa_c -L./target/release 
	target/cpptest

example: release
	g++ $(CFLAGS) -std=c++11 examples/training.cpp -o target/training -lc2pa_c -L./target/release
	target/training

test: test-c test-cpp