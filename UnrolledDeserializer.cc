#include "UnrolledDeserializer.h"
#include "TList.h"
#include "TBaseClass.h"
#include "TClonesArray.h"

namespace {
  TStreamerInfo* buildStreamerInfo(TClass* cl, void* pointer, bool canOptimize )
  {
    if (!cl) {
      return 0;
    }
    cl->BuildRealData(pointer);
    TStreamerInfo* sinfo = static_cast<TStreamerInfo*>(cl->GetStreamerInfo(cl->GetClassVersion()));
    
    // Create StreamerInfo for all base classes.
    TBaseClass* base = 0;
    TIter nextb(cl->GetListOfBases());
    while((base = static_cast<TBaseClass*>(nextb()) )) {
      if (base->IsSTLContainer()) {
        continue;
      }
      TClass* clm = TClass::GetClass(base->GetName());
      buildStreamerInfo(clm, pointer, canOptimize);
    }
    return sinfo;
  }
}

using namespace cce::tf;

UnrolledDeserializer::UnrolledDeserializer(TClass* iClass):bufferFile_{TBuffer::kWrite}, cls_(iClass){

  //Cribbed from TTree::BronchExec
  if(iClass == TClonesArray::Class()) {
    //Do not handle clones arrays
    abort();
  }

  if (iClass->GetCollectionProxy()) {
    TVirtualCollectionProxy* collProxy = iClass->GetCollectionProxy();
    TClass* inklass = collProxy->GetValueClass();
    if (!inklass && (collProxy->GetType() == 0)) {
      //no class defined
      abort();
    }
    if ( inklass && (inklass->GetCollectionProxy() == 0)) {
      ROOT::ESTLType stl = iClass->GetCollectionType();
      if ((stl != ROOT::kSTLmap) && (stl != ROOT::kSTLmultimap)) {
        if (!inklass->HasDataMemberInfo()) {
          //Container with no dictionary defined
          abort();
        }
        if (inklass->TestBit(TClass::kHasCustomStreamerMember)) {
          //do not support custom streamers as they can't be split
          abort();
        }
      }
    }
    //NOTE: for now, do not split collection of pointers
    sequence_ = createForContainer(*collProxy);
  } else {
    if (!iClass->HasDataMemberInfo() && !iClass->GetCollectionProxy()) {
      //can't find dictioanry
      abort();
    }

    if (!iClass->GetCollectionProxy() && iClass->TestBit(TClass::kHasCustomStreamerMember)) {
      //do not support custom streamer
      abort();
    }
    
    if(!iClass->CanSplit()) {
      //require class be splittable
      abort();
    }
    auto destr = [iClass](void * iPtr) { iClass->Destructor(iPtr); };
    
    std::unique_ptr<void, decltype(destr)> pObj( iClass->New(), destr);
    
    TStreamerInfo* sinfo = buildStreamerInfo(iClass, pObj.get(), true /*splitlevel == 0*/);
    if (!sinfo) {
      //failed to build StreamerInfo
      abort();
    }
    sequence_ = createForObject(*iClass, *sinfo);
  }
}

//NOTE:  TBranchElement::SetFillActionSequence()
// fInfo
// fType
//    for fType = 41
//     GetInfoImp(), GetParentClass(), GetCollectionProxy()
//    for fType = 3 or 4 and !fNewIDs.empty()
//     FindOnfileInfo(fClonesClass, fBranches)
//        NOTE: only wants fBranches in order to ask each branch for its fInfo
// types
//   fType == 3 is TClonesArray element
//   fType == 4 an STL container that is split
//   fType == 2 ?? a top level object being unrolled?
//   fType == 41 is a member data of an object in an unrolled STL container

std::unique_ptr<TStreamerInfoActions::TActionSequence> UnrolledDeserializer::createForContainer(TVirtualCollectionProxy& iProxy) const {
  return std::unique_ptr<TStreamerInfoActions::TActionSequence>();
}


namespace {
    bool canSelfReference(TClass *cl) {
       if (cl) {
          if (cl->GetCollectionProxy()) {
             TClass *inside = cl->GetCollectionProxy()->GetValueClass();
             if (inside) {
                return canSelfReference(inside);
             } else {
               return false;
             }
          }
          const static TClassRef stringClass("std::string");
          if (cl == stringClass || cl == TString::Class()) {
            return false;
          }
          // Here we could scan through the TStreamerInfo to see if there
          // is any pointer anywhere and know whether this is a possibility
          // of selfreference (but watch out for very indirect cases).
          return true;
       }
       return false;
    }

  std::unique_ptr<TStreamerInfoActions::TActionSequence> setActionSequence(TClass *originalClass, TStreamerInfo *localInfo, TVirtualCollectionProxy* collectionProxy, TStreamerInfoActions::TActionSequence::SequenceGetter_t create, bool isSplitNode, int iID, size_t iOffset )
  {
    TStreamerInfoActions::TIDs newIDs;
    if (!isSplitNode) {
      newIDs.insert(newIDs.begin(),iID); // Include the main element in the sequence.
    }
    
    auto original = create(localInfo, collectionProxy, originalClass);  

    std::unique_ptr<TStreamerInfoActions::TActionSequence> actionSequence( original->CreateSubSequence(newIDs, iOffset, create) );
    
    return actionSequence;
  }
}

// parent=nullptr id=-1 btype=0 (the default) fStreamerType=-1,  fEntryOffsetLen = 1000 (since TTree::fDefaultEntryOffsetLen is not chagned?) fType = 0 (NOTE: if has custom streamer than fType=-1) fOffset=0 (from default TBranch constructor)
// if CanSelfReference returns true then SetBit(kBranchAny)
// Since fStreamerType=-1 then the TLeafElement has fLenType=0 since this is a EDataType::kOther
//    fFillLeaves = (FillLeaves_t)&TBranchElement::FillLeavesMember;
//     TStreamerInfoActions::TActionSequence::WriteMemberWiseActionsGetter;
std::unique_ptr<TStreamerInfoActions::TActionSequence> UnrolledDeserializer::createForObject(TClass& iClass, TStreamerInfo& iInfo) const {

  auto create = TStreamerInfoActions::TActionSequence::ReadMemberWiseActionsGetter;
  return setActionSequence(nullptr, &iInfo, nullptr, create, false, -1, 0);
}
