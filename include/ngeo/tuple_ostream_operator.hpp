/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

# pragma once
/// @file Output stream operator for Boost.Tuple.

/// @cond IMPLEMENTATION
namespace ngeo { namespace detail {
    /** Print out an interval tuple on a stream.
     */
    /** Bottom case. Just print the head, no trailing separator.
     */
    template<typename H> std::ostream& print(
        std::ostream& s,
        boost::tuples::cons<H, boost::tuples::null_type> const& c
        )
    {
        return s << '(' << c.head << ')';
    }
    
    // Upper case, print the head interval and a trailing comma
    template < typename H, typename T > std::ostream& print(
        std::ostream& s,
        boost::tuples::cons<H,T> const& c
        )
    {
        s << '(' << c.head << "), ";
        return print(s, c.tail);
    }
}}
/// @endcond

namespace boost {
/** Output stream operator for Boost.Tuples.

    @internal We split out the meta-recursive print logic in to the @c detail::print template
    functions to simplify this operator.
 */
template < typename H, typename T > inline
std::ostream& operator << ( std::ostream& s, typename boost::tuples::cons<H,T> const& c)
{
    return ngeo::detail::print(s,c);
}

}
