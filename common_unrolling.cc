#include "common_unrolling.h"

#include "TList.h"
#include "TBaseClass.h"
#include "TClonesArray.h"
#include "TStreamerElement.h"


namespace cce::tf::unrolling {
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

  void checkIfCanHandle(TClass* iClass) {
    //Cribbed from TTree::BronchExec
    /*if(iClass == TClonesArray::Class()) { commented out since was getting false positives!
    //Do not handle clones arrays
    abort();
    } */
    
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
      if(element->GetClass() && element->GetClass()->GetCollectionProxy() ) {
        //Do not want to handle collections yet
        return false;
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
