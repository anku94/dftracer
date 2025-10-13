#include <dftracer/aggregator/aggregator.h>

template <>
std::shared_ptr<dftracer::Aggregator>
    dftracer::Singleton<dftracer::Aggregator>::instance = nullptr;
template <>
bool dftracer::Singleton<dftracer::Aggregator>::stop_creating_instances = false;

namespace dftracer {
bool Aggregator::aggregate(int index, ConstEventNameType event_name,
                           ConstEventNameType category,
                           TimeResolution start_time, TimeResolution duration,
                           std::unordered_map<std::string, std::any>* metadata,
                           ProcessID process_id, ThreadID tid) {
  bool is_first_local = is_first;
  is_first = false;
  std::unique_lock<std::shared_mutex> lock(mtx);
  // Calculate time_interval as the largest multiple of
  // config->trace_interval_ms * 1000 less than or equal to start_time
  TimeResolution interval_us = config->trace_interval_ms * 1000;
  TimeResolution time_interval = (start_time / interval_us) * interval_us;
  auto time_iter = aggregated_data_.find(time_interval);
  if (time_iter == aggregated_data_.end()) {
    aggregated_data_.insert_or_assign(
        time_interval, std::unordered_map<AggregatedKey, AggregatedValues*>());
  }
  auto last_interval_ = last_interval;
  if (time_interval > last_interval) {
    last_interval = time_interval;
  }

  auto key = AggregatedKey{category, event_name, time_interval, tid, metadata};
  auto iter2 = aggregated_data_[time_interval].find(key);
  auto num_value = new NumberAggregationValue<TimeResolution>(duration);
  if (iter2 != aggregated_data_[time_interval].end()) {
    iter2->second->update("dur", typeid(TimeResolution), num_value);
    if (metadata != nullptr) delete (metadata);
  } else {
    auto value = new AggregatedValues();
    value->update("dur", typeid(TimeResolution), num_value);
    aggregated_data_[time_interval].insert_or_assign(key, value);
    DFTRACER_LOG_INFO("Events in %llu are %d", time_interval,
                      aggregated_data_[time_interval].size());
  }
  return !is_first_local && last_interval_ != last_interval;
}
int Aggregator::get_previous_aggregations(AggregatedDataType& data, bool all) {
  std::unique_lock<std::shared_mutex> lock(mtx);
  DFTRACER_LOG_INFO("Getting %d timestamps all: %d", aggregated_data_.size(),
                    all);
  for (auto it = aggregated_data_.begin(); it != aggregated_data_.end();) {
    if (all || it->first < last_interval) {
      data[it->first] = std::move(it->second);
      it = aggregated_data_.erase(it);
    } else {
      ++it;
    }
  }
  return 0;
}
}  // namespace dftracer
