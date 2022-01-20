#if !defined(ComponentFactory_h)
#define ComponentFactory_h

#include <memory>
#include <unordered_map>

namespace cce::tf {

  template <class T>
    class ComponentFactory;

  template<typename R, typename... Args>
    class ComponentFactory<R*(Args...)> {
  public:

    using TemplateArgType = R*(Args...);
    using CreatedType = R;

    ComponentFactory(const ComponentFactory&) = delete;                   // stop default
    const ComponentFactory& operator=(const ComponentFactory&) = delete;  // stop default

    struct CMakerBase {
      CMakerBase(const std::string& iName) { ComponentFactory<R*(Args...)>::get()->registerCMaker(this, iName);}
      virtual std::unique_ptr<R> create(Args...) const = 0;
      virtual ~CMakerBase() = default;
    };
    template <class TPlug>
      struct CMaker : public CMakerBase {
      CMaker(const std::string& iName) : CMakerBase(iName) {}
      std::unique_ptr<R> create(Args... args) const override {
        return std::make_unique<TPlug>(std::forward<Args>(args)...);
      }
    };

    // ---------- const member functions ---------------------
    std::unique_ptr<R> create(const std::string& iName, Args... args) const {
      auto itFound =makers_.find(iName);
      if(itFound == makers_.end()) {
        return std::unique_ptr<R>();
      }
      return itFound->second->create(std::forward<Args>(args)...);
    }

    static ComponentFactory<R*(Args...)>* get();
    // ---------- member functions ---------------------------
    void registerCMaker(CMakerBase* iCMaker, const std::string& iName) {
      makers_[iName] = iCMaker;
    }

  private:
    ComponentFactory() { }
    std::unordered_map<std::string, CMakerBase*> makers_;
  };


}

#define REGISTER_COMPONENTFACTORY(_factory_)                                                               \
  namespace cce::tf {                                                     \
    template <>                                                                                                         \
      cce::tf::ComponentFactory<_factory_::TemplateArgType>* cce::tf::ComponentFactory<_factory_::TemplateArgType>::get() { \
      static cce::tf::ComponentFactory<_factory_::TemplateArgType> s_instance; \
      return &s_instance;                                                                                               \
    }                                                                                                                   \
  };


#endif
