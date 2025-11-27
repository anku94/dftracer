#include <dftracer/dftracer.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace {

constexpr ConstEventNameType REGION_CATEGORY = "kokkos";
constexpr ConstEventNameType PAR_FOR_CATEGORY = "kokkos";
constexpr ConstEventNameType PAR_REDUCE_CATEGORY = "kokkos";
constexpr ConstEventNameType PAR_SCAN_CATEGORY = "kokkos";

std::once_flag init_flag;

void ensure_dftracer_initialized() {
  std::call_once(init_flag, []() { initialize_main(nullptr, nullptr, nullptr); });
}

struct EventRecord {
  std::string name;       // owns bytes so DFTracer sees stable c_str()
  DFTracerData *handle;
};

std::vector<EventRecord *> event_stack;  // rely on ordered callbacks

void push_event(ConstEventNameType category, const char *tag, const char *name) {
  ensure_dftracer_initialized();
  std::string label = std::string(tag) + ":" + (name ? name : "");
  auto *rec = new EventRecord{std::move(label), nullptr};
  rec->handle = initialize_region(rec->name.c_str(), category, DF_DATA_EVENT);
  event_stack.push_back(rec);
}

void pop_event() {
  if (event_stack.empty()) return;
  auto *rec = event_stack.back();
  event_stack.pop_back();
  if (rec->handle) finalize_region(rec->handle);
  delete rec;
}

}  // namespace

extern "C" {

struct KokkosProfiling_DeviceInfo;

void kokkosp_init_library(const int, const uint64_t, const uint32_t,
                          KokkosProfiling_DeviceInfo *) {
  ensure_dftracer_initialized();
}

void kokkosp_finalize_library() {}

void kokkosp_push_profile_region(const char *name) {
  push_event(REGION_CATEGORY, "region", name);
}

void kokkosp_pop_profile_region() { pop_event(); }

void kokkosp_begin_parallel_for(const char *name, const uint32_t,
                                uint64_t *kernel_id) {
  (void)kernel_id;
  push_event(PAR_FOR_CATEGORY, "par_for", name);
}

void kokkosp_end_parallel_for(uint64_t kernel_id) {
  (void)kernel_id;
  pop_event();
}

void kokkosp_begin_parallel_reduce(const char *name, const uint32_t,
                                   uint64_t *kernel_id) {
  (void)kernel_id;
  push_event(PAR_REDUCE_CATEGORY, "par_reduce", name);
}

void kokkosp_end_parallel_reduce(uint64_t kernel_id) {
  (void)kernel_id;
  pop_event();
}

void kokkosp_begin_parallel_scan(const char *name, const uint32_t,
                                 uint64_t *kernel_id) {
  (void)kernel_id;
  push_event(PAR_SCAN_CATEGORY, "par_scan", name);
}

void kokkosp_end_parallel_scan(uint64_t kernel_id) {
  (void)kernel_id;
  pop_event();
}

void kokkosp_begin_fence(const char *, const uint32_t, uint64_t *) {}

void kokkosp_end_fence(uint64_t) {}

void kokkosp_begin_deep_copy(const void *, const char *, const void *,
                             const void *, const char *, const void *,
                             uint64_t) {}

void kokkosp_end_deep_copy() {}

void kokkosp_allocate_data(const void *, const char *, const void *,
                           uint64_t) {}

void kokkosp_deallocate_data(const void *, const char *, const void *,
                             uint64_t) {}

void kokkosp_create_profile_section(const char *, uint32_t *) {}

void kokkosp_destroy_profile_section(uint32_t) {}

void kokkosp_start_profile_section(uint32_t) {}

void kokkosp_stop_profile_section(uint32_t) {}

void kokkosp_profile_event(const char *) {}

}  // extern "C"
