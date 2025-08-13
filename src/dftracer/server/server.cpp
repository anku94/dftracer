#include "server.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

std::atomic<bool> running(true);

static void server_signal_handler(int signal) {
  if (signal == SIGINT) {
    running = false;
    std::cout << "\nSIGINT received. Shutting down server..." << std::endl;
  }
}

int main() {
  std::signal(SIGINT, server_signal_handler);

  auto server =
      dftracer::DFTracerService();  // Assuming Server is defined in server.h

  server.start();  // Or appropriate method to start the server

  while (running) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // Optionally, you can add server health checks or logs here
  }

  server.stop();  // Or appropriate method to stop the server

  return 0;
}