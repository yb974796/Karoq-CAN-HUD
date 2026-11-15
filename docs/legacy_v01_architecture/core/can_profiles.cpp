#include "can_profiles.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <limits>

namespace hud {
static std::vector<std::string> split(const std::string &s, char sep) {
  std::vector<std::string> out; std::istringstream in(s); std::string part;
  while(std::getline(in,part,sep)) out.push_back(part);
  return out;
}
static uint64_t extract_le(const uint8_t* bytes,unsigned bit,unsigned width){
  // Safe for up to 32-bit fields (currently limited below).
  uint64_t val=0;
  for(unsigned i=0;i<width;i++) {
    unsigned src=bit+i;
    val |= static_cast<uint64_t>((bytes[src/8]>>(src%8))&1U)<<i;
  }
  return val;
}
bool CanDecoder::load_csv(const std::string &path,std::string &error) {
  std::ifstream in(path); if(!in){error="Cannot open: "+path;return false;}
  std::vector<SignalRule> candidate;
  std::string line; unsigned line_num=0;
  while(std::getline(in,line)){
    ++line_num;
    if(line.empty() || line[0]=='#' || line.rfind("name,",0)==0) continue;
    auto p=split(line,',');
    if(p.size()!=9){error="Invalid column count at line "+std::to_string(line_num);return false;}
    try {
      SignalRule r{p[0],static_cast<uint32_t>(std::stoul(p[1],nullptr,0)),p[2]=="1",
        static_cast<unsigned>(std::stoul(p[3])),static_cast<unsigned>(std::stoul(p[4])),
        std::stod(p[5]),std::stod(p[6]),p[7],static_cast<uint32_t>(std::stoul(p[8]))};
      if(r.name.empty()||r.bit_length==0||r.bit_length>32||r.start_bit+r.bit_length>64||
         (!r.extended&&r.can_id>0x7FF)||r.can_id>0x1FFFFFFF||
         (p[2]!="0"&&p[2]!="1")||!std::isfinite(r.factor)||!std::isfinite(r.offset)||r.timeout_ms==0)
        throw std::runtime_error("Range error");
      candidate.push_back(r);
    } catch(const std::exception &e){error="Invalid rule at line "+std::to_string(line_num)+": "+e.what();return false;}
  }
  rules_=std::move(candidate); error.clear();return true;
}
bool CanDecoder::process(const CanFrame &f,HudState &state)const {
  if(f.dlc>8) return false;
  bool any=false;
  for(const auto &r: rules_) {
    if(r.can_id!=f.id||r.extended!=f.extended||r.start_bit+r.bit_length>f.dlc*8) continue;
    double val=extract_le(f.data,r.start_bit,r.bit_length)*r.factor+r.offset;
    state.signals[r.name]=SignalValue{val,f.timestamp_ms,r.timeout_ms,"can"};
    any=true;
  }
  return any;
}
}
