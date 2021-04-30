#include "UnrolledDeserializer.h"

#include "common_unrolling.h"

#include "TStreamerElement.h"
#include <iostream>

using namespace cce::tf;
using namespace cce::tf::unrolling;

UnrolledDeserializer::UnrolledDeserializer(TClass* iClass):bufferFile_{TBuffer::kRead}, cls_(iClass){
  offsetAndSequences_ = buildReadActionSequence(*iClass);
}
