/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

# pragma once

/** @file
    String utilities.
 */

# include <string>
# include <ctype.h>
# include <ngeo/strcmp.hpp>
# include <locale>
# include <boost/functional/hash/hash.hpp>
/* ------------------------------------------------------------------------ */
# if defined(_MSC_VER)
#   pragma warning(push)
#   if NG_STATIC
#     define API
#   else
#     if defined(NETWORK_GEOGRAPHICS_CORE_API)
#       define API _declspec(dllexport)
#     else
#       define API _declspec(dllimport)
#     endif
#     pragma warning(disable:4251)
#   endif
# else
#   define API
# endif
/* ------------------------------------------------------------------------ */
namespace ngeo { namespace util {

/* ------------------------------------------------------------------------ */
/** Skip space in a string until non-space or end of string.
    @a offset is updated to be the offset to the first non-space.
    @return @c true if a non-space was found, @c false otherwise.
 */
inline bool skip_space
    (std::string const& text ///< Input text
    ,size_t& offset         ///< [in/out] Starting offset
    )
{
    size_t n = text.length();
    while (offset < n && isspace(text[offset]))
        ++offset;
    return offset < n;
}

/** Skip space in a string until non-space or end of string.
    @a spot is updated to be the location of the first non-space.
    @return @c true if a non-space was found, @c false otherwise.
 */
inline bool skip_space
    (std::string::const_iterator& spot ///< [in/out] Start of scan range
    ,std::string::const_iterator const& limit ///< End of scan range
    )
{
    while (spot != limit && isspace(*spot))
        ++spot;
    return spot != limit;
}
/* ------------------------------------------------------------------------ */
/** Compare strings ignoring case and locale.
    @return @c true iff @a lhs is equal to @a rhs.
 */
inline bool iequal
    ( std::string const& lhs ///< Left string
    , std::string const& rhs ///< Right string
    )
{
    return 0 == stricmp(lhs, rhs);
}

/** Equality comparator for @c std::string, ignoring case and locale.
    A functor wrapper for @c ngeo::are_equal.
 */
struct ascii_iequal
    : public std::binary_function<std::string, std::string, bool>
{
    /** Compare two strings without regard to case or locale.
        This provides a strict weak ordering.
        @return @c true iff @a left &lt; @a right
     */
    bool operator ()
        ( std::string const& lhs
        , std::string const& rhs
        ) const
    { return iequal(lhs, rhs); }
};
/* ------------------------------------------------------------------------ */
/** Case insensitive ordering for @c std::string with ASCII data only.
    This compares two @c std::string instances without regard to case or locale.
    
    @note This is only safe for use on strings that are known to contain only ASCII data.

    Example:
    @code
        typedef std::set<std::string, ngeo::ascii_iless> case_insensitive_set;
        case_insensitive_set a_set;
    @endcode
 */
struct ascii_iless
    : public std::binary_function<std::string, std::string, bool>
{
    /** Compare two strings without regard to case or locale.
        This provides a strict weak ordering.
        @return @c true iff @a left &lt; @a right
     */
    bool operator()
        ( std::string const& left ///< Left hand side
        , std::string const& right ///< Right hand side
        ) const
    {
        return 0 > stricmp(left, right);
    }
};
/* ------------------------------------------------------------------------ */
/** Case insensitive, locale sensitive ordering for @c std::string.
    This compares two @c std::string instances without regard to case but with
    regard to locale. The locale defaults to the standard locale, but can be
    set in the constructor.

    Example:
    @code
        typedef std::set<std::string, ngeo::string_iless> case_insensitive_set;

        // Use standard locale
        case_insensitive_set std_locale_set;
        // Use a different locale
        std::local other_locale; // initialized omitted for brevity
        case_insenitive_set other_locale_set(other_locale);
    @endcode
 */
struct string_iless
    : public std::binary_function<std::string, std::string, bool>
{
    typedef std::string::value_type element_type; ///< String element type
    std::locale _locale; ///< Local to use for comparison

    /** Default constructor.
        Uses the default locale if @a loc isn't set.
     */
    string_iless
        ( std::locale const& loc = std::locale() ///< Locale to use for case conversion
        )
        : _locale(loc)
    {}

    /** Compare two strings without regard to case.
        This provides a strict weak ordering ignoring string case using locale.
        @return @c true iff @a left &lt; @a right
     */
    bool operator()
        ( std::string const& left ///< Left hand side
        , std::string const& right ///< Right hand side
        ) const
    {
        std::string::const_iterator lspot(left.begin())
                                  , lx(left.end())
                                  , rspot(right.begin())
                                  , rx(right.end())
                                  ;
        for ( ; lspot != lx && rspot != rx ; ++lspot , ++rspot ) {
            element_type l = std::toupper<element_type>(*lspot, _locale);
            element_type r = std::toupper<element_type>(*rspot, _locale);
            if (l < r)
                return true;
            else if (l > r)
                return false;
        }
        // equal as far as the shorter string goes
        // so lhs < rhs iff we ran out of lhs but not out of rhs.
        return lspot == lx && rspot != rx;
    }
};
/* ------------------------------------------------------------------------ */
struct ihash
    : std::unary_function<std::string, std::size_t>
{
    std::size_t operator()(std::string const& x) const
    {
        std::size_t seed = 0;
        std::locale locale;

        for(std::string::const_iterator it = x.begin();
            it != x.end(); ++it)
        {
            boost::hash_combine(seed, std::toupper(*it, locale));
        }

        return seed;
    }
};
/* ------------------------------------------------------------------------ */
}} // namespace ngeo
/* ------------------------------------------------------------------------ */
# undef API
# if defined(_MSC_VER)
#   pragma warning(pop)
# endif
