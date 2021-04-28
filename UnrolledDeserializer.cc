#include "UnrolledDeserializer.h"

#include "common_unrolling.h"

#include "TStreamerElement.h"
#include <iostream>

using namespace cce::tf;
using namespace cce::tf::unrolling;

UnrolledDeserializer::UnrolledDeserializer(TClass* iClass):bufferFile_{TBuffer::kRead}, cls_(iClass){

  checkIfCanHandle(iClass);

  TStreamerInfo* sinfo = buildStreamerInfo(iClass);
  if (!sinfo) {
    //failed to build StreamerInfo
    abort();
  }

  if(canUnroll(iClass, sinfo) ){
    sequences_ = createUnrolled(*iClass, *sinfo);
  } else {
    sequences_ = createRolled(*iClass, *sinfo);
  }
}

std::vector<std::unique_ptr<TStreamerInfoActions::TActionSequence>> UnrolledDeserializer::createRolled(TClass& iClass, TStreamerInfo& iInfo) const {

  std::vector<std::unique_ptr<TStreamerInfoActions::TActionSequence>> vec;
  vec.reserve(1);
  auto create = TStreamerInfoActions::TActionSequence::ReadMemberWiseActionsGetter;
  vec.emplace_back(setActionSequence(nullptr, &iInfo, nullptr, create, false, -1, 0));
  return vec;
}



// parent=nullptr id=-1 btype=0 (the default) fStreamerType=-1,  fEntryOffsetLen = 1000 (since TTree::fDefaultEntryOffsetLen is not chagned?) fType = 0 (NOTE: if has custom streamer than fType=-1) fOffset=0 (from default TBranch constructor)
// if CanSelfReference returns true then SetBit(kBranchAny)
// Since fStreamerType=-1 then the TLeafElement has fLenType=0 since this is a EDataType::kOther
//    fFillLeaves = (FillLeaves_t)&TBranchElement::FillLeavesMember;
//     TStreamerInfoActions::TActionSequence::WriteMemberWiseActionsGetter;

// Now with split level. id=-2 for the 'top level' after top level BranchElement is made, will call Unroll on it
// ftype is still 0 for the sub items (since we are effectively splitlevel == 1)

std::vector<std::unique_ptr<TStreamerInfoActions::TActionSequence>> UnrolledDeserializer::createUnrolled(TClass& iClass, TStreamerInfo& iInfo) const {

  std::vector<std::unique_ptr<TStreamerInfoActions::TActionSequence>> vec;
  vec.reserve(1);
  auto create = TStreamerInfoActions::TActionSequence::ReadMemberWiseActionsGetter;
  vec.emplace_back(setActionSequence(nullptr, &iInfo, nullptr, create, true, -2, 0));
  TIter next(iInfo.GetElements());
  TStreamerElement* element = 0;
  for (Int_t id = 0; (element = (TStreamerElement*) next()); ++id) {
    if(not elementNeedsOwnSequence(element, &iClass)) {
      continue;
    }
    auto ptr = element->GetClassPointer();
    if(ptr) {
      TStreamerInfo* sinfo = buildStreamerInfo(ptr);
      if(canUnroll(ptr,sinfo)) {
        auto offset = element->GetOffset();

        vec.emplace_back(setActionSequence(nullptr, sinfo, nullptr, create, true, -2, offset));

        TIter next(sinfo->GetElements());
        TStreamerElement* element = 0;
        for (Int_t id = 0; (element = (TStreamerElement*) next()); ++id) {
          if(not elementNeedsOwnSequence(element, &iClass)) {
            continue;
          }
          vec.emplace_back(setActionSequence(nullptr, sinfo, nullptr, create, false, id,offset));
        }
        continue;
      }
    }
      //std::cout <<" offset "<<element->GetOffset()<<" "<< (ptr? ptr->GetName(): "?")<<std::endl;
    vec.emplace_back(setActionSequence(nullptr, &iInfo, nullptr, create, false, id,0));
  }
  return vec;
}
