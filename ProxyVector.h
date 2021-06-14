#if !defined(ProxyVector_h)
#define ProxyVector_h

//Base implementation
template<typename P, typename... ARGS>
class ProxyVectorImpBase {
 public:
  ProxyVectorImpBase() = default;

 virtual ~ProxyVectorImpBase() = default;

 virtual void reserve(std::size_t) = 0;
 virtual void emplace_back(ARGS... args) = 0;
 virtual std::size_t size() const = 0;

 virtual P const& operator[](std::size_t index) const = 0;
 virtual P& operator[](std::size_t index) = 0;

 struct ConstIter {
   ProxyVectorImpBase<P, ARGS...> const* container_ = nullptr;
   std::size_t index_ = 0;

   P const& operator*() const {
     return container_->operator[](index_);
   }

   ConstIter& operator++() { ++index_; return *this;}

   bool operator==(ConstIter const& iOther) const {
     return index_ == iOther.index_;
   }

   bool operator!=(ConstIter const& iOther) const {
     return index_ != iOther.index_;
   }

 };

 ConstIter begin() const { return {this, 0}; }
 ConstIter end() const {return {this, size()}; }

};

//An explicit implementation
template<typename T, typename PROXY, typename... ARGS>
  class ProxyVectorImp final: public ProxyVectorImpBase<PROXY, ARGS...> {
 public:
  ProxyVectorImp() = default;

 void reserve(std::size_t iSize) {
   storage_.reserve(iSize);
 }

 void emplace_back(ARGS... iArgs ) {
   storage_.emplace_back(std::forward<ARGS>(iArgs)...);
 }

 std::size_t size() const { return storage_.size();}

 T const& operator[](std::size_t index) const {
   return storage_[index];
 }

 T& operator[](std::size_t index) {
   return storage_[index];
 }
 private:
 std::vector<T> storage_;
};


//User facing class
template<typename P, typename... ARGS>
class ProxyVector {
 public:
  ProxyVector() = default;

  ~ProxyVector() = default;

  ProxyVector& operator=(ProxyVector&&) = default;
  ProxyVector(ProxyVector&&) = default;

 void reserve(std::size_t iSize) { imp_->reserve(iSize);}
 void emplace_back(ARGS... iargs) {imp_->emplace_back(std::forward<ARGS>(iargs)...);}
 std::size_t size() const {return imp_->size();}

 P const& operator[](std::size_t index) const {
   return imp_->operator[](index);
 }
 P& operator[](std::size_t index) {
   return imp_->operator[](index);
 }

 struct ConstIter {
   ProxyVector<P, ARGS...> const* container_ = nullptr;
   std::size_t index_ = 0;

   P const& operator*() const {
     return container_->operator[](index_);
   }

   ConstIter& operator++() { ++index_; return *this;}

   bool operator==(ConstIter const& iOther) const {
     return index_ == iOther.index_;
   }

   bool operator!=(ConstIter const& iOther) const {
     return index_ != iOther.index_;
   }

 };

 ConstIter begin() const { return {this, 0}; }
 ConstIter end() const {return {this, size()}; }

 template<typename T>
 static ProxyVector<P, ARGS...> make() {
   return ProxyVector<P, ARGS...>( std::make_unique<ProxyVectorImp<T, P, ARGS...>>() );
 }

 private:
 ProxyVector( std::unique_ptr<ProxyVectorImpBase<P, ARGS...>> iImp): imp_(std::move(iImp)) {}
 std::unique_ptr<ProxyVectorImpBase<P, ARGS...>> imp_;
};


#endif
