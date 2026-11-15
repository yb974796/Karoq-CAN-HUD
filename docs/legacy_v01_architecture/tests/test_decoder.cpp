#include "can_profiles.hpp"
#include "navigation_mux.hpp"
#include "road_sign_mux.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main(int argc,char**argv){
  assert(argc==2);
  std::string err; hud::CanDecoder decoder;
  assert(decoder.load_csv(argv[1],err));
  hud::CanFrame f; f.id=0x30B; f.dlc=8; f.data[6]=250; f.timestamp_ms=5000;
  hud::HudState state;
  assert(decoder.process(f,state));
  assert(std::abs(state.signals.at("cluster_speed_kph").value-80.0)<0.00001);
  assert(state.signals.at("cluster_speed_kph").fresh(5200));
  assert(!state.signals.at("cluster_speed_kph").fresh(7000));
  f.extended=true;assert(!decoder.process(f,state));
  hud::NavigationMux nav;
  hud::NavGuidance route;route.source="ios_companion";route.valid=true;route.timestamp_ms=5000;
  route.meters_to_turn=123;nav.update(route);
  assert(nav.select(6000).meters_to_turn==123);
  assert(!nav.select(9001).valid);
  hud::RoadSignMux signs;
  hud::RoadSign s; s.valid=true;s.source="camera_vza";s.type=hud::RoadSign::Type::speed_limit;s.speed_kph=60;s.timestamp_ms=10000;
  signs.update(s);assert(signs.select(12000).speed_kph==60);
  assert(!signs.select(20000).valid);
  std::cout<<"HOST_CORE_TEST_PASS\n";
}
