#ifndef DFTRACER_SERIALIZATION_JSON_LINE_H
#define DFTRACER_SERIALIZATION_JSON_LINE_H

#include <dftracer/aggregator/aggregator.h>
#include <dftracer/core/enumeration.h>
#include <dftracer/core/typedef.h>
#include <dftracer/utils/configuration_manager.h>

#include <any>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <typeinfo>
#include <unordered_map>

namespace dftracer {
class JsonLines {
  bool include_metadata;
  HashType hostname_hash;
  bool convert_metadata(std::unordered_map<std::string, std::any> *metadata,
                        std::stringstream &meta_stream);

 public:
  JsonLines();
  size_t initialize(char *buffer, HashType hostname_hash);
  size_t data(char *buffer, int index, ConstEventNameType event_name,
              ConstEventNameType category, TimeResolution start_time,
              TimeResolution duration,
              std::unordered_map<std::string, std::any> *metadata,
              ProcessID process_id, ThreadID tid);
  size_t metadata(char *buffer, int index, ConstEventNameType name,
                  ConstEventNameType value, ConstEventNameType ph,
                  ProcessID process_id, ThreadID thread_id,
                  bool is_string = true);
  size_t counter(char *buffer, int index, ConstEventNameType name,
                 ConstEventNameType category, TimeResolution start_time,
                 ProcessID process_id, ThreadID thread_id,
                 std::unordered_map<std::string, std::any> *metadata);
  size_t aggregated(char *buffer, int index, ProcessID process_id,
                    dftracer::AggregatedDataType &data);
  size_t finalize(char *buffer, bool end_sym = false) {
    if (end_sym) {
      buffer[0] = ']';
      return 1;
    }
    return 0;
  }
};
}  // namespace dftracer

#endif  // DFTRACER_SERIALIZATION_JSON_LINE_H