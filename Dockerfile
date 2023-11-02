FROM rust:1.70.0

RUN apt-get update
RUN apt-get install -y musl-tools clang llvm g++-x86-64-linux-gnu

# RUN useradd -m runner
# USER runner

RUN cargo install cbindgen

RUN mkdir -p /home/runner
WORKDIR /home/runner

COPY . .

# ENTRYPOINT [ "make", "build-targets" ]