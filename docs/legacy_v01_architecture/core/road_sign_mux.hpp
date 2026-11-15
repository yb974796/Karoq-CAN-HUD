#pragma once
#include "hud_types.hpp"
namespace hud {
// Recognized camera sign takes precedence; map-derived speed limits must be labeled as estimates.
class RoadSignMux {
public:
 void update(const RoadSign &s){if(s.source=="camera_vza")camera_=s;else if(s.source=="map_estimate")map_=s;}
 RoadSign select(uint64_t now_ms)const{
   if(fresh(camera_,now_ms))return camera_;
   if(fresh(map_,now_ms))return map_;
   return {};
 }
private:
 static bool fresh(const RoadSign&s,uint64_t now){return s.valid&&now>=s.timestamp_ms&&now-s.timestamp_ms<=s.timeout_ms;}
 RoadSign camera_,map_;
};
}
