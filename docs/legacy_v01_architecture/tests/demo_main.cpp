#include "main_controller.hpp"
#include <iostream>
int main(int argc,char **argv){
 if(argc!=2){std::cerr<<"Usage: hud_demo path/to/can_rules.csv\n";return 2;}
 hud::MainController app;
 std::string error;
 if(!app.load_vehicle_rules(argv[1],error)){std::cerr<<error<<"\n";return 1;}
 // Synthetic CAN frame for unit testing only, not live CAN captured from Karoq.
 hud::CanFrame f;f.id=0x30B;f.dlc=8;f.data[6]=250;f.timestamp_ms=3000;app.on_can_frame(f);
 hud::NavGuidance nav;nav.source="ios_companion";nav.maneuver=hud::NavGuidance::Maneuver::right;
 nav.meters_to_turn=1700;nav.valid=true;nav.timestamp_ms=3000;app.on_navigation(nav);
 hud::RoadSign sign;sign.source="camera_vza";sign.type=hud::RoadSign::Type::speed_limit;
 sign.speed_kph=60;sign.valid=true;sign.timestamp_ms=3000;app.on_road_sign(sign);
 auto view=app.snapshot(3250);
 std::cout<<"CAN speed (test): "<<view.signals.at("cluster_speed_kph").value<<" km/h\n";
 std::cout<<"NAV mock distance: "<<view.navigation.meters_to_turn<<" m\n";
 std::cout<<"Road sign mock: "<<view.road_sign.speed_kph<<" km/h\n";
}
