#ifndef DFTRACER_AGGREGATOR_H
#define DFTRACER_AGGREGATOR_H
#include <dftracer/core/logging.h>
//
#include <dftracer/core/cpp_typedefs.h>
#include <dftracer/core/datastructure.h>
#include <dftracer/core/enumeration.h>
#include <dftracer/core/singleton.h>
#include <dftracer/core/typedef.h>
#include <dftracer/utils/configuration_manager.h>
#include <dftracer/utils/utils.h>

#include <any>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace dftracer {
typedef std::unordered_map<AggregatedKey, AggregatedValues *>
    AggregatedDataPair;
typedef std::map<TimeResolution, AggregatedDataPair> AggregatedDataType;
class Aggregator {
 private:
  AggregatedDataType aggregated_data_;
  std::shared_ptr<dftracer::ConfigurationManager> config;
  TimeResolution last_interval;
  bool is_first;
  std::shared_mutex mtx;
  template <typename T>
  inline void insert_number_value(TimeResolution &time_interval,
                                  AggregatedKey &aggregated_key,
                                  const std::string &key, T value) {
    auto iter = aggregated_data_[time_interval].find(aggregated_key);
    auto num_value = new NumberAggregationValue<T>(value);
    if (iter != aggregated_data_[time_interval].end()) {
      iter->second->update(key, typeid(TimeResolution), num_value);
    } else {
      auto value = new AggregatedValues();
      value->update(key, typeid(TimeResolution), num_value);
      aggregated_data_[time_interval].insert_or_assign(aggregated_key, value);
      DFTRACER_LOG_INFO("Events in %llu are %d", time_interval,
                        aggregated_data_[time_interval].size());
    }
  }

  template <typename T>
  inline void insert_general_value(TimeResolution &time_interval,
                                   AggregatedKey &aggregated_key,
                                   const std::string &key, T value) {
    auto iter = aggregated_data_[time_interval].find(aggregated_key);
    auto num_value = new AggregatedValue<T>(value);
    if (iter != aggregated_data_[time_interval].end()) {
      iter->second->update(key, typeid(TimeResolution), num_value);
    } else {
      auto value = new AggregatedValues();
      value->update(key, typeid(TimeResolution), num_value);
      aggregated_data_[time_interval].insert_or_assign(aggregated_key, value);
      DFTRACER_LOG_INFO("Events in %llu are %d", time_interval,
                        aggregated_data_[time_interval].size());
    }
  }

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
                 TimeResolution duration, dftracer::Metadata *metadata,
                 ProcessID process_id, ThreadID tid);
  int get_previous_aggregations(AggregatedDataType &data, bool all = false);
};
}  // namespace dftracer
#endif  // DFTRACER_AGGREGATOR_H