// unary operator -

// General includes.
#include "base/cl_sysdep.h"

// Specification.
#include "cln/real.h"


// Implementation.

#include "real/cl_R.h"
#include "cln/rational.h"
#include "cln/integer.h"
#include "cln/float.h"
#include "cln/sfloat.h"
#include "cln/ffloat.h"
#include "cln/dfloat.h"
#include "cln/lfloat.h"
#include "rational/cl_RA.h"
#include "integer/cl_I.h"

namespace cln {

const cl_R operator- (const cl_R& x)
#define minus(a) -a
#if 0
GEN_R_OP1_2(x, minus, return)
#else // fully inlined, faster
GEN_R_OP1_7(x, minus, return)
#endif

}  // namespace cln
