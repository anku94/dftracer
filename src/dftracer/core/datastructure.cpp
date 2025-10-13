#include <dftracer/core/datastructure.h>

namespace dftracer {

void BaseAggregatedValue::update(BaseAggregatedValue *value) {
  auto id = _id;
  DFTRACER_FOR_EACH_NUMERIC_TYPE(DFTRACER_ANY_ID_MACRO, value, {
    DFTRACER_NUM_AGGREGATE(_child, NumberAggregationValue, double);
    return;
  });
  DFTRACER_FOR_EACH_STRING_TYPE(DFTRACER_ANY_ID_MACRO, value, {
    DFTRACER_NUM_AGGREGATE(_child, AggregatedValue, double);
    return;
  });
}

BaseAggregatedValue *BaseAggregatedValue::get_value() { return _child; }
}  // namespace dftracer