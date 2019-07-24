/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

/* ------------------------------------------------------------------------ */
# pragma once
# include <limits>
# include <boost/operators.hpp>
/* ----------------------------------------------------------------------- */
namespace ngeo {
/* ----------------------------------------------------------------------- */
/** Create a distince type from a builtin numeric type.
    This template class converts a basic type into a class, so that instances
    of the class act like the basic type in normal use but as a distinct type
    when evaulating overloads. This is very handy when one has several distinct
    value types that map to the same basic type. That means we can have overloads
    based on the type even though the underlying basic type is the same. The
    second template argument, X, is used only for distinguishing instantiations of
    the template with the same base type. It doesn't have to exist. One can
    declare an instatiation like
    @code
    typedef numeric_type<int, class some_random_tag_name> some_random_type;
    @endcode
    It is not necessary to ever mention some_random_tag_name again. All we need is the
    entry in the symbol table.
 */
template < typename T, typename X >
class numeric_type
    : public
        boost::totally_ordered<numeric_type<T,X>,
        boost::unit_steppable<numeric_type<T,X>,
        boost::additive<numeric_type<T,X>,
        boost::additive<numeric_type<T,X>, T> > > >
{
public:
    typedef T host_type; //!< Base builtin type.
    typedef numeric_type self; //!< Self reference type.

    //! Default constructor.
    numeric_type() : _t() {}
    //! Construct from implementation type.
    numeric_type(host_type const t) : _t(t) {}
    //! Copy constructur.
    numeric_type(self const& that) : _t(that._t) {}

    //! Assignment from implementation type.
    numeric_type & operator = (host_type const t) { _t = t; return *this; }
    //! Self assignment.
    numeric_type & operator = (self const& that) { _t = that._t; return *this; }

    /// User conversion to implementation type.
    /// @internal If we have just a single const method conversion to a copy
    /// of the @c host_type then the stream operators don't work. Only a CR
    /// conversion operator satisifies the argument matching.
    operator host_type const& () const { return _t; }
    /// User conversion to implementation type.
    operator host_type& () { return _t; }
    /// Explicit conversion to host type
    host_type raw() const { return _t; }

    bool operator == ( self const& rhs ) const { return _t == rhs._t; }
    bool operator <  ( self const& rhs ) const { return _t < rhs._t; }
    bool operator == ( host_type const& t ) const { return _t == t; }
    bool operator != ( host_type const& t ) const { return _t != t; }
    bool operator <  ( host_type const& t ) const { return _t < t; }
    bool operator >  ( host_type const& t ) const { return _t > t; }
    bool operator <= ( host_type const& t ) const { return _t <= t; }
    bool operator >= ( host_type const& t ) const { return _t >= t; }

    self& operator += ( self const& that ) { _t += that._t; return *this; }
    self& operator -= ( self const& that ) { _t -= that._t; return *this; }

    self& operator += ( host_type const& t ) { _t += t; return *this; }
    self& operator -= ( host_type const& t ) { _t -= t; return *this; }

    self& operator ++() { ++_t; return *this; }
    self& operator --() { --_t; return *this; }

private:
    host_type   _t;
};

/*  I fiddled with this for a long time, but I could not remove ambiguity
    with any variant of the boost::operator template for mixed types. Only
    explicitly writing these methods worked.
 */

template < typename T, typename X >
bool operator < ( T const& lhs, numeric_type<T,X> const& rhs )
{
    return rhs > lhs;
}

template < typename T, typename X >
bool operator <= ( T const& lhs, numeric_type<T,X> const& rhs )
{
    return rhs >= lhs;
}

template < typename T, typename X >
bool operator >= ( T const& lhs, numeric_type<T,X> const& rhs )
{
    return rhs <= lhs;
}

template < typename T, typename X >
bool operator > ( T const& lhs, numeric_type<T,X> const& rhs )
{
    return rhs < lhs;
}

template < typename T, typename X >
bool operator == ( T const& lhs, numeric_type<T,X> const& rhs )
{
    return rhs == lhs;
}

template < typename T, typename X >
bool operator != ( T const& lhs, numeric_type<T,X> const& rhs )
{
    return rhs != lhs;
}
/* ----------------------------------------------------------------------- */
} /* end namespace ngeo */
/* ----------------------------------------------------------------------- */
