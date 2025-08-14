#ifndef DFTRACER_SERVER
#define DFTRACER_SERVER

#include <dftracer/core/logging.h>
#include <dftracer/core/singleton.h>
#include <dftracer/df_logger.h>
#include <dftracer/server/common/datastructure.h>
#include <dftracer/utils/configuration_manager.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace dftracer {

class DFTracerService {
 public:
  DFTracerService() : index{0}, interval(1000), running(false) {
    auto conf =
        dftracer::Singleton<dftracer::ConfigurationManager>::get_instance();
    conf->metadata = true;
    conf->enable = true;
    conf->compression = false;
    conf->write_buffer_size = 16 * 1024 * 1024;
    interval = conf->trace_interval_ms;
    if (conf->log_file.empty()) {
      throw std::runtime_error(
          "Configuration error: Please set the log_file prefix in the "
          "configuration.");
    } else {
      // Get hostname and append to log_file
      char hostname[256];
      if (gethostname(hostname, sizeof(hostname)) == 0) {
        conf->log_file += std::string("_") + hostname;
      } else {
        throw std::runtime_error("Failed to get hostname for log_file.");
      }
      if (conf->compression) {
        conf->log_file += ".pfw.gz";
      } else {
        conf->log_file += ".pfw";
      }
      logger = DFT_LOGGER_INIT();
      buffer_manager =
          dftracer::Singleton<dftracer::BufferManager>::get_instance();
      auto hostname_hash = logger->get_hash(hostname);
      this->buffer_manager->initialize(conf->log_file.c_str(), hostname_hash);
    }
  }

  ~DFTracerService() { stop(); }

  void start() {
    running = true;
    worker = std::thread([this]() { this->progressEngine(); });
  }

  void stop() {
    running = false;
    if (worker.joinable()) worker.join();
    this->buffer_manager->finalize(index.load(std::memory_order_relaxed), true);
    if (auto conf =
            dftracer::Singleton<dftracer::ConfigurationManager>::get_instance();
        conf && conf->compression) {
      // Compress the log file using gzip
      std::string cmd = "gzip -f " + conf->log_file;
      int ret = system(cmd.c_str());
      if (ret != 0) {
        fprintf(stderr, "Warning: gzip compression failed for %s\n",
                conf->log_file.c_str());
      }
    }
  }

 private:
  std::shared_ptr<DFTLogger> logger;
  std::atomic<int> index;
  unsigned int interval;
  std::atomic<bool> running;
  std::thread worker;
  std::mutex data_mutex;
  std::shared_ptr<dftracer::BufferManager> buffer_manager;

  void progressEngine() {
    int step = 1;
    while (running) {
      TimeResolution time = logger->get_time();
      printf("Capturing step: %d, timestep: %lld, interval: %u ms\r", step,
             static_cast<long long>(time), interval);
      fflush(stdout);
      getCpuMetrics(time);
      getMemMetrics(time);
      std::this_thread::sleep_for(std::chrono::milliseconds(interval));
      ++step;
    }
  }

  void getCpuMetrics(TimeResolution time) {
    FILE* file = fopen("/proc/stat", "r");
    if (!file) return;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
      std::string str(line);
      if (str.find("cpu ") == 0) {
        // Aggregate metrics for all CPUs
        CpuMetrics metrics;
        sscanf(str.c_str(),
               "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
               &metrics.user, &metrics.nice, &metrics.system, &metrics.idle,
               &metrics.iowait, &metrics.irq, &metrics.softirq, &metrics.steal,
               &metrics.guest, &metrics.guest_nice);

        // Calculate total jiffies for all CPUs
        unsigned long long total_jiffies =
            metrics.user + metrics.nice + metrics.system + metrics.idle +
            metrics.iowait + metrics.irq + metrics.softirq + metrics.steal +
            metrics.guest + metrics.guest_nice;

        if (total_jiffies == 0) total_jiffies = 1;  // Avoid division by zero

        std::unordered_map<std::string, std::any> metadata;
        metadata["user_pct"] = 100.0 * metrics.user / total_jiffies;
        metadata["nice_pct"] = 100.0 * metrics.nice / total_jiffies;
        metadata["system_pct"] = 100.0 * metrics.system / total_jiffies;
        metadata["idle_pct"] = 100.0 * metrics.idle / total_jiffies;
        metadata["iowait_pct"] = 100.0 * metrics.iowait / total_jiffies;
        metadata["irq_pct"] = 100.0 * metrics.irq / total_jiffies;
        metadata["softirq_pct"] = 100.0 * metrics.softirq / total_jiffies;
        metadata["steal_pct"] = 100.0 * metrics.steal / total_jiffies;
        metadata["guest_pct"] = 100.0 * metrics.guest / total_jiffies;
        metadata["guest_nice_pct"] = 100.0 * metrics.guest_nice / total_jiffies;

        int current_index = index.fetch_add(1, std::memory_order_relaxed);
        buffer_manager->log_counter_event(current_index, "cpu", time,
                                          &metadata);
      } else if (str.find("cpu") == 0 && isdigit(str[3])) {
        // Per-CPU metrics
        CpuMetrics cpu;
        int cpu_index = -1;
        // The values are in units of "jiffies" (typically 1/100 or 1/250 of a
        // second, depending on system HZ)
        sscanf(str.c_str(),
               "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
               &cpu_index, &cpu.user, &cpu.nice, &cpu.system, &cpu.idle,
               &cpu.iowait, &cpu.irq, &cpu.softirq, &cpu.steal, &cpu.guest,
               &cpu.guest_nice);

        // Calculate total jiffies for this CPU
        unsigned long long total_jiffies =
            cpu.user + cpu.nice + cpu.system + cpu.idle + cpu.iowait + cpu.irq +
            cpu.softirq + cpu.steal + cpu.guest + cpu.guest_nice;

        // Avoid division by zero
        if (total_jiffies == 0) total_jiffies = 1;

        // Convert each metric to percentage
        double user_pct = 100.0 * cpu.user / total_jiffies;
        double system_pct = 100.0 * cpu.system / total_jiffies;
        double idle_pct = 100.0 * cpu.idle / total_jiffies;
        double iowait_pct = 100.0 * cpu.iowait / total_jiffies;
        double irq_pct = 100.0 * cpu.irq / total_jiffies;
        double softirq_pct = 100.0 * cpu.softirq / total_jiffies;
        double steal_pct = 100.0 * cpu.steal / total_jiffies;

        auto metadata = std::unordered_map<std::string, std::any>();
        metadata.emplace("user_pct", user_pct);
        metadata.emplace("system_pct", system_pct);
        metadata.emplace("idle_pct", idle_pct);
        metadata.emplace("iowait_pct", iowait_pct);
        metadata.emplace("irq_pct", irq_pct);
        metadata.emplace("softirq_pct", softirq_pct);
        metadata.emplace("steal_pct", steal_pct);

        std::string cpu_name = "cpu-" + std::to_string(cpu_index);
        int current_index = index.fetch_add(1, std::memory_order_relaxed);
        buffer_manager->log_counter_event(current_index, cpu_name.c_str(), time,
                                          &metadata);
      }
    }
    fclose(file);
  }

  void getMemMetrics(TimeResolution time) {
    auto current_index = this->index.fetch_add(1, std::memory_order_relaxed);
    FILE* file = fopen("/proc/meminfo", "r");
    if (!file) return;
    char line[256];
    auto metadata = std::unordered_map<std::string, std::any>();
    while (fgets(line, sizeof(line), file)) {
      char key[64];
      unsigned long long value = 0;
      if (sscanf(line, "%63[^:]: %llu", key, &value) == 2) {
        std::string k(key);
        static unsigned long long mem_available = 0;
        if (k == "MemAvailable") {
          mem_available = value;
          metadata["MemAvailable"] =
              mem_available;  // MemAvailable is 100% of itself
        } else {
          // Convert to percentage of MemAvailable if MemAvailable is known and
          // nonzero
          if (mem_available > 0) {
            metadata[k] = 100.0 * static_cast<double>(value) / mem_available;
          } else {
            metadata[k] = 0.0;
          }
        }
      }
    }
    if (!metadata.empty()) {
      buffer_manager->log_counter_event(current_index, "memory", time,
                                        &metadata);
    }
    fclose(file);
    return;
  }
};
}  // namespace dftracer

#endif  // DFTRACER_SERVER
