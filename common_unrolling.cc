#include "common_unrolling.h"

#include "TList.h"
#include "TBaseClass.h"
#include "TClonesArray.h"
#include "TStreamerElement.h"
#include "TStreamerInfo.h"

#include <iostream>

using namespace cce::tf;
namespace {
  TStreamerInfo* buildStreamerInfo(TClass* cl);
  TStreamerInfo* buildStreamerInfo(TClass* cl, void* pointer);
  void checkIfCanHandle(TClass* iClass);
  bool elementNeedsOwnSequence(TStreamerElement* element, TClass* cl);
  bool canUnroll(TClass* iClass, TStreamerInfo* sinfo);

  unrolling::Sequence setActionSequence(TClass *originalClass, TStreamerInfo *localInfo, TVirtualCollectionProxy* collectionProxy, TStreamerInfoActions::TActionSequence::SequenceGetter_t create, bool isSplitNode, int iID, size_t iOffset );

  unrolling::OffsetAndSequences buildUnrolledActionSequence(TClass& iClass, TStreamerInfo& iInfo, TStreamerInfoActions::TActionSequence::SequenceGetter_t create);



  TStreamerInfo* buildStreamerInfo(TClass* cl, void* pointer)
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
      buildStreamerInfo(clm, pointer);
    }
    return sinfo;
  }

  TStreamerInfo* buildStreamerInfo(TClass* iClass) {
    auto destr = [iClass](void * iPtr) { iClass->Destructor(iPtr); };
    std::unique_ptr<void, decltype(destr)> pObj( iClass->New(), destr);

    return buildStreamerInfo(iClass, pObj.get());
  }

  void checkIfCanHandle(TClass* iClass) {
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
    } else {
      if (!iClass->HasDataMemberInfo() && !iClass->GetCollectionProxy()) {
        //can't find dictioanry
        abort();
      }
      
      if (!iClass->GetCollectionProxy() && iClass->TestBit(TClass::kHasCustomStreamerMember)) {
        //do not support custom streamer
        abort();
      }  
    }
  }

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


  bool elementNeedsOwnSequence(TStreamerElement* element, TClass* cl) {
    if (element->IsA() == TStreamerArtificial::Class()) {
      return false;
    }
    if (element->TestBit(TStreamerElement::kRepeat)) {
      return false;
    }
    if (element->TestBit(TStreamerElement::kCache) && !element->TestBit(TStreamerElement::kWrite)) {
      return false;
    }
 
    Bool_t isBase = (element->IsA() == TStreamerBase::Class());
    if (isBase) {
      TClass* clbase = element->GetClassPointer();
      if ((clbase == TObject::Class()) && cl->CanIgnoreTObjectStreamer()) {
        return false;
      }
      if (clbase->GetListOfRealData()->GetSize() == 0) {
        // Do not create a branch for empty bases.
        return false;
      }
    }
    return true;
  }

  // fBranchClass == sinfo->GetClass()
  bool canUnroll(TClass* iClass, TStreamerInfo* sinfo) {
    if(iClass->GetCollectionProxy()) {
      return false;
    }

    if(!iClass->CanSplit()) {
      return false;
    }

    //See if the class has an element we can't (yet) unroll
    //Cribbed from TBranchElement::Unroll
    TIter next(sinfo->GetElements());
    TStreamerElement* element = 0;
    for (Int_t id = 0; (element = (TStreamerElement*) next()); ++id) {
      if(not elementNeedsOwnSequence(element, iClass)) {
        continue;
      }

      auto streamerType = element->GetType();
      auto branchClass = sinfo->GetClass();
      if ((streamerType == TVirtualStreamerInfo::kObject) || (streamerType == TVirtualStreamerInfo::kBase) || (streamerType == TVirtualStreamerInfo::kTNamed) || (streamerType == TVirtualStreamerInfo::kTObject) || (streamerType == TVirtualStreamerInfo::kObjectp) || (streamerType == TVirtualStreamerInfo::kObjectP)) {
        if (canSelfReference(branchClass)) {
          if (branchClass->IsTObject()) {
            //SetBit(kBranchObject);
            return false;
          } else {
            //SetBit(kBranchAny);
          }
        }
      }
      if (element->IsA() == TStreamerBasicPointer::Class()) {
        // varying length array
        return false;
      }
      if (element->IsA() == TStreamerLoop::Class()) {
        // varying length array data member
        return false;
      }
    }
    //return false;
    return true;
  }

//  NOTE: fNewIDs are filled in via InitInfo which is called by GetInfoImp (which returns the TStreamerInfo). It only does
//   work if dealing with a container rather than an object.
  unrolling::Sequence setActionSequence(TClass *originalClass, TStreamerInfo *localInfo, TVirtualCollectionProxy* collectionProxy, TStreamerInfoActions::TActionSequence::SequenceGetter_t create, bool isSplitNode, int iID, size_t iOffset )
  {
    TStreamerInfoActions::TIDs newIDs;
    if (!isSplitNode) {
      newIDs.insert(newIDs.begin(),iID); // Include the main element in the sequence.
    }
    
    auto original = create(localInfo, collectionProxy, originalClass);  

    unrolling::Sequence actionSequence( original->CreateSubSequence(newIDs, iOffset, create) );

    return actionSequence;
  }


// parent=nullptr id=-1 btype=0 (the default) fStreamerType=-1,  fEntryOffsetLen = 1000 (since TTree::fDefaultEntryOffsetLen is not chagned?) fType = 0 (NOTE: if has custom streamer than fType=-1) fOffset=0 (from default TBranch constructor)
// if CanSelfReference returns true then SetBit(kBranchAny)
// Since fStreamerType=-1 then the TLeafElement has fLenType=0 since this is a EDataType::kOther
//    fFillLeaves = (FillLeaves_t)&TBranchElement::FillLeavesMember;
//     TStreamerInfoActions::TActionSequence::WriteMemberWiseActionsGetter;

// Now with split level. id=-2 for the 'top level' after top level BranchElement is made, will call Unroll on it
// ftype is still 0 for the sub items (since we are effectively splitlevel == 1)

  void addUnrolledActionSequencesForElement(TStreamerInfo* parentInfo, int idInParent, TStreamerElement* element, TClass& iClass, TStreamerInfoActions::TActionSequence::SequenceGetter_t create, int baseOffset, unrolling::OffsetAndSequences& oSeq) {
    if(not elementNeedsOwnSequence(element, &iClass)) {
      return;
    }
    auto ptr = element->GetClassPointer();
    if(ptr) {
      TStreamerInfo* sinfo = buildStreamerInfo(ptr);
      if(canUnroll(ptr,sinfo)) {
        //std::cout <<"unrolling "<<ptr->GetName()<<std::endl;
        auto offset = element->GetOffset();
        
        oSeq.emplace_back(baseOffset, setActionSequence(nullptr, sinfo, nullptr, create, true, -2, 0));
        
        TIter next(sinfo->GetElements());
        TStreamerElement* element = 0;
        for (Int_t id = 0; (element = (TStreamerElement*) next()); ++id) {
          if(not elementNeedsOwnSequence(element, ptr)) {
            return;
          }
          addUnrolledActionSequencesForElement(sinfo, id, element, *ptr, create, baseOffset+offset,oSeq);
        }
        return;
      }
      //std::cout <<"rolled "<<ptr->GetName()<<std::endl;
    }
   
    //std::cout <<" baseOffset "<<baseOffset<<" offset "<<element->GetOffset()<<" "<< element->GetName()<<" "<< (ptr? ptr->GetName(): "?")<<std::endl;
    oSeq.emplace_back(baseOffset, setActionSequence(nullptr, parentInfo, nullptr, create, false, idInParent, 0));
  }

  unrolling::OffsetAndSequences buildUnrolledActionSequence(TClass& iClass, TStreamerInfo& iInfo, TStreamerInfoActions::TActionSequence::SequenceGetter_t create) {
    unrolling::OffsetAndSequences vec;
    vec.reserve(1);
    vec.emplace_back(0, setActionSequence(nullptr, &iInfo, nullptr, create, true, -2, 0));
    TIter next(iInfo.GetElements());
    TStreamerElement* element = 0;
    for (Int_t id = 0; (element = (TStreamerElement*) next()); ++id) {
      addUnrolledActionSequencesForElement(&iInfo, id, element, iClass, create, 0, vec);
    }
    return vec;
  }

  unrolling::OffsetAndSequences buildActionSequence(TClass& iClass, TStreamerInfoActions::TActionSequence::SequenceGetter_t create) {
  checkIfCanHandle(&iClass);

  TStreamerInfo* sinfo = buildStreamerInfo(&iClass);
  if (!sinfo) {
    //failed to build StreamerInfo
    abort();
  }
  
  if(canUnroll(&iClass, sinfo) ){
    return buildUnrolledActionSequence(iClass, *sinfo, create);
  }

  unrolling::OffsetAndSequences vec;
  vec.reserve(1);
  vec.emplace_back(0,setActionSequence(nullptr, sinfo, nullptr, create, false, -1, 0));
  return vec; 
 }
}

namespace cce::tf::unrolling {
  unrolling::OffsetAndSequences buildReadActionSequence(TClass& iClass) {
    return buildActionSequence(iClass, TStreamerInfoActions::TActionSequence::ReadMemberWiseActionsGetter);
  }

  unrolling::OffsetAndSequences buildWriteActionSequence(TClass& iClass) {
    return buildActionSequence(iClass, TStreamerInfoActions::TActionSequence::WriteMemberWiseActionsGetter);
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
/*
// parent = nullptr fID=-1 fStreamerType=-1 fMother=this fType=0
std::unique_ptr<TStreamerInfoActions::TActionSequence> UnrolledSerializer::createForContainer(TVirtualCollectionProxy& iProxy) const {
  auto collProxy = iProxy.Generate(); //fCollProxy

  auto stlType = iProxy.GetCollectionType();
  if(stlType < 0 ) { stlType = -1*stlType;}
  return std::unique_ptr<TStreamerInfoActions::TActionSequence>();
}
*/

// parent=nullptr id=-1 btype=0 (the default) fStreamerType=-1,  fEntryOffsetLen = 1000 (since TTree::fDefaultEntryOffsetLen is not chagned?) fType = 0 (NOTE: if has custom streamer than fType=-1) fOffset=0 (from default TBranch constructor)
// if CanSelfReference returns true then SetBit(kBranchAny)
// Since fStreamerType=-1 then the TLeafElement has fLenType=0 since this is a EDataType::kOther
//    fFillLeaves = (FillLeaves_t)&TBranchElement::FillLeavesMember;
//     TStreamerInfoActions::TActionSequence::WriteMemberWiseActionsGetter;
//
