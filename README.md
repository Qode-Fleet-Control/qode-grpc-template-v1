# gRPC (C++) template

Provisioned from [`Qode-Fleet-Control/fleet-template-v1`](https://github.com/Qode-Fleet-Control/fleet-template-v1) — the fleet
lifecycle contract (`bin/`, `fleet.conf`, `compose.yaml`, deploy workflows) with a gRPC C++ starter laid on top.

A gRPC server in C++ (gRPC 1.51 + Protobuf from Debian trixie), built with CMake, which generates the stubs from `protos/helloworld.proto` at build time. It serves `helloworld.Greeter/SayHello`, the standard `grpc.health.v1.Health` service, and server reflection (so `grpcurl` works without the .proto). It speaks gRPC (HTTP/2), not HTTP/1.1, so `HEALTH_PATH` is empty: the fleet's check is a TCP accept on `$PORT`, the same as `qode-grpc-go-template-v1`.

## Origin

    hand-written (gRPC ships no project generator) — the Greeter server and protos/helloworld.proto from gRPC's examples/cpp/helloworld; CMakeLists.txt does what that example's CMakeLists does (protoc + grpc_cpp_plugin custom command, a hw_grpc_proto library, find_package(gRPC CONFIG)) without its common.cmake


## Run it

### On the fleet

The fleet runs it as containers (the docker runtime): `bin/run` builds the image with
`docker compose build` and then starts it with `docker compose up` in the foreground, publishing `$PORT`.

It listens on `0.0.0.0:$PORT` (default `8080`), read from the environment when the container starts,
and serves at the root of its own hostname (`https://<hash>.<FLEET_APP_DOMAIN>/`). `HEALTH_PATH` is empty, so the health check is a TCP accept on `$PORT`.

### With docker

```sh
PORT=8080 bin/run                 # build + run through compose, Ctrl-C to stop
docker compose up --build             # the same, by hand
grpcurl -plaintext localhost:8080 list
grpcurl -plaintext -d '{"name":"fleet"}' localhost:8080 helloworld.Greeter/SayHello
grpcurl -plaintext localhost:8080 grpc.health.v1.Health/Check
```

### Without docker

```sh
# Debian/Ubuntu: sudo apt install build-essential cmake pkg-config libgrpc++-dev libprotobuf-dev protobuf-compiler protobuf-compiler-grpc
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
PORT=8080 ./build/app
# or: FLEET_RUNTIME=process PORT=8080 bin/run
```

`fleet.conf` drives every script in `bin/`:

| step | docker runtime (fleet) | `FLEET_RUNTIME=process` |
|---|---|---|
| install | — | `(none)` |
| build | `docker compose build` | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j` |
| start | `docker compose up --remove-orphans` | `env PORT="$PORT" ./build/app` |

## Layout

- `protos/helloworld.proto` — the service definition (verbatim from gRPC's examples).
- `CMakeLists.txt` — generates `helloworld.pb.*` / `helloworld.grpc.pb.*` into the build tree, builds them as `hw_grpc_proto`, links the server `app` to it, `gRPC::grpc++` and `gRPC::grpc++_reflection`.
- `src/greeter_server.cc` — the Greeter implementation, health + reflection enabled, listening on `0.0.0.0:$PORT`.
- `Dockerfile` — `debian:trixie` build stage; `debian:trixie-slim` runtime with only `libgrpc++1.51t64` and `libprotobuf32t64`; non-root user `app`.
- `compose.yaml` — service `app`, publishes `${PORT:-8080}:${PORT:-8080}`, fleet variables passed through by name.

## Deviations from stock, and why

- The example takes `--port` (default 50051); this reads `$PORT` at runtime (default 8080, like the gRPC-Go template) and always binds `0.0.0.0`.
- The example's CMakeLists includes `common.cmake` to support building gRPC as a submodule / via FetchContent; this uses the system gRPC only, which keeps the build to about a minute.
- SIGINT/SIGTERM shut the server down cleanly (a watcher thread calls `Server::Shutdown()`), so `docker stop` / `bin/stop` do not wait for the kill timeout.
- `HEALTH_PATH` is empty: there is no HTTP/1.1 path to probe. Use the gRPC health service for real readiness.

## Verified

2026-10-05, Docker 29.8 on linux/amd64:

- `verify.sh <dir> 46504` (the migrate-docker-runtime skill's end-to-end check; `HEALTH_PATH` is empty, so each probe is a TCP accept) → `run=tcp-up restart=tcp-up containers_after_stop=0`; the container logged `Server listening on 0.0.0.0:46504`.
- `migrate.py audit <dir>` → `READY`.

Not verified: an actual RPC (`SayHello`, `grpc.health.v1.Health/Check`) — no gRPC client was available on the build host when this was cut. Run the `grpcurl` lines above once before relying on it.

The no-docker path (`FLEET_RUNTIME=process`) was not run on a host toolchain; it is the same CMake build the image runs.

See `docs/fleet-lifecycle.md` for the lifecycle contract.
