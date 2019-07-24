/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

# pragma once

/** @file
    String comparisons.
    This provides variants of @c strcmp that are useful but not generally available.
 */

# if !defined(_MSC_VER)
#   include <string.h>
# endif

/// Network Geographics namespace
namespace ngeo {
    
/** Compare two C strings, ignoring case.

    @return
    - < 0 if @a lhs < @a rhs
    - 0 if @a lhs == @a rhs
    - > 0 if @a lhs > @a rhs
 */
inline int strnicmp
    ( char const* lhs ///< Left string
    , char const* rhs ///< Right string
    , size_t count ///< Maximum number of characters to compare
    )
{
# if defined(_MSC_VER)
    return _strnicmp(lhs, rhs, count);
# else
    return strncasecmp(lhs, rhs, count);
# endif
}

/** Compare two C strings, ignoring case.

    @return
    - < 0 if @a lhs < @a rhs
    - 0 if @a lhs == @a rhs
    - > 0 if @a lhs > @a rhs
 */
inline int stricmp
    ( char const* lhs ///< Left string
    , char const* rhs ///< Right string
    )
{
# if defined(_MSC_VER)
    return _stricmp(lhs, rhs);
# else
    return strcasecmp(lhs, rhs);
# endif
}

/** Compare two @c std:string, ignoring case and locale.

    @return
    - < 0 if @a lhs < @a rhs
    - 0 if @a lhs == @a rhs
    - > 0 if @a lhs > @a rhs
 */
inline int stricmp
    ( std::string const& lhs ///< Left string
    , std::string const& rhs ///< Right string
    )
{
    int zret = strnicmp(lhs.data(), rhs.data(), std::min(lhs.length(), rhs.length()));
    // We get a false equality if one string is an initial subsequence of the other,
    // so check for that and adjust the result accordingly.
    if (0 == zret && lhs.length() != rhs.length())
        zret = lhs.length() < rhs.length() ? -1 : 1;
    return zret;
}

/** Compare a @c std:string and a C string, ignoring case and locale.

    @return
    - < 0 if @a lhs < @a rhs
    - 0 if @a lhs == @a rhs
    - > 0 if @a lhs > @a rhs
 */
inline int stricmp
    ( std::string const& lhs ///< Left string
    , char const* rhs ///< Right string
    )
{
    size_t r_length = rhs ? strlen(rhs) : 0;
    int zret = strnicmp(lhs.data(), rhs, std::min(lhs.length(), r_length));
    // We get a false equality if one string is an initial subsequence of the other,
    // so check for that and adjust the result accordingly.
    if (0 == zret && lhs.length() != r_length)
        zret = lhs.length() < r_length ? -1 : 1;
    return zret;
}

/** Compare a @c std:string and a C string, ignoring case and locale.

    @return
    - < 0 if @a lhs < @a rhs
    - 0 if @a lhs == @a rhs
    - > 0 if @a lhs > @a rhs
 */
inline int stricmp
    ( char const* lhs ///< Left string
    , std::string const& rhs ///< Right string
    )
{
    size_t l_length = lhs ? strlen(lhs) : 0;
    int zret = strnicmp(lhs, rhs.data(), std::min(l_length, rhs.length()));
    // We get a false equality if one string is an initial subsequence of the other,
    // so check for that and adjust the result accordingly.
    if (0 == zret && rhs.length() != l_length)
        zret = l_length < rhs.length() ? -1 : 1;
    return zret;
}

} // namespace ngeo
