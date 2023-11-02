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
	@while IFS= read -r line || [[ -n "$$line" ]]; do \
		rustup target add $$line; \
	done < targets.txt

release: 
	cargo build --release
	cbindgen --config cbindgen.toml --crate c2pa-c --output include/c2pa.h --lang c

build-targets: SHELL := /bin/bash
build-targets:
	@while IFS= read -r line || [[ -n "$$line" ]]; do \
		cargo build --release --target $$line; \
		cbindgen --config cbindgen.toml --crate c2pa-c --output include/c2pa.h --lang c; \
	done < targets.txt

build-musl: 
	cross build --release --target aarch64-unknown-linux-musl
	cbindgen --config cbindgen.toml --crate c2pa-c --output include/c2pa.h --lang c

build-docker:
	mkdir -p dist
	docker run \
		-v /var/run/docker.sock:/var/run/docker.sock \
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