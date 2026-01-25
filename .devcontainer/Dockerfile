FROM debian:stable-slim

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    gdb \
    git \
    telnet \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace