// jacobi().

// General includes.
#include "base/cl_sysdep.h"

// Specification.
#include "cln/numtheory.h"


// Implementation.

#include "cln/integer.h"
#include "integer/cl_I.h"
#include "cln/exception.h"
#include "base/cl_xmacros.h"

namespace cln {

int jacobi (const cl_I& a, const cl_I& b)
{
	// Check b > 0, b odd.
	if (!(b > 0))
		throw runtime_exception();
	if (!oddp(b))
		throw runtime_exception();
 {	Mutable(cl_I,a);
	Mutable(cl_I,b);
	// Ensure 0 <= a < b.
	a = mod(a,b);
	// If a and b are fixnums, choose faster routine.
	if (fixnump(b))
		return jacobi(FN_to_V(a),FN_to_V(b));
	var int v = 1;
	for (;;) {
		// (a/b) * v is invariant.
		if (b == 1)
			// b=1 implies (a/b) = 1.
			return v;
		if (a == 0)
			// b>1 and a=0 imply (a/b) = 0.
			return 0;
		if (a > (b >> 1)) {
			// a > b/2, so (a/b) = (-1/b) * ((b-a)/b),
			// and (-1/b) = -1 if b==3 mod 4.
			a = b-a;
			if (FN_to_V(logand(b,3)) == 3)
				v = -v;
			continue;
		}
		if ((a & 1) == 0) {
			// b>1 and a=2a', so (a/b) = (2/b) * (a'/b),
			// and (2/b) = -1 if b==3,5 mod 8.
			a = a>>1;
			switch (FN_to_V(logand(b,7))) {
				case 3: case 5: v = -v; break;
			}
			continue;
		}
		// a and b odd, 0 < a < b/2 < b, so apply quadratic reciprocity
		// law  (a/b) = (-1)^((a-1)/2)((b-1)/2) * (b/a).
		if (FN_to_V(logand(logand(a,b),3)) == 3)
			v = -v;
		swap(cl_I, a,b);
		// Now a > 2*b, set a := a mod b.
		if ((a >> 3) >= b)
			a = mod(a,b);
		else
			{ a = a-b; do { a = a-b; } while (a >= b); }
	}
}}

}  // namespace cln
