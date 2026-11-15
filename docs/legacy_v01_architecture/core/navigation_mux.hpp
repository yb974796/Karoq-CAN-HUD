#pragma once
#include "hud_types.hpp"
namespace hud {
// Arbitrates normalized routes independently of CAN, BAP or BLE/Wi-Fi transports.
// An empty/no-active navigation state clears the HUD arrow rather than showing stale guidance.
class NavigationMux {
public:
  void update(const NavGuidance &nav) { if(nav.source=="oem_bap")oem_=nav; else if(nav.source=="ios_companion")ios_=nav; }
  NavGuidance select(uint64_t now_ms, bool prefer_oem=true) const {
    const auto &a=prefer_oem?oem_:ios_;
    const auto &b=prefer_oem?ios_:oem_;
    if(a.fresh(now_ms))return a;
    if(b.fresh(now_ms))return b;
    return {};
  }
private:
  NavGuidance oem_,ios_;
};
}
