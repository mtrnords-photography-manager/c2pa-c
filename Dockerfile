FROM rust:1.70.0

RUN apt-get update
RUN apt-get install musl-tools clang llvm -y

RUN curl -fsSL https://get.docker.com -o get-docker.sh
RUN sh ./get-docker.sh

RUN useradd -m runner
USER runner

RUN cargo install cbindgen

RUN mkdir -p /home/runner
WORKDIR /home/runner

COPY . .

RUN make install-targets

# ENTRYPOINT [ "make", "build-targets" ]