/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */
/* ------------------------------------------------------------------------ */
# include "ip_local.hpp"
# include "ip_static.hpp"
# include <ctype.h>
# include <iomanip>
# include <assert.h>
/* ------------------------------------------------------------------------ */
// file local utilities
namespace {
    // return true if we found and read the separator
    bool get_non_numeric_separator(std::istream& s, int &c) {
        if (s) {
            s >> std::ws;
            c = s.peek();
            if (c == EOF) {
        	s.clear(s.rdstate() & ~(std::ios::failbit | std::ios::eofbit));
            } else if (!isdigit(c)) {
                s.get();
                s >> std::ws;
                return true;
            }
        }
        return false;
    }

    // return true if we found and read the separator
    bool skip_non_numeric_separator(std::istream& s) {
    	int c;
    	return get_non_numeric_separator(s, c);
    }

# if 0
    // Read from a stream, presuming it to be in octet form. Return false
    // if impropertly formatted. In this case a is left with the partial
    // result. The ip4_mask logic depends on this.
    bool
    read_octets_org(std::istream& s, ngeo::ip4_addr::host_type& a) {
        ngeo::ip4_addr::host_type u;
        int c;
        a = 0;			// set to known state

        for (int i = 0 ; i < 3 && s ; ++i ) {	// get four octets
            s >> std::ws >> u >> std::ws;			// read next octet
            c = s.get();	// eat separator
            a <<= 8;		// shift current value
            a += u & 0xFF;  // add in current value
            if (c != '.' || u > 0xFF) {
                // Hit a missing or bad separator or the value was too big for an octet.
                if ('.' != c && 0 == i) {
                    // If there was no separator at all, then treat the input
                    // as a raw integer specifying the address.
                    a = u;
                    s.clear(s.rdstate() & ~(std::ios::failbit | std::ios::eofbit));
                    return true;
                } else {
                    // It's mis-formatted, mark as failure.
                    s.setstate(std::ios::failbit);
                    return false;
                }
            }
        }

        if (s >> std::ws >> u >> std::ws) { // read the last octet
            assert(u <= 0xFF);
            a <<= 8;	// shift current value
            a += u &0xFF;	// add in current value
            return true;
        }
        return false;
    }
# endif
    enum octet_style_t { OCTETS_INVALID = 0, OCTETS_MASK, OCTETS_CIDR };
    octet_style_t
    read_octets(std::istream& s, ngeo::ip4_addr::host_type& a) {
        ngeo::ip4_addr::host_type v = 0; // current octet value
        int count = 0; // # of octets found.
        int digits = 0; // # of digits in this octet.
        int c; // input character
        octet_style_t zret = OCTETS_INVALID;
        bool fail = false;

        a = 0;

        // Need to peek so we don't consume the next character
        while (s && !fail && EOF != (c = s.peek())) {
            if ('.' == c) {
                s.get();
                if (++count > 3 || v > 255) {
                    fail = true;
                } else {
                    a = (a << 8) + v;
                    digits = 0;
                    v = 0;
                }
            } else if (isdigit(c)) {
                s.get();
                ++digits;
                v = v * 10 + c - '0';
                // allow a single number to represent the address.
                if (v > 255 && count > 0) fail = true;
            } else {
               break;
            }
        }

        a = (a << 8) + v;
        if (!fail && ((0 == count && digits) || 3 == count)) {
            s.clear(s.rdstate() & ~(std::ios::failbit | std::ios::eofbit));
            zret = count ? OCTETS_MASK : OCTETS_CIDR;
        } else {
            s.setstate(std::ios::failbit);
        }
        return zret;
    }

    void
    write_octets(std::ostream& s, ngeo::ip4_addr::host_type a) {
        s   << ((a>>24)&0xFF) << "." << ((a>>16)&0xFF) << "."
            << ((a>>8)&0xFF)  << "." << (a&0xFF);
    }

    void
    write_octets(std::ostream& s, ngeo::ip4_addr::host_type a, int w) {
        s   << std::setw(w) << std::setfill(' ') << ((a>>24)&0xFF) << "."
            << std::setw(w) << std::setfill(' ') << ((a>>16)&0xFF) << "."
            << std::setw(w) << std::setfill(' ') << ((a>>8)&0xFF)  << "."
            << std::setw(w) << std::setfill(' ') << (a&0xFF);
    }

    template <typename T> std::istream&
    read_range(std::istream& s, T& p) {
        typedef typename T::metric_type METRIC;
        METRIC l, h;	// temporaries

        if (s >> std::ws) {
            if (!isdigit(s.peek())) { // of the form "-###"
                l = METRIC::MIN;
	        s.get(); // skip separator
	        s >> h;
            } else if (s >> l) { // no leading separator
                if (skip_non_numeric_separator(s)) {
                    if (isdigit(s.peek())) {
                        s >> h;
                    } else { // "###-"
                        h = METRIC::MAX;
            		s.clear(s.rdstate() & ~(std::ios::failbit | std::ios::eofbit));
                    }
                } else
                    h = l; // assume it's a singleton
            }
            if (s) p.set(l, h);
        }
	return s;
    }

    /**
     * Pulling out of the general range reader, so we can deal with both networks and ranges
     * as string inputs to ranges
     */
    std::istream&
    read_ip4_range(std::istream& s, ngeo::ip4_range& p) {
        typedef ngeo::ip4_range::metric_type METRIC;
        METRIC l, h;	// temporaries

        if (s >> std::ws) {
            if (!isdigit(s.peek())) { // of the form "-###"
                l = METRIC::MIN;
                s.get(); // skip separator
                s >> h;
            } else if (s >> l) { // no leading separator
            	int c;
            	if (get_non_numeric_separator(s, c)) {
            	    if (c == '/') {	// This is the network form
	            	ngeo::ip4_mask mask;
	            	if (s >> mask) {
		            ngeo::ip4_net net(l, mask);
		            l = net.addr();
		            h = net.max_addr();
	            	}
	            } else { // Assume it is a dash and proceed with the straight network logic
	                if (isdigit(s.peek())) {
	                    s >> h;
	                } else { // "###-"
	                    h = METRIC::MAX;
	                    s.clear(s.rdstate() & ~(std::ios::failbit | std::ios::eofbit));
	                }
	            }
                } else {
                    h = l; // assume it's a singleton
                }
	    }
	    if (s) p.set(l, h);
	}
	return s;
    }
}
/* ------------------------------------------------------------------------ */
namespace ngeo {
using boost::lexical_cast;

/* ------------------------------------------------------------------------ */

std::ostream&
operator << (std::ostream& s, ip_port const& p) {
    return s << p.host_order();
}

std::istream&
operator >> (std::istream& s, ip_port& p) {
    ip_port::host_type port;
    if (s >> port) p.set(port);
    return s;
}

ip_port::lexicon_type& ip_port::get_lexicon() { return PORT_LEXICON; }
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
// Instantiate the base template code.
# if defined(_MSC_VER)
template struct interval<ip_port>;
# endif

ip_port_range::ip_port_range(std::string const& str) {
    *this = boost::lexical_cast<self>(str);
}

std::ostream& operator << (std::ostream& s,  const ip_port_range& p) {
    return s << p.min() << ip_port_range::SEPARATOR << p.max();
}

std::istream& operator >> (std::istream& s, ip_port_range& p) {
    return read_range<ip_port_range>(s, p);
}
/* ------------------------------------------------------------------------ */

ip4_addr::ip4_addr(std::string const& s) {
    *this = lexical_cast<self>(s);
}

bool ip4_addr::is_valid(std::string const &addr_str) {
    std::istringstream s(addr_str);
    ip4_addr::host_type a;
    return read_octets(s, a) != OCTETS_INVALID;
}

int
ip4_addr::msb_count(bool set) const {
    /*  Probably a bit over the top, but I always worry about right shifts
        and what comes in at the top. This constant lets us force the
        MSB to be zero.
     */
    static const host_type ZERO_MSB = ~(host_type(1) << (WIDTH-1));
    host_type m = ~ZERO_MSB;
    unsigned int zret = 0;

    while (zret < WIDTH && (((m & _addr) != 0) == set)) {
        ++zret;
        m >>= 1;
        m &= ZERO_MSB;
    }
    return zret;
}

int
ip4_addr::lsb_count(bool set) const {
    host_type m(1);
    unsigned int zret = 0;
    while (zret < WIDTH && (((m & _addr) != 0) == set)) {
        ++zret;
        m <<= 1;
    }
    return zret;
}

std::istream&
operator >> (std::istream& s, ip4_addr& addr) {
    ip4_addr::host_type a;
    if (read_octets(s, a)) addr.set(a);
    return s;
}

std::ostream&
operator << (std::ostream& s,  const ip4_addr& addr) {
    write_octets(s, addr.host_order());
    return s;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
ip4_mask::ip4_mask(std::string const& s) {
    *this = lexical_cast<self>(s);
}

std::istream&
operator >> (std::istream& s, ip4_mask& m) {
    // SKH - note that we are using ip4_addr::host_type here instead of ip4_mask::host_type
    // because we're reading the mask in octet form, not CIDR.
    ip4_addr::host_type u;
    octet_style_t style = read_octets(s,u);
    if (OCTETS_CIDR == style && u <= ip4_mask::WIDTH) {
            m.set(u);
    } else if (OCTETS_INVALID != style) {
        // Either it's octet form, or it's a raw number that's more than 32.
        m = ip4_mask(ip4_addr(u)); // force octet interpretation, not CIDR.
    } else {	// just not right.
        m.set(0);
    }
    return s;
}

std::ostream&
operator << (std::ostream& s, ip4_mask const& mask){
    return s << mask.count();
}
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
ip4_net::ip4_net(std::string const& s) {
    *this = lexical_cast<self>(s);
}

// need to use temporaries so that we can guarantee that the address and
// mask are compatible.
std::istream& operator >> (std::istream& s, ip4_net& net) {
    ip4_addr a;
    ip4_mask m = 32;

    if (!s) return s; // bad input, abort
    s >> std::ws;

    if (s.peek() == ip4_net::EMPTY_CHAR) {
        // if the leading char is the invalid char then either it's the invalid notation
        // or it's just wrong. So we succeed if we read invalid and fail otherwise.
        char c;
        s.get(); // drop leading invalid char
        if (s >> std::ws && s >> c && ip4_net::SEPARATOR == c && s >> std::ws && s >> c && ip4_net::EMPTY_CHAR == c)
            net = ip4_net(); // empty network
        else // bad input
    	    s.setstate(std::ios::failbit);
    } else if (s >> a) {
    	if (skip_non_numeric_separator(s)) {
	    if (s >> m) // validate good read
	        net.set(a,m);
	    else
	        s.setstate(std::ios::failbit);
    	} else
	    net.set(a, m);
    }
    return s;
}

std::ostream& operator << (std::ostream& s, const ip4_net& net) {
    if (net.is_empty())
        return s << ip4_net::EMPTY_CHAR << ip4_net::SEPARATOR << ip4_net::EMPTY_CHAR;
    else
    	return s << net.addr() << ip4_net::SEPARATOR << net.mask();
}
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

ip4_range::ip4_range(std::string const& s) {
    *this = lexical_cast<self>(s);
}

//! Get an iterator for the first network in the network cover of the range.
ip4_range::net_iterator
ip4_range::net_begin() const {
    return net_iterator(*this);
}

//! Get an iterator past the end of the network cover of the range.
ip4_range::net_iterator
ip4_range::net_end() const {
    return net_iterator();
}


/*  Compute the next network in the unique minimal network covering of the
    range.

    Basically, peel off the largest valid network from the front of the range.
    This greedy algorithm will always provide the unique minimal network cover.
    The two bounds on the size of the network are starting address and the
    size of the remaining range.
 */
bool
ip4_range::extract_next_network(ip4_net& net) {
    // fail somewhat gracefully for an empty range
    if (!*this) {
        net = ip4_net();
        return false;
    }

    int width(_min.lsb_count(false));
    net.set(_min, ip4_mask::WIDTH - width); // largest possible network
    // if it's too big, shrink it till it fits
    while (net.max_addr() > _max)
        net = ip4_net(_min, net.mask() >> 1);

    /*  Need to be careful about wrapping during state update.
        If the max address is in the net, we've done everything so reset
        the range to empty. Otherwise, set the minimum to one past the end
        of the network. We know that won't wrap because it's strictly less
        than _max.
     */
    _min = net.max_addr();
    if (_min == _max) *this = ip4_range();
    else ++_min;

    return !*this;
}

/*  The range is a network if the size is 2^k and no more than the size of the
    maximum network based at the minimum value.
 */
bool
ip4_range::is_network() const {
    if (*this) {
        ip4_addr::host_type size = _max.host_order() - _min.host_order() + 1;
        int size_k = ip4_addr(size).lsb_count(0); //
        if (size_k == 32 || size == (unsigned int)(1 << size_k )) { // size is 2^k
            return size_k <= _min.lsb_count(0); // size not greater than largest network at _min
        }
    }
    return false;
}

std::istream&
operator >> (std::istream& s, ip4_range& r) {
    return read_ip4_range(s, r);
}


std::ostream& operator << (std::ostream& s, const ip4_range& r) {
    return s << r.min() << ip4_range::SEPARATOR << r.max();
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
ip4_pepa::ip4_pepa(std::string const& s) {
    *this = lexical_cast<self>(s);
}

std::istream& operator >> (std::istream& s, ip4_pepa& pepa) {
    ip4_addr addr;
    ip4_mask mask;

    if (s && s >> addr && skip_non_numeric_separator(s) && s >> mask) {
        pepa.set(addr,mask);
    }
    return s;
}

std::ostream& operator << (std::ostream& s, ip4_pepa const& pepa) {
    return s << pepa.addr() << ip4_pepa::SEPARATOR << pepa.mask();
}
/* ------------------------------------------------------------------------ */

std::string icmp_type::get_name() const { return ICMP_LEXICON[_value]; }
icmp_type::lexicon_type& icmp_type::get_lexicon() { return ICMP_LEXICON; }

std::istream& operator >> (std::istream& s, icmp_type& i) {
    /*  We assume that none of the names start with a digit. So check
        the first character. If a digit, presume numeric input. Otherwise
        if it's a letter presume a name (anything else is bogus).
     */
    icmp_type::host_type v(icmp_type::INVALID);
    if (s >> std::ws) {
        int c = s.peek();
        if (isdigit(c)) {
            s >> v >> std::ws;
        } else if (isgraph(c)) {
            std::string name;
            if (s >> ip::stream::identifier(name) >> std::ws) {
                std::transform(name.begin(), name.end(), name.begin(), toupper); // force upper case
                v = ICMP_LEXICON[name];
            }
        }
    }
    // If =v= was set to a valid value, the input was good so store it.
    // Otherwise mark the input failure.
    if (s && icmp_type::is_valid(v))
        i._value = v;
    else
        s.setstate(std::ios::failbit);
    return s;
}
/* ------------------------------------------------------------------------ */
std::string ip4_protocol::get_name() const { return IP_PROTOCOL_LEXICON[*this]; }
ip4_protocol::lexicon_type& ip4_protocol::get_lexicon() { return IP_PROTOCOL_LEXICON; }
/* ------------------------------------------------------------------------ */
std::istream& operator >> (std::istream& s, ip_protocol & p) {
    if (s >> std::ws) {
        int c = s.peek();
        if (isdigit(c)) {
            ip_protocol::host_type v;
            if (s >> v >> std::ws)
                p._value = ip_protocol::bound(v);
        } else if (isgraph(c)) {
            std::string name;
            if (s >> ip::stream::identifier(name) >> std::ws) {
                std::transform(name.begin(), name.end(), name.begin(), toupper); // force upper case
                p._value = IP_PROTOCOL_LEXICON[name];
            }
        } else {
            s.setstate(std::ios::failbit);
        }
    }
    return s;
}
/* ------------------------------------------------------------------------ */
# if defined(_DEBUG)
// This class just does various numeric checking
class ValidateNumerics
{
public:
	ValidateNumerics()
	{
		// check that ip_port fundamental types has the right # of bits
		assert(std::numeric_limits<ip_port::host_type>::digits == ip_port::WIDTH);
		// check that ip4_addr fundamental types has the right # of bits
		assert(std::numeric_limits<ip4_addr::host_type>::digits == ip4_addr::WIDTH);
	}
};

namespace { ValidateNumerics Validation; }
# endif
/* ------------------------------------------------------------------------ */
} // namespaces
/* ------------------------------------------------------------------------ */
