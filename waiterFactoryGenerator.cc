#include "waiterFactoryGenerator.h"
#include "WaiterFactory.h"
#include "configKeyValuePairs.h"
#include <iostream>

std::function<std::unique_ptr<cce::tf::WaiterBase>(unsigned int, std::size_t)>
cce::tf::waiterFactoryGenerator(std::string_view iType, std::string_view iOptions) {
  std::function<std::unique_ptr<WaiterBase>(unsigned int, std::size_t)> waitFactory;

  auto keyValues = cce::tf::configKeyValuePairs(iOptions);
  waitFactory = [type=std::string(iType), params=ConfigurationParameters(keyValues)](unsigned int iNLanes, size_t iNDataProducts) {
    auto maker = WaiterFactory::get()->create(type, iNLanes, iNDataProducts, params);
    if(not maker) {
      return maker;
    }
    
    //make sure all parameters given were used
    auto unusedOptions = params.unusedKeys();
    if(not unusedOptions.empty()) {
      std::cout <<"Unused options in "<<type<<"\n";
      for(auto const& key: unusedOptions) {
        std::cout <<"  '"<<key<<"'"<<std::endl;
      }
      return decltype(maker)();
    }

    return maker;
  };

  return waitFactory;
}
