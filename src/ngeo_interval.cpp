/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

# include <ngeo/interval.hpp>
# include <ostream>

namespace ngeo {

std::ostream& operator << (
                           std::ostream& s,       //!< [in,out] Output stream.
                           interval_types::relation const& r //!< [in] Relation value.
                           )
{
    char const* retval = "invalid";
    switch (r) {
        case interval_types::NONE:
            retval = "none";
            break;
        case interval_types::EQUAL:
            retval = "equal";
            break;
        case interval_types::SUBSET:
            retval = "subset";
            break;
        case interval_types::SUPERSET:
            retval = "superset";
            break;
        case interval_types::ADJACENT:
            retval = "adjacent";
            break;
        case interval_types::OVERLAP:
            retval = "overlap";
            break;
        case interval_types::ADJACENT_OVERLAP:
    	    retval = "adjacent overlap";
    	    break;
    }
    s << retval;
    return s;                              
}

} // namespace ngeo
