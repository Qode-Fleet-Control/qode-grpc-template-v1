// The Greeter server from gRPC's examples/cpp/helloworld/greeter_server.cc,
// adapted to the fleet: the listen address is 0.0.0.0:$PORT (read at runtime,
// default 8080) instead of a --port flag, and it stops cleanly on SIGTERM.
//
// Exposes:
//   helloworld.Greeter/SayHello  - the example service
//   grpc.health.v1.Health        - the standard health service (serving)
//   grpc.reflection.v1alpha      - server reflection, so grpcurl works as is:
//     grpcurl -plaintext -d '{"name":"fleet"}' localhost:8080 helloworld.Greeter/SayHello

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "helloworld.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
  Status SayHello(ServerContext* context, const HelloRequest* request, HelloReply* reply) override {
    reply->set_message("Hello " + request->name());
    return Status::OK;
  }
};

static std::unique_ptr<Server> g_server;

int main() {
  const char* env_port = std::getenv("PORT");
  const std::string port = (env_port && *env_port) ? env_port : "8080";
  const std::string server_address = "0.0.0.0:" + port;

  GreeterServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  ServerBuilder builder;
  int selected_port = 0;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(), &selected_port);
  builder.RegisterService(&service);
  g_server = builder.BuildAndStart();
  if (!g_server || selected_port == 0) {
    std::cerr << "Server failed to listen on " << server_address << std::endl;
    return 1;
  }
  std::cout << "Server listening on " << server_address << std::endl;

  // Shut down on SIGINT / SIGTERM (docker stop). Shutdown() must not run
  // inside the signal handler, so a watcher thread waits for the signal.
  sigset_t signals;
  sigemptyset(&signals);
  sigaddset(&signals, SIGINT);
  sigaddset(&signals, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &signals, nullptr);
  std::thread([signals]() mutable {
    int sig = 0;
    sigwait(&signals, &sig);
    g_server->Shutdown();
  }).detach();

  g_server->Wait();
  return 0;
}
