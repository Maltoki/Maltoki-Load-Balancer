FROM ubuntu:latest

# Install dependencies
RUN apt update && apt install -y \
    cmake \
    g++ \
    ninja-build \
    make

# Set the working directory
WORKDIR /app

# Copy source files
COPY . .

# Create a build directory
RUN cmake -S . -B build -G Ninja && cmake --build build
