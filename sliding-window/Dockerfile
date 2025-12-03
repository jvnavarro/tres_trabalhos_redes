# Use Ubuntu as base image
FROM ubuntu:22.04

# Install g++ and necessary build tools
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY common.h .
COPY client/client.cpp client/
COPY server/server.cpp server/

# Compile the server and client
RUN g++ -o server server/server.cpp -pthread && \
    g++ -o client client/client.cpp -pthread

# Expose the port used by the server
EXPOSE 8080/udp

# Default command (can be overridden)
CMD ["./server"]
