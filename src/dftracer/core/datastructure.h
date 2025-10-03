#ifndef DFTRACER_CORE_DATASTRUCTURE_H
#define DFTRACER_CORE_DATASTRUCTURE_H

#include <dftracer/core/enumeration.h>
#include <dftracer/core/logging.h>
#include <dftracer/core/typedef.h>
#include <stddef.h>

#include <any>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace dftracer {
struct AggregatedKey {
  std::string category;
  std::string event_name;
  TimeResolution time_interval;
  ThreadID thread_id;
  std::unordered_map<std::string, std::any> *additional_keys;
  AggregatedKey()
      : category(nullptr),
        event_name(nullptr),
        time_interval(0),
        thread_id(0),
        additional_keys(nullptr) {}

  AggregatedKey(ConstEventNameType category_, ConstEventNameType event_name_,
                TimeResolution time_interval_, ThreadID thread_id_,
                std::unordered_map<std::string, std::any> *metadata_)
      : category(category_),
        event_name(event_name_),
        time_interval(time_interval_),
        thread_id(thread_id_),
        additional_keys(metadata_) {}
  AggregatedKey(const AggregatedKey &other)
      : category(other.category),
        event_name(other.event_name),
        time_interval(other.time_interval),
        thread_id(other.thread_id),
        additional_keys(other.additional_keys) {}
  bool operator==(const AggregatedKey &other) const {
    return category == other.category && event_name == other.event_name &&
           time_interval == other.time_interval &&
           thread_id == other.thread_id &&
           additional_keys == other.additional_keys;
  }
};

// Specialize std::hash for AggregatedKey
}  // namespace dftracer

namespace std {
template <>
struct hash<dftracer::AggregatedKey> {
  std::size_t operator()(const dftracer::AggregatedKey &key) const {
    std::size_t h1 = std::hash<std::string>()(key.category);
    std::size_t h2 = std::hash<std::string>()(key.event_name);
    std::size_t h3 = std::hash<TimeResolution>()(key.time_interval);
    std::size_t h4 = std::hash<ThreadID>()(key.thread_id);

    std::size_t h5 = 0;
    if (key.additional_keys) {
      for (const auto &pair : *key.additional_keys) {
        h5 ^= std::hash<std::string>()(pair.first);
        // For std::any, we can only hash the type info
        h5 ^= pair.second.type().hash_code();
      }
    }

    // Combine hashes
    std::size_t result = h1;
    result ^= h2 + 0x9e3779b9 + (result << 6) + (result >> 2);
    result ^= h3 + 0x9e3779b9 + (result << 6) + (result >> 2);
    result ^= h4 + 0x9e3779b9 + (result << 6) + (result >> 2);
    result ^= h5 + 0x9e3779b9 + (result << 6) + (result >> 2);

    return result;
  }
};
}  // namespace std

namespace dftracer {

#define NUM_AGGREGATE(OBJ, CLASS, TYPE) \
  dynamic_cast<CLASS<TYPE> *>(OBJ)->update(dynamic_cast<CLASS<TYPE> *>(value));

#define EXTRACT_NUM_AGGREGATOR(OBJ, TYPE)                          \
  auto *num_value =                                                \
      dynamic_cast<dftracer::NumberAggregationValue<TYPE> *>(OBJ); \
  if (num_value) {                                                 \
    metadata->insert({base_key + "_count", num_value->count});     \
    metadata->insert({base_key + "_sum", num_value->sum});         \
    metadata->insert({base_key + "_min", num_value->min});         \
    metadata->insert({base_key + "_max", num_value->max});         \
  }

#define EXTRACT_AGGREGATOR_SIMPLE(OBJ, TYPE)                              \
  auto *num_value = dynamic_cast<dftracer::AggregatedValue<TYPE> *>(OBJ); \
  if (num_value) {                                                        \
    metadata->insert({base_key + "_count", num_value->count});            \
  }

struct BaseAggregatedValue {
 public:
  BaseAggregatedValue *_child;
  ValueType _type;
  std::type_index _id;

 protected:
  BaseAggregatedValue(BaseAggregatedValue *child, ValueType type,
                      std::type_index id)
      : _child(child), _type(type), _id(id) {}

 public:
  virtual ~BaseAggregatedValue() = default;
  void update(BaseAggregatedValue *value);
  BaseAggregatedValue *get_value();
};

template <typename T>
struct AggregatedValue : public BaseAggregatedValue {
 protected:
  AggregatedValue(const AggregatedValue<T> &value, BaseAggregatedValue *child)
      : BaseAggregatedValue(child, ValueType::VALUE_TYPE_STRING, typeid(T)),
        count(value.count) {}
  AggregatedValue(const AggregatedValue<T> &value, BaseAggregatedValue *child,
                  ValueType id, std::type_index tid)
      : BaseAggregatedValue(child, id, tid), count(value.count) {}
  AggregatedValue(BaseAggregatedValue *child, ValueType id, std::type_index tid)
      : BaseAggregatedValue(child, id, tid), count(1) {}

 public:
  size_t count;
  void update(AggregatedValue<T> *value) { count += value->count; }
};

template <typename T>
struct NumberAggregationValue : public AggregatedValue<T> {
 public:
  T min, max, sum;
  NumberAggregationValue(NumberAggregationValue<T> &value)
      : AggregatedValue<T>(value, this, ValueType::VALUE_TYPE_NUMBER,
                           typeid(T)),
        min(value.min),
        max(value.max),
        sum(value.sum) {}
  NumberAggregationValue(T value)
      : AggregatedValue<T>(this, ValueType::VALUE_TYPE_NUMBER, typeid(T)),
        min(value),
        max(value),
        sum(value) {}
  void update(NumberAggregationValue<T> *value) {
    if (value->min < min) min = value->min;
    if (value->max > max) max = value->max;
    sum += value->sum;
    AggregatedValue<T>::update(value);
  }
};

class AggregatedValues {
 public:
  AggregatedValues() {}
  ~AggregatedValues() {}
  std::unordered_map<std::string, BaseAggregatedValue *> values;
  int update(const std::string &key, const std::type_info &id,
             BaseAggregatedValue *value) {
    auto it = values.find(key);
    if (it != values.end()) {
      it->second->update(value);
      delete value;
    } else {
      values.insert_or_assign(key, value);
    }
    return 0;
  }
};
}  // namespace dftracer
#endif  // DFTRACER_CORE_DATASTRUCTURE_H
