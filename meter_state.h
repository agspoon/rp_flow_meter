#pragma once
#include <cstdint>

// Small fixed-size table tracking per-meter state, keyed by the last two
// bytes of the meter's BLE MAC address. Bounded and allocation-free --
// supports up to 8 distinct meters without any config changes.
//
// `inline` (not `static`) so this single array/table is shared correctly
// even if ESPHome ends up compiling different lambdas (the BLE advertise
// handler vs. the periodic heartbeat) into separate translation units --
// C++17 inline variables are merged by the linker into one instance either
// way, avoiding a subtle "each .cpp gets its own empty copy" bug.
struct MeterState {
  uint16_t mac_suffix = 0;
  bool used = false;
  uint32_t last_counter = 0;         // most recent total_usage value seen
  uint32_t last_change_millis = 0;   // millis() at the most recent advertisement processed
  uint32_t last_seen_millis = 0;     // millis() at last ANY advertisement received (heartbeat/liveness)
  int last_rssi = 0;                 // RSSI from the most recent advertisement
  uint32_t rate_anchor_counter = 0;  // total_usage at the start of the current flow-rate averaging window
  uint32_t rate_anchor_millis = 0;   // millis() at the start of the current flow-rate averaging window
  float last_flow_rate = 0.0f;       // most recently computed (windowed) flow rate, carried forward
                                       // between window boundaries
};

inline MeterState meter_states[8];

inline MeterState *get_meter_state(uint16_t suffix) {
  for (auto &s : meter_states) {
    if (s.used && s.mac_suffix == suffix)
      return &s;
  }
  for (auto &s : meter_states) {
    if (!s.used) {
      s.used = true;
      s.mac_suffix = suffix;
      return &s;
    }
  }
  return nullptr;  // table full -- more than 8 distinct meters seen
}
