#include <dftracer/core/logging.h>
#include <dftracer/core/singleton.h>
#include <dftracer/serialization/json_line.h>

#include <cstring>
#include <memory>
#include <mutex>
namespace dftracer {
template <>
std::shared_ptr<JsonLines> Singleton<JsonLines>::instance = nullptr;
template <>
bool Singleton<JsonLines>::stop_creating_instances = false;
JsonLines::JsonLines() : include_metadata(false) {
  auto conf = Singleton<ConfigurationManager>::get_instance();
  include_metadata = conf->metadata;
}

size_t JsonLines::initialize(char *buffer, HashType hostname_hash) {
  this->hostname_hash = hostname_hash;
  buffer[0] = '[';
  buffer[1] = '\n';
  return 2;
}

bool JsonLines::convert_metadata(
    std::unordered_map<std::string, std::any> *metadata,
    std::stringstream &meta_stream) {
  auto meta_size = metadata->size();
  long unsigned int i = 0;
  bool has_meta = false;
  for (const auto &item : *metadata) {
    has_meta = true;
    if (item.second.type() == typeid(unsigned long long)) {
      meta_stream << "\"" << item.first
                  << "\":" << std::any_cast<unsigned long long>(item.second);
      if (i < meta_size - 1) meta_stream << ",";
    } else if (item.second.type() == typeid(unsigned int)) {
      meta_stream << "\"" << item.first
                  << "\":" << std::any_cast<unsigned int>(item.second);
      if (i < meta_size - 1) meta_stream << ",";
    } else if (item.second.type() == typeid(double)) {
      meta_stream << "\"" << item.first
                  << "\":" << std::any_cast<double>(item.second);
      if (i < meta_size - 1) meta_stream << ",";
    } else if (item.second.type() == typeid(int)) {
      meta_stream << "\"" << item.first
                  << "\":" << std::any_cast<int>(item.second);
      if (i < meta_size - 1) meta_stream << ",";
    } else if (item.second.type() == typeid(const char *)) {
      meta_stream << "\"" << item.first << "\":\""
                  << std::any_cast<const char *>(item.second) << "\"";
      if (i < meta_size - 1) meta_stream << ",";
    } else if (item.second.type() == typeid(std::string)) {
      meta_stream << "\"" << item.first << "\":\""
                  << std::any_cast<std::string>(item.second) << "\"";
      if (i < meta_size - 1) meta_stream << ",";
    } else if (item.second.type() == typeid(size_t)) {
      meta_stream << "\"" << item.first
                  << "\":" << std::any_cast<size_t>(item.second) << "";
      if (i < meta_size - 1) meta_stream << ",";
    } else if (item.second.type() == typeid(uint16_t)) {
      meta_stream << "\"" << item.first
                  << "\":" << std::any_cast<uint16_t>(item.second) << "";
      if (i < meta_size - 1) meta_stream << ",";

    } else if (item.second.type() == typeid(HashType)) {
      meta_stream << "\"" << item.first << "\":\""
                  << std::any_cast<HashType>(item.second) << "\"";
      if (i < meta_size - 1) meta_stream << ",";

    } else if (item.second.type() == typeid(long)) {
      meta_stream << "\"" << item.first
                  << "\":" << std::any_cast<long>(item.second) << "";
      if (i < meta_size - 1) meta_stream << ",";
    } else if (item.second.type() == typeid(ssize_t)) {
      meta_stream << "\"" << item.first
                  << "\":" << std::any_cast<ssize_t>(item.second) << "";
      if (i < meta_size - 1) meta_stream << ",";
    } else if (item.second.type() == typeid(off_t)) {
      meta_stream << "\"" << item.first
                  << "\":" << std::any_cast<off_t>(item.second) << "";
      if (i < meta_size - 1) meta_stream << ",";
    } else if (item.second.type() == typeid(off64_t)) {
      meta_stream << "\"" << item.first
                  << "\":" << std::any_cast<off64_t>(item.second) << "";
      if (i < meta_size - 1) meta_stream << ",";
    } else {
      DFTRACER_LOG_INFO("No conversion for type %s", item.first.c_str());
    }
    i++;
  }
  if (has_meta && metadata && metadata->size() > 0) delete (metadata);
  return has_meta;
}

size_t JsonLines::data(char *buffer, int index, ConstEventNameType event_name,
                       ConstEventNameType category, TimeResolution start_time,
                       TimeResolution duration,
                       std::unordered_map<std::string, std::any> *metadata,
                       ProcessID process_id, ThreadID thread_id) {
  size_t written_size = 0;
  if (include_metadata && metadata != nullptr) {
    std::stringstream all_stream;
    std::stringstream meta_stream;
    bool has_meta = convert_metadata(metadata, meta_stream);

    if (has_meta) {
      all_stream << "," << meta_stream.str();
    }
    written_size = sprintf(
        buffer,
        R"({"id":%d,"name":"%s","cat":"%s","pid":%d,"tid":%lu,"ts":%llu,"dur":%llu,"ph":"X","args":{"hhash":"%s"%s}})",
        index, event_name, category, process_id, thread_id, start_time,
        duration, this->hostname_hash, all_stream.str().c_str());
  } else {
    written_size = sprintf(
        buffer,
        R"({"id":%d,"name":"%s","cat":"%s","pid":%d,"tid":%lu,"ts":%llu,"dur":%llu,"ph":"X"})",
        index, event_name, category, process_id, thread_id, start_time,
        duration);
  }
  if (written_size > 0) {
    buffer[written_size++] = '\n';
    buffer[written_size] = '\0';
  }
  DFTRACER_LOG_DEBUG("JsonLines.serialize %s", buffer);
  return written_size;
}

size_t JsonLines::counter(char *buffer, int index,
                          ConstEventNameType event_name,
                          ConstEventNameType category,
                          TimeResolution start_time, ProcessID process_id,
                          ThreadID thread_id,
                          std::unordered_map<std::string, std::any> *metadata) {
  size_t written_size = 0;
  if (metadata != nullptr && !metadata->empty()) {
    std::stringstream all_stream;
    std::stringstream meta_stream;
    bool has_meta = convert_metadata(metadata, meta_stream);
    if (has_meta) {
      all_stream << "," << meta_stream.str();
    }
    written_size = sprintf(
        buffer,
        R"({"name":"%s","cat":"%s","ts":%llu,"ph":"C","pid":%d,"tid":%lu,"args":{"hhash":"%s"%s}})",
        event_name, category, start_time, process_id, thread_id,
        this->hostname_hash, all_stream.str().c_str());
  } else {
    written_size = sprintf(
        buffer,
        R"({"name":"%s","cat":"%s","ts":%llu,"ph":"C","pid":%d,"tid":%lu})",
        event_name, category, start_time, process_id, thread_id);
  }
  if (written_size > 0) {
    buffer[written_size++] = '\n';
    buffer[written_size] = '\0';
  }
  DFTRACER_LOG_DEBUG("JsonLines.serialize %s", buffer);
  return written_size;
}

size_t JsonLines::metadata(char *buffer, int index, ConstEventNameType name,
                           ConstEventNameType value, ConstEventNameType ph,
                           ProcessID process_id, ThreadID thread_id,
                           bool is_string) {
  size_t written_size = 0;
  if (is_string) {
    written_size = sprintf(
        buffer,
        R"({"id":%d,"name":"%s","cat":"dftracer","pid":%d,"tid":%lu,"ph":"M","args":{"hhash":"%s","name":"%s","value":"%s"}})",
        index, ph, process_id, thread_id, this->hostname_hash, name, value);
  } else {
    written_size = sprintf(
        buffer,
        R"({"id":%d,"name":"%s","cat":"dftracer","pid":%dq,"tid":%lu,"ph":"M","args":{"hhash":"%s","name":"%s","value":%s}})",
        index, ph, process_id, thread_id, this->hostname_hash, name, value);
  }
  buffer[written_size++] = '\n';
  buffer[written_size] = '\0';
  DFTRACER_LOG_DEBUG("ChromeWriter.convert_json_metadata %s", buffer);
  return written_size;
}

size_t JsonLines::aggregated(char *buffer, int index, ProcessID process_id,
                             dftracer::AggregatedDataType &data) {
  size_t total_written = 0;

  DFTRACER_LOG_INFO("Writing %d intervals", data.size());
  for (const auto &interval_entry : data) {
    const TimeResolution &interval = interval_entry.first;
    const auto &event_map = interval_entry.second;
    DFTRACER_LOG_INFO("Writing %d events for %llu", event_map.size(), interval);
    for (const auto &event_entry : event_map) {
      AggregatedValues *event_values = event_entry.second;
      auto key = event_entry.first;
      auto metadata = key.additional_keys;
      for (const auto &value_entry : event_values->values) {
        const std::string &base_key = value_entry.first;
        BaseAggregatedValue *base_value = value_entry.second;
        if (!base_value) continue;
        if (base_value->_id == typeid(double)) {
          EXTRACT_NUM_AGGREGATOR(base_value, double);
        } else if (base_value->_id == typeid(unsigned long long)) {
          EXTRACT_NUM_AGGREGATOR(base_value, unsigned long long);
        } else if (base_value->_id == typeid(unsigned int)) {
          EXTRACT_NUM_AGGREGATOR(base_value, unsigned int);
        } else if (base_value->_id == typeid(int)) {
          EXTRACT_NUM_AGGREGATOR(base_value, int);
        } else if (base_value->_id == typeid(size_t)) {
          EXTRACT_NUM_AGGREGATOR(base_value, size_t);
        } else if (base_value->_id == typeid(uint16_t)) {
          EXTRACT_NUM_AGGREGATOR(base_value, uint16_t);
        } else if (base_value->_id == typeid(HashType)) {
          EXTRACT_AGGREGATOR_SIMPLE(base_value, HashType);
        } else if (base_value->_id == typeid(long)) {
          EXTRACT_NUM_AGGREGATOR(base_value, long);
        } else if (base_value->_id == typeid(ssize_t)) {
          EXTRACT_NUM_AGGREGATOR(base_value, ssize_t);
        } else if (base_value->_id == typeid(off_t)) {
          EXTRACT_NUM_AGGREGATOR(base_value, off_t);
        } else if (base_value->_id == typeid(off64_t)) {
          EXTRACT_NUM_AGGREGATOR(base_value, off64_t);
        } else if (base_value->_id == typeid(std::string)) {
          EXTRACT_AGGREGATOR_SIMPLE(base_value, std::string);
        } else if (base_value->_id == typeid(const char *)) {
          EXTRACT_AGGREGATOR_SIMPLE(base_value, const char *);
        } else {
          DFTRACER_LOG_INFO("No conversion for type %s",
                            base_value->_id.name());
        }
      }
      total_written += counter(buffer + total_written, index,
                               key.event_name.c_str(), key.category.c_str(),
                               interval, process_id, key.thread_id, metadata);
    }
  }
  return total_written;
}

}  // namespace dftracer