#include "UnrolledSerializer.h"

#include "common_unrolling.h"

#include "TStreamerElement.h"

#include <iostream>

using namespace cce::tf;
using namespace cce::tf::unrolling;

UnrolledSerializer::UnrolledSerializer(TClass* iClass):
  bufferFile_{TBuffer::kWrite},
  offsetAndSequences_{buildWriteActionSequence(*iClass)} {}
