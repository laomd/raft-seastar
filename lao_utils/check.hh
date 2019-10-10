#include <stdexcept>
#include "common.hh"

BEGIN_NAMESPACE(laomd)

#define DCHECK(expr) \
if (!(expr)) {       \
    throw std::logic_error("check" #expr "failed.");                \
}

END_NAMESPACE(laomd)