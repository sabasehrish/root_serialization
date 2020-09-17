#if !defined(summarize_serializers_h)
#define summarize_serializers_h

#include <vector>
#include <iostream>
#include <chrono>
#include <iomanip>
#include "SerializerWrapper.h"

inline void summarize_serializers(std::vector<std::vector<SerializerWrapper>> const& iSerializersPerLane) {

  std::chrono::microseconds serializerTime = std::chrono::microseconds::zero();
  
  std::vector<std::pair<std::string_view, std::chrono::microseconds>> serializerTimes;
  serializerTimes.reserve(iSerializersPerLane[0].size());
  bool isFirst = true;
  for(auto const& serializers: iSerializersPerLane) {
    if(isFirst) {
      isFirst = false;
      for(auto& s: serializers) {
	serializerTimes.emplace_back(s.name(), s.accumulatedTime());
	serializerTime += s.accumulatedTime();
      }
    } else {
      int i =0;
      for(auto& s: serializers) {
	serializerTimes[i++].second += s.accumulatedTime();
	serializerTime += s.accumulatedTime();
      }
    }
  }

  std::sort(serializerTimes.begin(),serializerTimes.end(), [](auto const& iLHS, auto const& iRHS) {
      return iLHS.second > iRHS.second;
    });

  std::cout <<"Serialization total time: "<<serializerTime.count()<<"us\n";
  std::cout <<"Serialization times\n";
  for(auto const& p: serializerTimes) {
    std::cout <<"time: "<<p.second.count()<<"us "<<std::setprecision(4)<<(100.*p.second.count()/serializerTime.count())<<"%\tname: "<<p.first<<"\n";
  }
}

#endif
