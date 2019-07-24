/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

# pragma once

# include <boost/tuple/tuple.hpp>
# include <boost/bind.hpp>
# include <boost/mpl/if.hpp>
# include <ngeo/tuple_ostream_operator.hpp>


/** @file
    This is an utility extention to Boost::Tuples.
    The purpose is to be able to convert an existing tuple type to another tuple
    type with an additional type added to the front of the list of types in the tuple.
    For instance, converting a tuple of the form @c tuple<A2,A1,A0>
    to @c tuple<A3,A2,A1,A0>:

    @code
    typedef boost::tuples::add_type<A3, T>::type longer_tuple;
    @endcode
 */

namespace boost { namespace tuples {

/** Metafunction to create a @c get function suitable for @c boost::bind.
    @a N is the element index (0 based) and @a T is the tuple type.

    Usage:
    @code
    typedef boost::tuple::<int, double, string> triple;
    // A bound function to get the string element.
    boost::bind(boost::tuples::get_mf<2,triple>::type, _1));
    @endcode

    You can also use the static @c bind method to return a bound object
    without explicitly calling @c boost::bind. The following is exactly equivalent
    to the previous example.
    @code
    boost::tuples::get_mf<2,triple>::bind();
    @endcode
 */
template < int N, typename T >
struct get_mf {
    typedef typename boost::tuples::element<N,T>::type return_type;
    inline static return_type& type(T& tuple) { return tuple.get<N>(); }
    typedef return_type& (*function_type)(T&);
    /*  The return type was discovered via compiler errors and is sensitive to
        any implementation change in Boost.bind. I'm not sure how to do this
        better as Boost.bind doesn't have a facility for computing this type.
     */
    inline static boost::_bi::bind_t< return_type&
                                    , function_type
                                    , boost::_bi::list1<typename boost::_bi::list_av_1< boost::arg<1> >::B1>
                                    > bind() { return boost::bind(type, _1); }
};


/** Metafunction to compute a new tuple from a type and an existing tuple.
    For a type @c R and a tuple <tt>tuple<T0,T1...></tt> the result is the tuple
    <tt>tuple<R,T0,T1,...></tt>

    Example:
    @code
        typedef add_type<thing_t, current_tuple_t>::type extended_tuple_t;
    @endcode
 */
template < typename R, typename T, int N = 0 >
struct add_type {
    //! @cond NO_DOCS
    typedef typename add_type<R, T, length<T>::value >::type type;
    //! @endcond NO_DOCS
};

/** Metafunction class that calls @c add_type.
    Example:
    @code
    typedef boost::tuple<int, char, float> tuple_t;
    typedef add_type_mf::apply<thing_t, tuple_t>::type extended_tuple_t;
    // extended_tuple_t == boost::tuple<thing_t, int, char, float>
    @endcode
 */
struct add_type_mf {
    //! @cond NO_DOCS
    template < typename TYPE, typename TUPLE > struct apply {
        typedef typename add_type<TYPE,TUPLE>::type type;
    };
    //! @endcond NO_DOCS
};

//! @cond NO_DOCS
template < typename R, typename T >
struct add_type<R,T,1> {
	typedef tuple<R,
		typename element<0, T>::type
	> type;
};

template < typename R, typename T >
struct add_type<R,T,2> {
	typedef tuple<R,
		typename element<0, T>::type,
		typename element<1, T>::type
	> type;
};

template < typename R, typename T >
struct add_type<R,T,3> {
	typedef tuple<R,
		typename element<0, T>::type,
		typename element<1, T>::type,
		typename element<2, T>::type
	> type;
};

template < typename R, typename T >
struct add_type<R,T,4> {
	typedef tuple<R,
		typename element<0, T>::type,
		typename element<1, T>::type,
		typename element<2, T>::type,
		typename element<3, T>::type
	> type;
};

template < typename R, typename T >
struct add_type<R,T,5> {
	typedef tuple<R,
		typename element<0, T>::type,
		typename element<1, T>::type,
		typename element<2, T>::type,
		typename element<3, T>::type,
		typename element<4, T>::type
	> type;
};

template < typename R, typename T >
struct add_type<R,T,6> {
	typedef tuple<R,
		typename element<0, T>::type,
		typename element<1, T>::type,
		typename element<2, T>::type,
		typename element<3, T>::type,
		typename element<4, T>::type,
		typename element<5, T>::type
	> type;
};

template < typename R, typename T >
struct add_type<R,T,7> {
	typedef tuple<R,
		typename element<0, T>::type,
		typename element<1, T>::type,
		typename element<2, T>::type,
		typename element<3, T>::type,
		typename element<4, T>::type,
		typename element<5, T>::type,
		typename element<6, T>::type
	> type;
};

template < typename R, typename T >
struct add_type<R,T,8> {
	typedef tuple<R,
		typename element<0, T>::type,
		typename element<1, T>::type,
		typename element<2, T>::type,
		typename element<3, T>::type,
		typename element<4, T>::type,
		typename element<5, T>::type,
		typename element<6, T>::type,
		typename element<7, T>::type
	> type;
};

template < typename R, typename T >
struct add_type<R,T,9> {
	typedef tuple<R,
		typename element<0, T>::type,
		typename element<1, T>::type,
		typename element<2, T>::type,
		typename element<3, T>::type,
		typename element<4, T>::type,
		typename element<5, T>::type,
		typename element<6, T>::type,
		typename element<7, T>::type,
		typename element<8, T>::type
	> type;
};

/**
 * Generic methods that walk down a the dimensions of a pair
 * of tuples and invoke the templated FUNC on both of those dimensions
 */
template<typename Region, typename FUNC, int Index>
struct calc_tuple_helper {
    static void calc_tuples(Region const &t1, Region const &t2, FUNC &functor) 
    {
        functor(boost::tuples::get<Index>(t1), boost::tuples::get<Index>(t2));
        calc_tuple_helper<Region, FUNC, Index-1>::calc_tuples(t1, t2, functor);
    }  
    
    static void calc_tuples(Region const &t1, Region &t2, FUNC &functor) 
    {
        functor(boost::tuples::get<Index>(t1), boost::tuples::get<Index>(t2));
        calc_tuple_helper<Region, FUNC, Index-1>::calc_tuples(t1, t2, functor);
    }        
};

template<typename Region, typename FUNC>
struct calc_tuple_helper<Region, FUNC, 0> {
    static void calc_tuples(Region const &t1, Region const &t2, FUNC &functor)
    {
        functor(boost::tuples::get<0>(t1), boost::tuples::get<0>(t2));
    }

    static void calc_tuples(Region const &t1, Region &t2, FUNC &functor)
    {
        functor(boost::tuples::get<0>(t1), boost::tuples::get<0>(t2));
    }
};

//! @endcond
}}
