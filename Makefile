OS := $(shell uname)
CFLAGS = -I. -Wall 
ifeq ($(OS), Darwin)
CFLAGS += -framework Security
endif
ifeq ($(OS), Linux)
CFLAGS = -pthread -Wl,--no-as-needed -ldl -lm
endif

install-targets: SHELL := /bin/bash
install-targets:
	while IFS= read -r line || [[ -n "$$line" ]]; do \
		rustup target add $$line; \
	done < targets.txt

release: 
	cargo build --release
	cbindgen --config cbindgen.toml --crate c2pa-c --output include/c2pa.h --lang c

build-targets: SHELL := /bin/bash
build-targets:
	while IFS= read -r line || [[ -n "$$line" ]]; do \
		if [[ $$line == *-musl ]]; then \
			echo "Building for musl"; \
			CC_aarch64_unknown_linux_musl=clang \
			AR_aarch64_unknown_linux_musl=llvm-ar \
			CARGO_TARGET_AARCH64_UNKNOWN_LINUX_MUSL_RUSTFLAGS="-Clink-self-contained=yes -Clinker=rust-lld" \
			RUSTFLAGS="-C target-feature=-crt-static" \
			cargo build --release --target $$line; \
		else \
			echo "Non-musl build"; \
			cargo build --release --target $$line; \
		fi; \
		cbindgen --config cbindgen.toml --crate c2pa-c --output include/c2pa.h --lang c; \
	done < targets.txt
	
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