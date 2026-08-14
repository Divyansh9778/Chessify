FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
    g++ \
    cmake \
    make \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S server -B server/build \
    -DCMAKE_BUILD_TYPE=Release

RUN cmake --build server/build --config Release

CMD ["sh", "-c", "echo '[DOCKER] Starting ChessifyServer'; echo '[DOCKER] Current directory:'; pwd; echo '[DOCKER] Binary:'; ls -lh ./server/build/ChessifyServer; echo '[DOCKER] Dependencies:'; ldd ./server/build/ChessifyServer; echo '[DOCKER] Executing server...'; exec ./server/build/ChessifyServer"]