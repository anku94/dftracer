#ifndef DFTRACER_AGGREGATOR_H
#define DFTRACER_AGGREGATOR_H
#include <dftracer/core/logging.h>
//
#include <dftracer/core/datastructure.h>
#include <dftracer/core/enumeration.h>
#include <dftracer/core/singleton.h>
#include <dftracer/core/typedef.h>
#include <dftracer/utils/configuration_manager.h>

#include <any>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace dftracer {
typedef std::map<TimeResolution,
                 std::unordered_map<AggregatedKey, AggregatedValues *>>
    AggregatedDataType;
class Aggregator {
 private:
  AggregatedDataType aggregated_data_;
  std::shared_ptr<dftracer::ConfigurationManager> config;
  TimeResolution last_interval;
  bool is_first;
  std::shared_mutex mtx;

 public:
  Aggregator() {
    config =
        dftracer::Singleton<dftracer::ConfigurationManager>::get_instance();
    last_interval = 0;
    is_first = true;
  }
  ~Aggregator() {}
  void finalize() {
    std::unique_lock<std::shared_mutex> lock(mtx);
    for (auto &interval_map : aggregated_data_) {
      for (auto &pair : interval_map.second) {
        for (auto &val_pair : pair.second->values) {
          delete val_pair.second;
        }
        delete pair.second;
      }
    }
  }
  bool aggregate(int index, ConstEventNameType event_name,
                 ConstEventNameType category, TimeResolution start_time,
                 TimeResolution duration,
                 std::unordered_map<std::string, std::any> *metadata,
                 ProcessID process_id, ThreadID tid);
  int get_previous_aggregations(AggregatedDataType &data, bool all = false);
};
}  // namespace dftracer
#endif  // DFTRACER_AGGREGATOR_H