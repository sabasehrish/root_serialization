#if !defined(EventIdentifier_h)
#define EventIdentifier_h

namespace cce::tf {
struct EventIdentifier {
  unsigned int run;
  unsigned int lumi;
  unsigned long long event;
};
}
#endif
