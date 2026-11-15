#pragma once
#include "can_profiles.hpp"
#include "navigation_mux.hpp"
#include "road_sign_mux.hpp"
namespace hud {
// Main program controller, independent of ESP32 peripherals and physical source.
class MainController {
public:
 bool load_vehicle_rules(const std::string &file, std::string &err){return decoder_.load_csv(file,err);}
 void on_can_frame(const CanFrame &f){ decoder_.process(f,state_); }
 void on_navigation(const NavGuidance &n){nav_.update(n);}
 void on_road_sign(const RoadSign &s){signs_.update(s);}
 HudState snapshot(uint64_t now_ms) const {
   HudState view=state_;
   for(auto it=view.signals.begin();it!=view.signals.end();) {
     if(!it->second.fresh(now_ms)) it=view.signals.erase(it); else ++it;
   }
   view.navigation=nav_.select(now_ms);
   view.road_sign=signs_.select(now_ms);
   return view;
 }
private:
 CanDecoder decoder_;
 HudState state_;
 NavigationMux nav_;
 RoadSignMux signs_;
};
}
