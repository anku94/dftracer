#include <dftracer/core/datastructure.h>

namespace dftracer {

void BaseAggregatedValue::update(BaseAggregatedValue *value) {
  if (_id == typeid(double)) {
    NUM_AGGREGATE(_child, NumberAggregationValue, double);
  } else if (_id == typeid(unsigned long long)) {
    NUM_AGGREGATE(_child, NumberAggregationValue, unsigned long long);
  } else if (_id == typeid(unsigned int)) {
    NUM_AGGREGATE(_child, NumberAggregationValue, unsigned int);
  } else if (_id == typeid(int)) {
    NUM_AGGREGATE(_child, NumberAggregationValue, int);
  } else if (_id == typeid(size_t)) {
    NUM_AGGREGATE(_child, NumberAggregationValue, size_t);
  } else if (_id == typeid(uint16_t)) {
    NUM_AGGREGATE(_child, NumberAggregationValue, uint16_t);
  } else if (_id == typeid(HashType)) {
    NUM_AGGREGATE(_child, AggregatedValue, HashType);
  } else if (_id == typeid(long)) {
    NUM_AGGREGATE(_child, NumberAggregationValue, long);
  } else if (_id == typeid(ssize_t)) {
    NUM_AGGREGATE(_child, NumberAggregationValue, ssize_t);
  } else if (_id == typeid(off_t)) {
    NUM_AGGREGATE(_child, NumberAggregationValue, off_t);
  } else if (_id == typeid(off64_t)) {
    NUM_AGGREGATE(_child, NumberAggregationValue, off64_t);
  } else if (_id == typeid(std::string)) {
    NUM_AGGREGATE(_child, AggregatedValue, std::string);
  } else if (_id == typeid(const char *)) {
    NUM_AGGREGATE(_child, AggregatedValue, const char *);
  } else {
    DFTRACER_LOG_INFO("No conversion for type %s", _id.name());
  }
}

BaseAggregatedValue *BaseAggregatedValue::get_value() { return _child; }
}  // namespace dftracer