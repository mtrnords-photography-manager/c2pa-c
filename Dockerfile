FROM rust:1.70.0

RUN curl -fsSL https://get.docker.com -o get-docker.sh
RUN sh ./get-docker.sh

ENV CROSS_CONTAINER_IN_CONTAINER=true
ENV CFLAGS="-pthread -Wl,--no-as-needed -ldl -lm"

# RUN useradd -m runner
# USER runner

RUN cargo install cbindgen
RUN cargo install cross --git https://github.com/cross-rs/cross

WORKDIR /home/runner

COPY . .

# RUN make install-targets

# ENTRYPOINT [ "make", "build-targets" ]