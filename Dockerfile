FROM ubuntu:24.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential cmake \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN rm -rf build \
    && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF \
    && cmake --build build --config Release --target customdb_server --parallel 2

FROM ubuntu:24.04
RUN apt-get update \
    && apt-get install -y --no-install-recommends libstdc++6 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /app/build/customdb_server /app/customdb_server
RUN mkdir -p /app/data
EXPOSE 5432
CMD ["/app/customdb_server", "--host", "0.0.0.0", "--port", "5432", "--data", "/app/data"]
