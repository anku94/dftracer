#ifndef DFTRACER_BUFFER_H
#define DFTRACER_BUFFER_H
#include <dftracer/compression/zlib_compression.h>
#include <dftracer/core/logging.h>
//
#include <dftracer/aggregator/aggregator.h>
#include <dftracer/core/enumeration.h>
#include <dftracer/core/typedef.h>
#include <dftracer/serialization/json_line.h>
#include <dftracer/utils/configuration_manager.h>
#include <dftracer/writer/stdio_writer.h>

#include <any>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
namespace dftracer {
class BufferManager {
 public:
  BufferManager()
      : enable_compression(false),
        buffer(nullptr),
        buffer_size(0),
        buffer_pos(0),
        mtx() {}
  ~BufferManager() {}

  int initialize(const char* filename, HashType hostname_hash);

  int finalize(int index, ProcessID process_id, bool end_sym = false);

  void log_data_event(int index, ConstEventNameType event_name,
                      ConstEventNameType category, TimeResolution start_time,
                      TimeResolution duration,
                      std::unordered_map<std::string, std::any>* metadata,
                      ProcessID process_id, ThreadID tid);

  void log_metadata_event(int index, ConstEventNameType name,
                          ConstEventNameType value, ConstEventNameType ph,
                          ProcessID process_id, ThreadID tid,
                          bool is_string = true);

  void log_counter_event(int index, ConstEventNameType name,
                         ConstEventNameType category, TimeResolution start_time,
                         ProcessID process_id, ThreadID thread_id,
                         std::unordered_map<std::string, std::any>* metadata);

 private:
  void compress_and_write_if_needed(size_t size, bool force = false);
  bool enable_compression;
  bool enable_aggregation;
  char* buffer;
  size_t buffer_size;
  size_t buffer_pos;
  std::shared_mutex mtx;

  std::shared_ptr<dftracer::JsonLines> serializer;
  std::shared_ptr<dftracer::ZlibCompression> compressor;
  std::shared_ptr<dftracer::STDIOWriter> writer;
  std::shared_ptr<dftracer::Aggregator> aggregator;
};
}  // namespace dftracer
#endif  // DFTRACER_BUFFER_H