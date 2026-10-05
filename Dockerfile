# Built by .github/workflows/deploy.yml and pushed to Artifact Registry.
#
# gRPC C++ 1.51 on Debian trixie. Multi-stage: the build stage has protoc,
# grpc_cpp_plugin and the dev packages and generates the stubs from protos/;
# the runtime stage carries only the gRPC and Protobuf shared libraries and
# runs as a non-root user. The port is read from $PORT when the container
# starts, not at build time.
FROM debian:trixie AS build
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      build-essential cmake ninja-build pkg-config \
      libgrpc++-dev libprotobuf-dev protobuf-compiler protobuf-compiler-grpc \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY CMakeLists.txt ./
COPY protos ./protos
COPY src ./src
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build \
 && install -D build/app /out/app

FROM debian:trixie-slim AS runtime
RUN apt-get update \
 && apt-get install -y --no-install-recommends libgrpc++1.51t64 libprotobuf32t64 \
 && rm -rf /var/lib/apt/lists/* \
 && useradd -r -u 10001 app
WORKDIR /app
ARG BUILD_ID=""
ENV PORT=8080 BUILD_ID=$BUILD_ID
COPY --from=build /out/app /app/app
EXPOSE 8080
USER app
ENTRYPOINT ["/app/app"]
