/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

/* ------------------------------------------------------------------------ */
# pragma once
/* ------------------------------------------------------------------------ */
/** @file
    Top level include for Internet Protocol support.
 */
/** @page ngeo_ip IPv4 Data Manipulation Library
    This library is intended for manipulation IPv4 data as data. It is not
    a networking library and does not provide any network operations.

    The data is stored internally in host order. The common methods
    @c host_order and @c network_order are provided to access the data
    in raw format. No other access is provided so that all such access
    explicitly declaims the byte ordering (avoiding potentially nasty
    confusion).

    Many automatic conversions are provided, but not all in order to avoid
    potential ambiguities.

    Names for various values (such as TCP ports) can be changed statically
    in the @c ip_init.cpp file, or at run time via the @c lexicon for
    the class.

    @section totally_ordered Totally Ordered
    A totally ordered type is defined to have the following properties.
    - Exactly one of A < B, A > B, or A = B is true any values A,B.
    - If A > B and B > C, then A > C.

    Many of the classes in this library are totally ordered, but not all.
    This is noted in the class documentation. Any totally ordered class is
    also has a strict weak ordering, thereby satisfying the STL container key
    requirement.

 */
/* ------------------------------------------------------------------------ */
# include <ngeo/ip_base.hpp>
# include <ngeo/ip_service.hpp>
/* ------------------------------------------------------------------------ */
