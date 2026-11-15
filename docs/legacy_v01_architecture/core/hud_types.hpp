#pragma once
#include <cstdint>
#include <string>
#include <map>

namespace hud {
struct CanFrame {
  uint32_t id=0;
  bool extended=false;
  uint8_t dlc=0;
  uint8_t data[8]={};
  uint64_t timestamp_ms=0;
};
struct SignalValue {
  double value=0;
  uint64_t timestamp_ms=0;
  uint32_t timeout_ms=1000;
  std::string origin;
  bool fresh(uint64_t now_ms) const { return now_ms >= timestamp_ms && now_ms-timestamp_ms <= timeout_ms; }
};
struct NavGuidance {
  enum class Maneuver { none, left, right, straight, uturn, roundabout };
  std::string source; // e.g., oem_bap, ios_companion (NOT Apple Maps auto-access)
  Maneuver maneuver=Maneuver::none;
  double meters_to_turn=0;
  double meters_remaining=0;
  uint64_t timestamp_ms=0;
  uint32_t timeout_ms=3000;
  bool valid=false;
  bool fresh(uint64_t now_ms) const { return valid && now_ms>=timestamp_ms && now_ms-timestamp_ms <=timeout_ms; }
};
struct RoadSign {
  enum class Type { unknown, speed_limit, no_overtaking, end_restriction };
  Type type=Type::unknown;
  int speed_kph=0;
  std::string source; // camera_vza, map_estimate; never infer from navigation arrow
  uint64_t timestamp_ms=0;
  uint32_t timeout_ms=5000;
  bool valid=false;
};
struct HudState {
  std::map<std::string,SignalValue> signals;
  NavGuidance navigation;
  RoadSign road_sign;
};
}
