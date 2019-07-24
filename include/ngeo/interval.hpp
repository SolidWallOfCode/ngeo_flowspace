/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

# pragma once

# include <limits>
# include <functional>
# include <boost/operators.hpp>
# include <boost/mpl/if.hpp>
# include <boost/mpl/has_xxx.hpp>
# include <boost/mpl/logical.hpp>
# include <boost/type_traits/is_arithmetic.hpp>

#   if NG_STATIC
#       define API
#   else
#       if defined(_MSC_VER)
#           if defined(NETWORK_GEOGRAPHICS_IP_API)
#               define API _declspec(dllexport)
#           else
#               define API _declspec(dllimport)
#           endif
#       else
#           define API
#       endif
#   endif

/** @file
    Support classes for creating intervals of numeric values.

    The template class can be used directly via a @c typedef or
    used as a base class if additional functionality is required.
 */
 
namespace ngeo {

/// Internal implementation namespace.
namespace detail {
    /** Create the "has_static_min_max" metafunction.
        This is used in a metric class to indicate that it has static members
        named "MIN" and "MAX" that contain the minimum and maximum values for
        the metric. The metric class should do the following if it wants
        flowspace to use those members:
        @code
        struct static_MIN_MAX_tag;
        @endcode
     */
    BOOST_MPL_HAS_XXX_TRAIT_DEF(static_MIN_MAX_tag);

    /** Compute min/max for a metric via @c numeric_limits
     */
    template <
        typename M ///< Metric type
    >
    struct extrema_by_numeric_limits {
        /// Minimum value for @a M.
        static M min() { return std::numeric_limits<M>::min(); }
        /// Maximum value for @a M.
        static M max() { return std::numeric_limits<M>::max(); }
    };

    /** Compute min/max for a metric via MIN,MAX static members.
     */
    template <
        typename M ///< Metric type
    >
    struct extrema_by_static_members  {
        /// Minimum value for @a M.
        static M min () { return M::MIN; }
        /// Maximum value for @a M.
        static M max () { return M::MAX; }
    };

    /// Pre-fix increment in functor form.
    template < typename T >
    struct pre_increment {
        T& operator () (T& t) { return ++t; }
    };

    /// Identity operator in functor form.
    template < typename T >
    struct identity {
        T& operator () (T& t) const { return t; }
        T const& operator() (T const& t) const { return t; }
    };

    /** Used to detect whether a trait has been specialized.
        @internal The point of this and the related meta functions is to provide
        certain numeric methods in an @c interval for metric types that support
        addition and subtraction without requiring all metric types to do so.
        Types that satisfy @c boost::is_arithmetic enable the methods because
        we know they will work. Otherwise the methods are disabled unless the
        client provides a trait that is the implementation.

        @see interval::width
        @see interval::operator<<
        @see interval::operator>>
     */
    BOOST_MPL_HAS_XXX_TRAIT_DEF(default_tag);

    /** Client hook for subtracting metrics in an @c interval.
        A client that wants to support the @c width and shift methods for @c interval
        must specialize this struct for the metric type with an implementation that
        returns @a lhs - @a rhs. The specialization must be placed after this
        this header file and before the declaration of the @c interval type.
        
        If the metric type has the subtract operator then the simplest implementation
        for the UDT @c metric_type is
        @code
        template <>
        struct ngeo::detail::subtraction_trait<metric_type>
            : public std::minus<metric_type>
        {};
        @endcode
     */
    template < typename T >
    struct subtraction_trait {
        struct default_tag; // DO NOT PUT THIS IN ANY SPECIALIZATION
        T operator () ( T const& lhs, T const& rhs ) const { return T(); }
    };

    /// Internal mechanism for computing the difference and width of two metrics.
    template < typename T >
    struct difference {
        /// Subtract.
        /// @returns lhs - rhs.
        T operator () (T const& lhs, T const& rhs) const {
            return typename boost::mpl::if_<boost::is_arithmetic<T>, std::minus<T>, subtraction_trait<T> >::type()(lhs, rhs);
        }
        /// Default adjustment.
        /// To get the width we need (max - min + 1), except in the case where the default
        /// subtraction is used, which returns the default constructed metric. This method
        /// adds one unless the default @c subtraction_trait is used.
        T& operator () (T& t) const {
            return typename boost::mpl::if_<boost::mpl::or_< boost::is_arithmetic <T>, boost::mpl::not_ <detail::has_default_tag <subtraction_trait <T> > > >
                , detail::pre_increment<T>
                , detail::identity<T>
            >::type()(t);
        }
    };

    /** Client hook for adding metrics in an @c interval.
        A client that wants to support the @c width and shift methods for @c interval
        must specialize this struct for the metric type with an implementation that
        returns @a lhs + @a rhs. The specialization must be placed after this
        this header file and before the declaration of the @c interval type.
        
        If the metric type has the subtract operator then the simplest implementation
        for the UDT @c metric_type is
        @code
        template <>
        struct ngeo::detail::addition_trait<metric_type>
            : public std::plus<metric_type>
        {};
        @endcode
     */
    template < typename T >
    struct addition_trait {
        struct default_tag; // DO NOT PUT THIS IN ANY SPECIALIZATION
        T operator () ( T const& lhs, T const& rhs ) const { return T(); }
    };

    /// Internal mechanism for computing the sum of two metrics.
    template < typename T >
    struct sum {
        T operator() (T const& lhs, T const& rhs) const {
            return typename boost::mpl::if_<boost::is_arithmetic<T>, std::plus<T>, addition_trait<T> >::type()(lhs, rhs);
        }
    };
} // namespace detail


/** Non-templated types for the template class @c interval.
    @see @c interval
 */
struct interval_types {
    //! Interval relation type.
    typedef enum
        { NONE                  //!< No relationship.
        , EQUAL                 //!< Same interval.
        , SUBSET                //!< All elements in LHS are also in RHS.
        , SUPERSET              //!< Every element in RHS is in LHS.
        , OVERLAP               //!< There exists at least one element in both LHS and RHS.
        , ADJACENT              //!< The two intervals are adjacent and disjoint.
        , ADJACENT_OVERLAP      //!< Intervals are adjacent or overlap.
        } relation;
};

/** A class that represents a numeric interval.
    The template argument @a T is presumed to
    - be completely ordered.
    - have prefix increment and decrement operators
    - have value semantics
    - have valid definitions in @c std::numeric_limits<T>

    The interval is always an inclusive (closed) contiguous interval,
    defined by the minimum and maximum values contained in the interval.
    An interval can be @em empty and contain no values. This is the state
    of a default constructed interval.
 */
template <
    typename T ///< Metric type.
>
struct interval
    : public interval_types
    , public boost::orable<interval<T>
        , boost::andable<interval<T>
    > > {
    T _min; //!< The minimum value in the interval
    T _max; //!< the maximum value in the interval

    typedef interval self;  //!< self reference type.
    typedef T metric_type;       //!< metric type for interval.
    typedef T const& arg_type; //!< argument type

    /// Functor to retrieve the extrema of @a metric_type.
    typedef typename boost::mpl::if_<detail::has_static_MIN_MAX_tag<T>, detail::extrema_by_static_members<T>, detail::extrema_by_numeric_limits<T> >::type extrema_functor;

    /** Default constructor.
        An empty interval is constructed.
     */
    interval() : _min(extrema_functor::max()), _max(extrema_functor::min()) { }

    /** Construct a singleton interval.
        The single argument is used as the initial value for the
        minimum and maximum interval values.
        @note Not marked @c explicit and so serves as a conversion from
        scalar values to an interval.
     */
    interval (
        arg_type single //!< the singleton value for the interval
    ) : _min(single), _max(single) {
    }

    /** Standard constructor.
        The two values are used as the minimum and maximum for the
        interval.

        @note The arguments are sorted so that a non-empty interval
        results in all cases. An empty range can be obtained by using
        the default constructor.
     */
    interval (
        arg_type x1,  //!< minimum value for the interval
        arg_type x2   //!< maximum value for the interval
    ) : _min(std::min(x1,x2)), _max(std::max(x1,x2)) {
    }

    ~interval () {
    }

    /** Reset the range to new values.
        The arguments are used as the minimum and maximum values regardless of order.
     */
    self& set (
        arg_type x1, //!< Interval extrema
        arg_type x2  //!< Interval extrema
    ) {
        _min = std::min(x1,x2);
        _max = std::max(x1,x2);
        return *this;
    }

    /// Set the interval to be a singleton.
    self& set(
        arg_type x ///< New value for minimum and maximum.
    ) {
        _min = _max = x;
        return *this;
    }

    /** Get the minimum value in the interval.
        @note The return value is unspecified if the interval is empty.
     */
    arg_type min() const { return _min; }

    /** Get the maximum value in the interval.
        @note The return value is unspecified if the interval is empty.
     */
    arg_type max() const { return _max; }

    //! Generate an interval that contains all values of the metric.
    static self all() {
        self total(extrema_functor::min(), extrema_functor::max());
        return total;
    }

    /** Logical intersection test for two intervals.
        @return @c true if there is at least one common value in the
        two intervals, @c false otherwise.
    */
    bool has_intersection (
        self const& that //!< Other interval
    ) const {
        return ( that._min <= _min && _min <= that._max )
            || ( _min <= that._min && that._min <= _max )
            ;
    }

    /** Compute the intersection of two intervals
        @return The interval consisting of values that are contained by
        both intervals. This may be the empty interval if the intervals
        are disjoint.
        @internal Co-variant
     */
    self intersection (
        self const& that //!< Other interval
    ) const {
        return self(std::max(_min, that._min), std::min(_max, that._max));
    }

    /** Test for adjacency.
        @return @c true if the intervals are adjacent.
        @note Only disjoint intervals can be adjacent.
     */
    bool is_adjacent_to (
        self const& that ///< Other interval
    ) const {
        /*  Need to be careful here. We don't know much about T and
            we certainly don't know if "t+1" even compiles for T.
            We do require the increment operator, however, so we can
            use that on a copy to get the equivalent of t+1 for adjacency
            testing. We must also handle the possiblity that T has a
            modulus and not depend on ++t > t always being true.
            However, we know that if t1 > t0 then ++t0 > t0.
         */
        if (_max < that._min) {
            T x(_max);
            return ++x == that._min;
        } else if (that._max < _min) {
            T x(that._max);
            return ++x == _min;
        }
        return false;
    }

    //! Test if the union of two intervals is also an interval.
    bool has_union (
        self const& that ///< Other interval
    ) const {
        return this->has_intersection(that) || this->is_adjacent_to(that);
    }

    /** Test if an interval is a superset of or equal to another.
        @return @c true if every value in @c that is also in @c this.
     */
    bool is_superset_of (
        self const& that ///< Other interval
    ) const {
        return _min <= that._min && that._max <= _max;
    }

    /** Test if an interval is a subset or equal to another.
        @return @c true if every value in @c this is also in @c that.
     */
    bool is_subset_of (
        self const& that ///< Other interval
    ) const {
        return that.is_superset_of(*this);
    }

    /** Test if an interval is a strict superset of another.
        @return @c true if @c this is strictly a superset of @a rhs.
     */
    bool is_strict_superset_of (
        self const& that ///< Other interval
    ) const {
        return (_min < that._min && that._max <= _max)
            || (_min <= that._min && that._max < _max);
    }

    /** Test if an interval is a strict subset of another.
        @return @c true if @c this is strictly a subset of @a that.
     */
    bool is_strict_subset_of (
        self const& that ///< Other interval
    ) const {
        return that.is_strict_superset_of(*this);
    }
    
    /** Determine the relationship between @c this and @a that interval.
        @return The relationship type.
     */
    relation relationship (
        self const& that ///< Other interval
    ) const {
        relation retval = NONE;
        if (this->has_intersection(that)) {
            if (*this == that)
                retval = EQUAL;
            else if (this->is_subset_of(that))
                retval = SUBSET;
            else if (this->is_superset_of(that))
                retval = SUPERSET;
            else
                retval = OVERLAP;
        } 
        else if (this->is_adjacent_to(that))
            retval = ADJACENT;
        return retval;
    }

    /** Compute the convex hull of this interval and another one.
        @return The smallest interval that is a superset of @c this 
        and @a that interval.
        @internal Co-variant
     */
    self hull (
        self const& that ///< Other interval
    ) const {
        // need to account for empty intervals
        return !*this ? that
            : !that ? *this
            : interval(std::min(_min, that._min), std::max(_max, that._max));
    }

    //! @name Interval operators
    //@{

    //! Check if the interval is exactly one element.
    bool is_singleton() const { return _min == _max; }

    //! Check if the interval is empty.
    bool is_empty() const { return _min > _max; }

    /** Test for empty, operator form.
        @return @c true if the interval is empty, @c false otherwise.
     */
    bool operator ! () const { return _min > _max; }

    /// Boolean equivalent type used for logical operators.
    typedef bool (self::*pseudo_bool)() const;

    /** Test for not empty, operator form.
        This allows the range to be used directly as a boolean expression.
        @return The equivalent of @c true if the range contains members,
        @c false if the range is empty.
     */
    operator pseudo_bool () const { return this->operator!() ? 0 : &self::operator!; }

    /// @return @c true if the range is maximal, @c false otherwise.
    bool is_maximal() const { return _min == extrema_functor::min() && _max == extrema_functor::max(); }

    /// Size of interval.
    /// @return The number of elements in the span, or one less iff the span is maximal,
    /// or a default constructed instance if the interval is empty.
    /// @note This clips the return value to the maximum for the @c metric_type. Arguably, it
    /// should throw in that case, but in practice clients will either consider such clipping
    /// what should be done anyway, or check for a maximal span.
    T width() const {
        if (this->is_empty()) return T(); // avoid invalid intervals
        else if (this->is_maximal()) return extrema_functor::max(); // avoid overflow
        else {
            typename detail::difference<T> op;
            T zret = op(_max, _min);
            zret = op(zret);
            return zret;
        }
    }

    /** Clip interval.
        Remove all element in @c this interval not in @a that interval.
     */
    self& operator &= (
        self const& that
    ) {
        *this = this->intersection(that);
        return *this;
    }

    /** Convex hull.
        Extend interval to cover all elements in @c this and @a that.
     */
    self& operator |= (
        self const& that
    ) {
        *this = this->hull(that);
        return *this;
    }

    /** Shift the span toward the minimum metric value.
        @a n is subtracted from both the mininum and maximum values of the span.
        @note Clipping to the minimum metric value occurs silently and independently for the endpoints.
        @return A reference to @c this object.
     */
    self& operator << (
        metric_type n ///< Shift amount
    ) {
        detail::difference<T> op;
        _min = _min < n ? extrema_functor::min() : op(_min, n);
        _max = _max < n ? extrema_functor::min() : op(_max, n);
        return *this;
    }

    /** Shift the span toward the maximum metric value.
        @a n is added to both the mininum and maximum values of the span.
        @note Clipping to the maximum metric value occurs silently and independently for the endpoints.
        @return A reference to @c this object.
     */
    self& operator >> (
        metric_type n ///< Shift amoount
    ) {
        metric_type limit = extrema_functor::max();
        limit = detail::difference<T>()(limit, n);
        detail::sum<T> op;
        _min = _min >= limit ? limit : op(_min, n);
        _max = _max >= limit ? limit : op(_max, n);
        return *this;
    }

    //@}

    /** Functor for lexicographic ordering.
        If, for some reason, an interval needs to be put in a container
        that requires a strict weak ordering, the default @c operator @c < will
        not work. Instead, this functor should be used as the comparison
        functor. E.g.
        @code
        typedef std::set<interval<T>, interval<T>::lexicographic_order> container;
        @endcode

        @note Lexicographic ordering is a standard tuple ordering where the
        order is determined by pairwise comparing the elements of both tuples.
        The first pair of elements that are not equal determine the ordering
        of the overall tuples.
     */
    struct lexicographic_order
        : public std::binary_function<self, self, bool> {
        //! Functor operator.
        bool operator () (self const& lhs, self const& rhs) const {
            return lhs._min == rhs._min
                ? lhs._max < rhs._max
                : lhs._min < rhs._min
                ;
        }
    };
};

/** Equality.
    Two intervals are equal if their min and max values are equal.
    @relates interval
 */
template <typename T> inline bool
operator == ( interval<T> const& lhs, interval<T> const& rhs ) {
    return lhs.min() == rhs.min() && lhs.max() == rhs.max();
}

/** Inequality.
    Two intervals are equal if their min and max values are equal.
    @relates interval
 */
template <typename T>  inline bool
operator != ( interval<T> const& lhs, interval<T> const& rhs ) {
    return !(lhs == rhs);
}

/** Operator form of logical intersection test for two intervals.
    @return @c true if there is at least one common value in the
    two intervals, @c false otherwise.
    @note Yeah, a bit ugly, using an operator that is not standardly
    boolean but
    - There don't seem to be better choices (&&,|| not good)
    - The assymmetry between intersection and union makes for only three natural operators
    - ^ at least looks like "intersects"
    @relates interval
 */
template <typename T> inline bool
operator ^ ( interval<T> const& lhs, interval<T> const& rhs ) {
    return lhs.has_intersection(rhs);
}

/** Containment ordering.
    @return @c true if @c this is a strict subset of @a rhs.
    @note Equivalent to @c is_strict_subset.
    @relates interval
 */
template <typename T> inline bool
operator < ( interval<T> const& lhs, interval<T> const& rhs ) {
    return rhs.is_strict_superset_of(lhs);
}

/** Containment ordering.
    @return @c true if @c this is a subset of @a rhs.
    @note Equivalent to @c is_subset.
    @relates interval
 */
template <typename T> inline bool
operator <= ( interval<T> const& lhs, interval<T> const& rhs ) {
    return rhs.is_superset_of(lhs);
}

/** Containment ordering.
    @return @c true if @c this is a strict superset of @a rhs.
    @note Equivalent to @c is_strict_superset.
    @relates interval
 */
template <typename T> inline bool
operator > ( interval<T> const& lhs, interval<T> const& rhs ) {
    return lhs.is_strict_superset_of(rhs);
}

/** Containment ordering.
    @return @c true if @c this is a superset of @a rhs.
    @note Equivalent to @c is_superset.
    @relates interval
    */
template <typename T> inline bool
operator >= ( interval<T> const& lhs, interval<T> const& rhs ) {
    return lhs.is_superset_of(rhs);
}

/** Write interval to stream.
    The interval is written as the minimum value, "..", the maximum value,
    unless the interval is empty in which case it is written as "*..*".
    @relates interval
 */
template < typename T > inline std::ostream&
operator << ( std::ostream& s, interval<T> const& intv ) {
    if (intv) s << intv.min() << ".." << intv.max();
    else s << "*..*";
    return s;
}

/** Read interval from stream.
    @return @a s
    @relates interval
 */
template < typename T > inline std::istream&
operator >> ( std::istream& s, interval<T>& intv ) {
    char c;
    T min, max;

    s >> std::ws;
    if (s && s.peek() == '*') {
        s >> c >> c; // *.
		if ('.' == c) s >> c; // dot means 2 char separator, otherwise 1 char.
		s >> c;
        intv = interval<T>();
	} else if (
		(s >> min) // initial metric.
		&& ( s >> std::ws >> c ) // whitespace and 1st separator char
		&& (('.' != c) || (s >> c)) // separator is either '..' or a single char
		&& (s >> std::ws ) // allow more white space after separator
		&& (s >> max) // final metric
		) {
		intv.set(min, max);
    }
    return s;
}

namespace detail {
    /** A functor for performing calculations on the intervals in a region.
     *  Can be used in conjunction with boost::tuples::calc_tuple_helper to calculate the
     *  overall region relationship between two regions.  The interval types of the dimensions
     *  may differ, but they must be the same between the same dimensions of the two regions
     */
    struct calc_region_functor
        : public interval_types {
        calc_region_functor() : _relation(EQUAL)  {}
        
        /// Compute the relationship between two intervals, with state.
        template <typename RT>
        void operator()(
            RT const& r1, ///< Left hand interval
            RT const& r2  ///< Right hand interval
        ) {
            if (_relation != NONE) {
                relation layerm_relationship = r1.relationship(r2);
                if (layerm_relationship == NONE) {
                    _relation = layerm_relationship;
                }
                else if (layerm_relationship != EQUAL) {  // If this dimension is equal, our previous estimate is not changed
                    if (_relation == EQUAL) {  // Just pickup whatever dimrel is
                        _relation = layerm_relationship;
                    } else if (_relation == ADJACENT || layerm_relationship == ADJACENT) {
                        _relation = NONE;
                    } else if (layerm_relationship != _relation) {
                        _relation = OVERLAP;
                    }
                }
            }
        }
        /// Test accumulated result for overlapping.
        bool overlaps() const {
            return _relation != NONE && _relation != ADJACENT;
        }
        /// @return The current accumulated result.
        relation result() { return _relation; }

        relation _relation;  ///< Accumulator.
    };

    /// Functor to expand an interval by one on each side, if possible.
    struct unit_expand_interval_functor {
        unit_expand_interval_functor() {} ///< Default constructor.
        
        /// Unit expand @a r1 and put the result in @a r2.
        template <typename RT>
        void operator()(
            RT const &r1, ///< [in] Source interval.
            RT &r2 ///< [out] Result interval.
        ) {
            typename RT::metric_type min_val = r1.min();
            typename RT::metric_type max_val = r1.max();       
            if (min_val != RT::extrema_functor::min()) --min_val;
            if (max_val != RT::extrema_functor::max()) ++max_val;
            r2.set(min_val, max_val);    
        }
        /// Unit expand @a r in place.
        template <typename RT>
        void operator()(
            RT &r /// [in,out] Interval to expand.
        ) {
            typename RT::metric_type min_val = r.min();
            typename RT::metric_type max_val = r.max();       
            if (min_val != RT::extrema_functor::min()) --min_val;
            if (max_val != RT::extrema_functor::max()) ++max_val;
            r.set(min_val, max_val);
        }
    };

}

/// Stream output operator for interval type.
extern API std::ostream& operator << (
    std::ostream& s,       //!< [in,out]
    interval_types::relation const& r //!< [in]
);

} // namespace

# undef API
