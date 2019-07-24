/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

# pragma once

# include <algorithm>
# include <map>
# include <sstream>
# include <list>

# include <boost/lexical_cast.hpp>
# include <boost/foreach.hpp>

# include <ngeo/lexicon.hpp>

/* ------------------------------------------------------------------------ */
namespace ip { namespace stream {
/** Wrapper for std::string to read only an indentifier from the stream.
    Using a string directly reads until whitespace.
    @code
    std::string s;
    istr >> identifier(s); // read just one identifier token and put it in @a s
    @endcode
 */
struct identifier
{
    identifier(std::string& s)
        : _s(s)
    {
    }

    std::string& _s;
};
}}

namespace std {
    inline std::istream& operator >> (std::istream& str, ip::stream::identifier const& wrap)
    {
        int c;
        while ( ( c = str.peek() ) != EOF ) {
            if (::isalnum(c) || '_' == c) wrap._s += str.get();
            else break;
        }
        return str;
    }

}
/* ------------------------------------------------------------------------ */
