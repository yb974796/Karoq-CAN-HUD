#pragma once
#include "hud_types.hpp"
#include <vector>
#include <string>

namespace hud {
// Initial decoder supports DBC Intel unsigned (@1+) signals with DLC<=8 only.
// DBC Motorola, signed, multiplexing, counters and BAP need dedicated decoders.
struct SignalRule {
  std::string name;
  uint32_t can_id;
  bool extended;
  unsigned start_bit;
  unsigned bit_length;
  double factor;
  double offset;
  std::string unit;
  uint32_t timeout_ms;
};
class CanDecoder {
public:
  bool load_csv(const std::string &path, std::string &error);
  bool process(const CanFrame &frame, HudState &state) const;
  const std::vector<SignalRule>& rules() const { return rules_; }
private:
  std::vector<SignalRule> rules_;
};
}
