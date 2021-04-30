#include "UnrolledDeserializer.h"

#include "common_unrolling.h"

#include "TStreamerElement.h"
#include <iostream>

using namespace cce::tf;
using namespace cce::tf::unrolling;

UnrolledDeserializer::UnrolledDeserializer(TClass* iClass): offsetAndSequences_{buildReadActionSequence(*iClass)}{}

