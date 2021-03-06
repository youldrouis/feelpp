// catalanconst().

// General includes.
#include "base/cl_sysdep.h"

// Specification.
#include "cln/float.h"


// Implementation.

#include "float/cl_F.h"
#include "float/transcendental/cl_F_tran.h"

namespace cln {

const cl_F catalanconst (void)
{
	floatformatcase(default_float_format
	,	return cl_SF_catalanconst();
	,	return cl_FF_catalanconst();
	,	return cl_DF_catalanconst();
	,	return catalanconst(len);
	);
}

}  // namespace cln
