FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y \
    g++ \
    cmake \
    make \
    libboost-all-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S server -B server/build \
    -DCMAKE_BUILD_TYPE=Release

RUN cmake --build server/build --config Release

CMD ["./server/build/ChessifyServer"]