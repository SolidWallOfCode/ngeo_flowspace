/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

/* ------------------------------------------------------------------------ */
# pragma once
# include <iosfwd>
# include <limits>
# include <string>
# include <sstream>
# include <vector>
# include <ngeo/numeric_type.hpp>
# include <ngeo/interval.hpp>
# if !defined(_MSC_VER)
#   include <endian.h>
# endif
/* ------------------------------------------------------------------------ */
#   if defined(_MSC_VER)
#       if NG_STATIC
#           define API
#       else
#           if defined(NETWORK_GEOGRAPHICS_IP_API)
#               define API _declspec(dllexport)
#           else
#               define API _declspec(dllimport)
#           endif
#       endif
#   else
#       define API __attribute__ ((visibility("default")))
#       define LOCAL __attribute__ ((visibility("hidden")))
#   endif
/* ------------------------------------------------------------------------ */
namespace ngeo { 
/* ------------------------------------------------------------------------ */
class ip4_mask;	// used in ip4_addr ctor
class ip4_net;	// declared friend in ip4_mask
class ip4_pepa;	// declared friend in ip4_mask
// Declared in name only so that clients do not need to include this header
// to use the IP library. If you want to access the API elements that use
// @c lexicon you must include the @c ngeo/lexicon.hpp header.
template < typename T > class lexicon;
/* ------------------------------------------------------------------------ */
/** @file
    Classes for working with basic Internet Protocol data.

    All internal data is stored in host order. This can cause problems when
    used for data in external situations, but overall it was considered
    easier to do those conversions when necessary. The methods @c host_order
    and @c network_order are provided for convenience in this regard.
 */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/** A UDP/TCP port.
    This class is @ref totally_ordered.
 */
class API ip_port : public
    boost::totally_ordered <
        ip_port,
        boost::unit_steppable <
            ip_port,
            boost::additive <
                ip_port,
                boost::additive < 
                    ip_port,
                    unsigned short
    > > > > {
public:
    typedef ip_port self;               //!< self reference type
    typedef unsigned short host_type;	//!< implementation type
    typedef ngeo::lexicon<self> lexicon_type;      ///< Name associations

    //! The width of the type in bits
    static unsigned int const WIDTH = 16;
    //! A bit mask for half of the data
    static unsigned int const HALF_MASK = ~(~static_cast<host_type>(0) << (WIDTH/2));

    //! Default constructor.
    ip_port() : _port(0) {
    }

    //! Construct from @c host_type.
    ip_port(
        host_type port //!< [in] host order port value
    )
        : _port(port) {
    }

    // use compiler generated copy and assignment

    //! The port value in host type and order.
    host_type host_order() const {
        return _port;
    }

    //! The port value in host type and network order.
    host_type network_order() const {
        return hton(_port);
    }

    //! Reset the data from native format.
    void set(
        host_type p //!< [in] host order port value
    ) {
        _port = p;
    }

    //! Static instance with the minimum port value
    static self const MIN;
    //! Static instance with the maximum port value
    static self const MAX;
    /// Mark as having built in MIN/MAX members.
    struct static_MIN_MAX_tag;

    /// @name Host <-> network conversions.
    //@{
    /// Convert from network order to host order.
    static host_type ntoh(host_type x);
    /// Convert from host order to network order.
    static host_type hton(host_type x);
    //@}

    /// @name Numeric operators
    //@{
    //! Add a port to this port.
    self& operator += (self const& rhs) {
        _port += rhs._port;
        return *this;
    }

    //! Subtract a port from this port.
    self& operator -= (self const& rhs) {
        _port -= rhs._port;
        return *this;
    }

    //! Add a host value to this port.
    self& operator += (host_type rhs) {
        _port += rhs;
        return *this;
    }

    //! Subtract a host value from this port.
    self& operator -= (host_type rhs) {
        _port -= rhs;
        return *this;
    }

    //! Pre-increment operator
    self& operator++ () {
        ++_port;
        return *this;
    }

    //! Pre-decrement operator
    self& operator -- () {
        --_port;
        return *this;
    }
    //@}

    static lexicon_type& get_lexicon();
protected:
    host_type _port; //!< The port value.
};

//! Alias for convenience, since IPv4 and IPv6 UDP/TCP ports are identical.
typedef ip_port ip4_port;

/// @cond NOT_DOCUMENTED
// These are here to avoid operator ambiguity. No need to clutter the documenation.
/** Two ports are equal if their port values are equal.
    @relates ip_port
 */
inline bool
operator == (ip_port const& lhs, ip_port const& rhs) {
    return lhs.host_order() == rhs.host_order();
}

/** Two ports are not equal if their port values are not equal.
    @relates ip_port
 */
inline bool
operator <  (ip_port const& lhs, ip_port const& rhs) {
    return lhs.host_order() < rhs.host_order();
}
/// @endcond

/** Write port @a p to stream @a s.
    @relates ip_port
 */
API std::ostream& operator << (
    std::ostream& s, //!< [in,out]
    ip_port const& p //!< [in]
);
/** Read port from stream.
    @relates ip_port
 */
API std::istream& operator >> (
    std::istream& s, //!< [in,out]
    ip_port& p       //!< [out]
);

// These are defined outside the class because of the conditional compilation
# if (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN) || defined(_M_IX86)
inline ip_port::host_type ip_port::ntoh(host_type x) { return static_cast<host_type>( ((x >> (WIDTH/2)) & HALF_MASK) | ((x & HALF_MASK) << (WIDTH/2)) ); }
inline ip_port::host_type ip_port::hton(host_type x) { return ntoh(x); }
# else
inline ip_port::host_type ip_port::ntoh(host_type x) { return x; }
inline ip_port::host_type ip_port::hton(host_type x) { return x; }
# endif

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
//! @cond NOT_DOCUMENTED
// Enable numeric interval methods.
namespace detail {
    template <> struct subtraction_trait<ip_port> : public std::minus<ip_port> {};
    template <> struct addition_trait<ip_port> : public std::plus<ip_port> {};
}
// This lets us put the template instantation in our library rather than
// duplicating the code in all clients.
template struct API interval<ip_port>;
//! @endcond

/// A range of @c ip_port values.
class API ip_port_range
    : public interval<ip_port> {
public:
    typedef ip_port_range self;         //!< Self reference type.
    typedef interval<ip_port> super;    //!< Super class reference type.

    /// Delimiter used between the minimum and maximum values.
    static char const SEPARATOR = '-';

    /** Default constructor.
        The constructed range is empty.
     */
    ip_port_range() : super() {
    }

    /** Copy constructor from the super class.
        @internal This handles co-variance to some extent. Super class
        operators / methods that return temporaries will return instances
        of @c super. This provides a user conversion from that instance
        to an instance of this class so that those operators / methods
        work as expected by the client. This can be automatic because
        we add only methods to the super class.
     */
    ip_port_range(super const& s) : super(s) {
    }

    /** Construct the inclusive range of two ports.
     */
    ip_port_range(
        ip_port const& lower,   //!< [in] the lesser endpoint of the range
        ip_port const& upper    //!< [in] the greater endpoint of the range
    )
        : super(lower,upper) {
    }

    /** Construct a range that contains the single value @a port.
     */
    ip_port_range(
        ip_port const& port //!< [in] Singleton port for the range.
    )
        : super(port) {
    }

    /** Construct from a string.
        If @a str is invalid the range is default constructed.
        @note This can be done with @c lexical_cast but clients like this
        kind of syntatic sugar.
     */
    ip_port_range(
        std::string const& str //!< [in] Text representation of the range.
    );
};

/** Alias because IPv4 and IPv6 ports are identical.
 */
typedef ip_port_range ip4_port_range;

/** Write range @a r to stream @a s.
    The range is written as "MIN-MAX" where MIN is the minimum port value and
    MAX is the maximum port value.
    @relates ip_port_range
 */
API std::ostream& operator << (
    std::ostream& s,       //!< [in,out]
    ip_port_range const& r //!< [in]
);
/** Read range from stream.
    The input should be in the form "MIN-MAX" where MIN is the minimum port
    value and MAX is the maximum port value. Other variations that are supported
    are
    - "#" : treated as the range "#-#", i.e. a singleton range containing #.
    - "-#" : treated as the range @c ip_port::MIN to #.
    - "#-" : treated as the range # to @c ip_port::MAX.
    @relates ip_port_range
 */
API std::istream& operator >> (
    std::istream& s,  //!< [in,out]
    ip_port_range& r  //!< [out]
);
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/** IPv4 address.
    This stores a single IPv4 address. It is internally consistent and can
    be accessed in network or host order.
    This class is @ref totally_ordered.
 */
class API ip4_addr
    : public boost::totally_ordered <
        ip4_addr,
        boost::unit_steppable <
            ip4_addr,
            boost::additive <
                ip4_addr,
                unsigned int,
                boost::shiftable <
                    ip4_addr,
                    unsigned int,
                    boost::bitwise <
                        ip4_addr,
                        boost::bitwise <
                            ip4_addr,
                            unsigned int
    > > > > > > {
public:
    typedef ip4_addr self;              //!< standard self reference type
    // NB: If this is changed, it must also be changed in the boost operator templates
    typedef unsigned int host_type;     //!< local implementation type for address

    //! The size of the host type in bits.
    static unsigned int const WIDTH = 32;
    //! A mask for half of the host type.
    static unsigned int const HALF_MASK = ~(~static_cast<host_type>(0) << (WIDTH/2));

    //! Default constructor.
    ip4_addr() : _addr(0) { }
    //! Host type constructor.
    ip4_addr(
        host_type t //!< Address in host type and order
    )
        : _addr(t) {
    }

    /** Construct from a network mask.
        Only the type changes, the underlying value is preserved.
     */
    ip4_addr(
        ip4_mask const& mask //!< Mask value
    ); // definition delayed until we can call ip4_mask methods

    /// Construct from octet string.
    ip4_addr(
        std::string const& s ///< String representation of address.
    );
    
    static bool is_valid(std::string const &s);

    /** Rewrite address value.
        Set the current address to a new value, specified in host order.
        @return Self reference for chaining.
     */
    self& set(
        host_type a ///< New address.
    ) {
        _addr = a;
        return *this;
    }

    //! The address in host type and order.
    host_type host_order() const {
        return _addr;
    }
    //! The address in host type and network order.
    host_type network_order() const {
        return hton(_addr);
    }

    /// Minimum IP4 address value
    static self const MIN;
    /// Maximum IP4 address value
    static self const MAX;
    /// Mark as having built in MIN/MAX members.
    struct static_MIN_MAX_tag;

    /// @name Network <-> Host address conversions
    //@{
    /// Convert from network order to host order.
    static host_type ntoh(host_type x);
    /// Convert from host order to network order.
    static host_type hton(host_type x);
    //@}

    /// Access octet.
    /// Octets are indexed with the MSB as 0 and the LSB as 3.
    /// @return The octet at the @a index.
    unsigned char operator [] (size_t index) const { return static_cast<unsigned char>(index > 2 ? _addr&0xFF : ((_addr >> 8*(3-index))&0xFF)); }

    /// @name Numeric operators
    //@{
    //! Left shift.
    self& operator <<= (
        unsigned int n //!< Number of bit positions to shift.
    ) {
        _addr <<= n;
        return *this;
    }
    //! Right shift.
    self& operator >>= (
        unsigned int n  //!< Number of bit positions to shift.
    ) {
        _addr >>= n;
        return *this;
    }
    //! Bitwise complement.
    self operator ~ () const {
        self tmp(~_addr);
        return tmp;
    }
    //! Pre-increment
    self& operator ++  () {
        ++_addr;
        return *this;
    }
    //! Pre-decrement
    inline self& operator --  () {
        --_addr;
        return *this;
    }
    //! Add an offset to an address
    self& operator += (
        host_type offset ///< Value to add.
    ) {
        _addr += offset;
        return *this;
    }
    //! Subtract an offset from an address
    self& operator -= (
        host_type offset ///< Value to substract.
    ) {
        _addr -= offset;
        return *this;
    }

    //! Bitwise and
    self& operator &= (
        self const& rhs ///< Right operand.
    ) {
        _addr &= rhs._addr;
        return *this;
    }

    //! Bitwise or
    self& operator |= (
        self const& rhs ///< Right operand.
    ) {
        _addr |= rhs._addr;
        return *this;
    }

    //! Bitwise xor
    self& operator ^= (
        self const& rhs ///< Right operand.
    ) {
        _addr ^= rhs._addr;
        return *this;
    }

    //! Bitwise and
    self& operator &= (
        host_type rhs ///< Right operand.
    ) {
        _addr &= rhs;
        return *this;
    }

    //! Bitwise or
    self& operator |= (
        host_type rhs ///< Right operand.
    ) {
        _addr |= rhs;
        return *this;
    }

    //! Bitwise xor
    self& operator ^= (
        host_type rhs ///< Right operand.
    ) {
        _addr ^= rhs;
        return *this;
    }

    //@}

    /** Most significant bit count.
        @return The number of bits, starting with the MSB,
        that are set (@a set is @c true) or reset (@a set is @c false).
     */
    int msb_count(
        bool set ///< Target bit value.
    ) const;

    /** Least significant bit count.
        @return The number of bits, starting with the LSB,
        that are set (@a set is @c true) or reset (@a set is @c false).
     */
    int lsb_count(
        bool set ///< Target bit value.
    ) const;

private:
    host_type _addr;   //!< Storage for the address.
};

//! Equality.
inline bool operator == (
    ip4_addr const& lhs, ///< Left operand.
    ip4_addr const& rhs ///< Right operand.
) {
    return lhs.host_order() == rhs.host_order();
}

//! Addresses are ordered by numeric value in host order.
inline bool operator <  (
    ip4_addr const& lhs, ///< Left operand.
    ip4_addr const& rhs ///< Right operand.
) {
    return lhs.host_order() < rhs.host_order();
}

// Defined outside of class because of the conditional compilation
# if defined(_M_IX86)
// Abuse the fact that an address is exactly twice as wide as a port
inline ip4_addr::host_type ip4_addr::ntoh(host_type x) { return ip_port::ntoh(static_cast<ip_port::host_type>(x >> ip_port::WIDTH)) | (ip_port::ntoh(static_cast<ip_port::host_type>(x & HALF_MASK)) << ip_port::WIDTH); }
inline ip4_addr::host_type ip4_addr::hton(host_type x) { return ntoh(x); }
# else
inline ip4_addr::host_type ip4_addr::ntoh(host_type x) { return x; }
inline ip4_addr::host_type ip4_addr::hton(host_type x) { return x; }
# endif

/** Write address to stream in octet form.
    @relates ip4_addr
 */
API std::ostream& operator << (
    std::ostream& s, ///< Output stream.
    ip4_addr const& a ///< Output value.
);

/** Read address from stream.
    This expects the address in octet form.
    @relates ip4_addr
 */
API std::istream& operator >> (
    std::istream& s, ///> Input stream.
    ip4_addr& a ///< Input target.
);
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/** Store an IPv4 network mask.
    This class stores the mask and provides various utility operations on the
    mask, some in conjuction with @c ip4_addr.

    Only CIDR style masks are stored. This means that all masks consist of a
    contiguous sequence of set bits, starting at the most significant bit,
    followed by a contiguous sequence of zero bits to the least significant bit.
    For this reason it is always possible to convert
    from a mask to an address but converting from an address to a mask may lose
    bits.

    This class is @ref totally_ordered.
 */
class API ip4_mask
    : public
    boost::totally_ordered<ip4_mask,
    boost::orable<ip4_mask,
    boost::andable<ip4_mask,
    boost::shiftable<ip4_mask, unsigned int
    > > > >
{
public:
    typedef ip4_mask self;      //!< standard self reference type
    typedef int host_type;  //!< the built in used for storage

    // !! This must be @c unsigned @c int or it won't link properly with gcc !!
    //static unsigned int const WIDTH = ip4_addr::WIDTH; //!< width in bits of a mask
    static unsigned int const WIDTH = 32; //!< width in bits of a mask

    //! Default constructor. Mask is constructed as all zero bits.
    ip4_mask() : _count(0)
    { }

    /** Construct mask from bit count.
        @note To initialize from an address, use the constructor taking
        an @c ip4_addr.
     */
    ip4_mask(host_type count) : _count(bounded_count(count))
    { }

    /** Construct from an address.
        @note This uses the initial sequence of set bits from the address. Any
        bits in the address from the highest zero bit on down are ignored.
     */
    explicit ip4_mask(ip4_addr const& a) : _count(a.msb_count(true))
    { }

    /** Construct from a string.
        The string can be in either octet form or CIDR form.
     */
    ip4_mask(std::string const& str);

    /** Create a mask for an address.
        @return The least specific mask that will cover all bits set in the address,
        i.e. such that @a mask & @a addr == @a addr.
        @note This is the same as the constructor if the address is a valid mask,
        but quite different if the address is not.
     */
    static ip4_mask cover(ip4_addr const& a) { return ip4_mask(WIDTH - a.lsb_count(false)); }

    //! Rewrite mask with new value.
    void set(
        host_type count //!< New mask bit count.
        )
    {
        _count = bounded_count(count);
    }

    //! Force a value to be a valid bit count.
    static host_type bounded_count(host_type count)
    {
        return std::min(static_cast<host_type>(WIDTH), std::max(0, count));
    }

    /** Shift the mask left @a n bits.
        @note Zero bits are shifted in.
     */
    self& operator<<= (unsigned int n)
    {
        _count = bounded_count(_count - n);
        return *this;
    }

    /** Shift the mask right @a n bits.
        @note Set bits are shifted in.
     */
    self& operator>>= (unsigned int n)
    {
        _count = bounded_count(_count + n);
        return *this;
    }

    //! Bitwise and.
    self& operator &= (self const& rhs)
    {
        _count = std::min(_count,rhs._count);
        return *this;
    }

    //! Bitwise or.
    self& operator |= (self const& rhs)
    {
        _count = std::max(_count,rhs._count);
        return *this;
    }

    //! Get the bit count for the mask.
    host_type count() const
    {
        return _count;
    }

    //! Get the mask in host order format
    ip4_addr::host_type host_order() const
    {
        return _count ? ~static_cast<ip4_addr::host_type>(0) << (WIDTH - _count) : 0;
    }

    //! Get the mask in network order format
    ip4_addr::host_type network_order() const
    {
        return ip4_addr::hton(this->host_order());
    }

private:
    host_type _count;
};

//! Two masks are equal if their bit counts are equal.
inline bool
operator == (ip4_mask const& lhs, ip4_mask const& rhs)
{
    return lhs.count() == rhs.count();
}

/** Ordering.
    Masks are ordered by the cardinality of the mask, i.e. the number of
    addresses covered by the mask. The term "specific" is used as an
    antonymn for the cardinality, so that more specific masks cover fewer
    addresses. This means that masks are ordered from most specific to
    least specific.
    */
inline bool
operator <  (ip4_mask const& lhs, ip4_mask const& rhs)
{
    return lhs.count() < rhs.count();
}

/** Bitwise exclusive or.
    @return The address value resulting from the bitwise exclusive or
    of the two masks.
    @note This returns an address, not a mask, as it will never be the
    case that the bitwise exclusive or of two masks will be a valid mask.
    */
inline ip4_addr
operator ^ (ip4_mask const& lhs, ip4_mask const& rhs)
{
    ip4_addr zret(lhs);
    zret ^= rhs;
    return zret;
}

/** Bitwise complement.
    This yields an @c ip4_addr because in almost every case the bitwise
    complement is not a valid mask.
 */
inline ip4_addr
operator ~ (ip4_mask const& mask)
{
    ip4_addr zret(~(mask.host_order()));
    return zret;
}

/** Bitwise and of an address and mask.
    A common operation worth an overload.
 */
inline ip4_addr
operator & (ip4_addr const& addr, ip4_mask const& mask)
{
    ip4_addr zret(mask);
    zret &= addr;
    return zret;
}

/** Bitwise and of an address and mask.
    A common operation worth an overload.
 */
inline ip4_addr
operator & (ip4_mask const& mask, ip4_addr const& addr)
{
    ip4_addr zret(mask);
    zret &= addr;
    return zret;
}

/** Write the mask to a stream.
    The mask is written in CIDR format.
 */
API std::ostream& operator << (std::ostream&,  ip4_mask const&);
/** Read the mask from a stream.
    The mask can be in either CIDR or octet format.
 */
API std::istream& operator >> (std::istream& s, ip4_mask& a);

// needed to delay this definition until @c ip4_mask is defined
inline ip4_addr::ip4_addr(ip4_mask const& m) : _addr(m.host_order()) { }
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/** Class for storing an IP network.
    This stores an IP network, which consists of an IP address and an IP
    mask. The network address is automatically modified to be compatible with
    the mask. If you need to store the full address along with its corresponding
    network, use the @c ip4_pepa (Protocol End Point Address) class.
 */
class API ip4_net
    : public
    boost::totally_ordered<ip4_net>
{
public:
    typedef ip4_net self;   //!< Standard self reference type

    /** The character used as a separator between the address and mask
        in the string form.
     */
    static char const SEPARATOR = '/';

    //! The character used to represent an empty network.
    static char const EMPTY_CHAR = '*';

    //! Default constructor. The network is empty.
    ip4_net() : m_addr(ip4_addr::MAX), m_mask(0)
    { }

    /** Construct from address and mask.
        @note The stored address is modified to be compatible with the mask.
     */
    ip4_net(
        ip4_addr const& addr, //!< address of network
        ip4_mask const& mask  //!< mask of network
        ) : m_addr(addr & mask), m_mask(mask)
    { }

    /** Construct from an address.
        The constructed network is a singleton network, containing only @a addr.
        @note Marked explicit because this conversion isn't always what is expected.
     */
    explicit ip4_net(
        ip4_addr const& addr
        ) : m_addr(addr), m_mask(ip4_addr::WIDTH)
    {}

    /** Construct from string.
        The netmask can be in octet or CIDR format.
     */
    ip4_net(std::string const& s);

    //! Reset the network.
    self& set(
        ip4_addr const& addr, //!< address of network
        ip4_mask const& mask  //!< mask of network
        )
    {
        m_addr = addr & mask;
        m_mask = mask;
        return *this;
    }

    //! User conversion to an IP address
	operator ip4_addr() const
    { return m_addr; }
    //! User conversion to an IP mask
	operator ip4_mask() const
    { return m_mask; }

    /** The network address.
        @note This is also the minimum address in the network.
     */
	ip4_addr addr() const
    { return m_addr; }

    /** The network mask. */
	ip4_mask mask() const
    { return m_mask; }

    //! The maximum address in the network
	ip4_addr max_addr() const { return m_addr | ~m_mask; }

    //! Test if the network is the empty network.
    bool is_empty() const
    {
        return m_mask.count() == 0 && m_addr != ip4_addr::MIN;
    }

    /** Check if the address is in the network.
        @return @c true if @a addr is an address in the network.
     */
    bool contains(ip4_addr const& addr) const
    {
        return (addr & m_mask) == m_addr;
    }

    /** Test for network containment.
        @return @c true if @c this network is a strict subset of @a that network.
        @note Because of the structure of IP networks, two networks will always
        have one of three relationships:
            - disjoint (no addresses in common)
            - identity (the networks are identical)
            - subset (all addresses in one network are also in the other network)
     */
    bool is_strict_subset_of(ip4_net const& that) const
    {
        return ((m_addr & that.m_mask) == that.m_addr) && (that.m_mask < m_mask);
    }

    /** Test for network containment or equality.
        @return @c true if every address in @c this network is also in @a that.
     */
    bool is_subset_of(ip4_net const& that) const
    {
        return ((m_addr & that.m_mask) == that.m_addr) && (that.m_mask <= m_mask);
    }

    /** Test for network containment.
        @return @c true if @c this is a strict superset of @a that.
     */
    bool is_strict_superset_of(self const& that) const
    {
        return that.is_strict_subset_of(*this);
    }

    /** Test for network containment or equality.
        @return @c true if @c this contains every address in @a that.
     */
    bool is_superset_of(self const& that) const
    {
        return that.is_subset_of(*this);
    }

    /** Test for common addresses between networks.
        @return @c true if there exists at least one address that is in both networks.
     */
    bool has_intersection(ip4_net const& that) const
    {
        /*  For IPv4 networks, all networks that intersect must have the
            same initial bit sequence for the network address.
         */
        ip4_mask m(std::min(m_mask, that.m_mask));
        return (m_addr & m) == (that.m_addr & m);
    }

    /** Functor for lexicographic ordering.
        If, for some reason, networks need to be put in a container
        that requires a strict weak ordering, the default operator < will
        not work. Instead, this functor should be used as the comparison
        functor. E.g.
        @code
        typedef std::set<ip4_net, ip4_net::lexicographic_order> container;
        @endcode
        This ordering is not expected to be useful as an ordering but only 
        to satisfy a common STL container requirement.

        @note Lexicographic ordering is a standard tuple ordering where the
        order is determined by pairwise comparing the elements of both tuples.
        The first pair of elements that are not equal determine the ordering
        of the overall tuples. In this case, the network is treated as a 2-tuple
        of the minimum and maximum addresses contained in the network.
     */
    struct lexicographic_order
        : public std::binary_function<self, self, bool>
    {
        //! Comparison operator.
        bool operator () (self const& lhs, self const& rhs) const {
            return lhs.m_addr == rhs.m_addr
                ? lhs.m_mask > rhs.m_mask
                : lhs.m_addr < rhs.m_addr
                ;
        }
    };

private:
    ip4_addr m_addr;//!< The network address
    ip4_mask m_mask;//!< The network mask

    friend struct lexicographic_order;
};

/** Equality.
    @relates ip4_net
 */
inline bool
operator == (ip4_net const& lhs, ip4_net const& rhs)
{
    return lhs.addr() == rhs.addr() && lhs.mask() == rhs.mask();
}

/** Inequality.
    @relates ip4_net
 */
inline bool
operator != (ip4_net const& lhs, ip4_net const& rhs)
{
    return !(lhs == rhs);
}

/** Operator form of @c is_strict_subset_of.
    @relates ip4_net
 */
inline bool
operator < (ip4_net const& lhs, ip4_net const& rhs)
{
    return lhs.is_strict_subset_of(rhs);
}

/** Operator form of @c is_subset_of.
    @relates ip4_net
 */
inline bool
operator <= (ip4_net const& lhs, ip4_net const& rhs)
{
    return lhs.is_subset_of(rhs);
}

/** Operator form of @c is_strict_superset_of.
    @relates ip4_net
 */
inline bool
operator > (ip4_net const& lhs, ip4_net const& rhs)
{
    return lhs.is_strict_superset_of(rhs);
}

/** Operator form of @c is_superset_of.
    @relates ip4_net
 */
inline bool
operator >= (ip4_net const& lhs, ip4_net const& rhs)
{
    return lhs.is_superset_of(rhs);
}


/** Read a network from a stream.
    The stream should contain a network specified by "ADDR / MASK".
    The @c ADDR should in the standard dotted octet form, followed by
    the literal character '/' followed by the mask in either dotted octet
    or bit count form. Spaces not allowed between digits in the same octet
    or bit count nor between an octet and its trailing dot.
    If the input is malformed the network is set to 0/0.
    @relates ip4_net
 */
API std::istream& operator >> (std::istream& s, ip4_net& net);
/** Write the network to a stream.
    The network is written as "ADDR/MASK" where
    @c ADDR is in octet format and @c MASK is in CIDR format.
    @relates ip4_net
 */
API std::ostream& operator << (std::ostream& s, ip4_net const& net);
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
class ip4_net_generator;

/** Store a range of IPv4 addresses.
 */
class API ip4_range
    : public interval<ip4_addr>
{
public:
    typedef ip4_range self;     //!< Standard self reference type
    typedef interval<ip4_addr> super;   //!< Super class reference type

    //! character used between range end values
    static char const SEPARATOR = '-';

    /** Default constructor.
        The range is initially empty.
     */
    ip4_range() : super()
    {}

    //! Construct from the super class.
    ip4_range(super const& s) : super(s)
    {}

    /** Construct from two endpoints.
        The range contains the end points and all values inbetween.
     */
    ip4_range(ip4_addr x1, ip4_addr x2)
        : super(x1, x2)
    { }

    //! Construct a singleton range.
    ip4_range(ip4_addr const& addr)
        : super(addr)
    {}

    /** Construct a range from a network.
        The range contains exactly those addresses in the network.
     */
    ip4_range(ip4_net const& net)
        : super(net.addr(), net.max_addr())
    {}

    /** Construct a range from its string representation.
     */
    ip4_range(std::string const& s);

    /** Iterator for stepping through the networks that cover this range.
        @internal This has been a strongly debated topic for a long time,
        but I've come to the conclusion that, if at all possible, iterators
        are the best overall solution. Even if the iterator implementation
        is a bit complex, it saves downstream because of all of the STL
        algorithms that work with iterators.
     */
    typedef ip4_net_generator net_iterator;

    /** Calculate the next network and update the state.
        @return @c true if there are addresses left in the range after the
        update, @c false if the range is now empty.
        @note This modifies the range on which it is called.
     */
    bool extract_next_network(ip4_net& net);

    /** Get an iterator for the first network in the network cover of the range.

        Example use for copying the networks to an STL compliant container.
        @code
            copy(r.net_begin(), r.net_end(), inserter(container, container.end()));
        @endcode
     */
    net_iterator net_begin() const;

    //! Get an iterator past the end of the network cover of the range.
    net_iterator net_end() const;

    //! Test if the range is also a network.
    bool is_network() const;
};

/** Read the range from a stream.
    The input should be in the form "MIN - MAX" where
    @c MIN is the minimum value in the range and @c MAX
    is the maximum value.
    @relates ip4_range
 */
API std::istream& operator >> (std::istream& , ip4_range& );
/** Write the range to a stream.
    The range is written as "MIN-MAX" where
    @c MIN is the minimum value in the range and @c MAX
    is the maximum value.
    @relates ip4_range
 */
API std::ostream& operator << (std::ostream& , const ip4_range& );

/** Networks from range generator.
    This generates networks from a range. It operates in the same way
    as an iterator, with @c operator* used to access the current network
    and @c operator++ used to generate the next network.
    
    The default constructed generator is an empty generator and can be
    used to detect when all networks have been generated.

    For example, to copy the networks to an STL compliant container one
    can use
    @code
        ip4_range r;
        copy(ip4_net_generator(r), ip4_net_generator(), inserter(container, container.end()));
    @endcode
    
    This is equivalent to
    @code
    ip4_range r;
    copy(r.net_begin(), r.net_end(), inserter(container, container.end()));
    @endcode
    
    @note The networks are generated from the original range such that the
    generated networks cover exactly the same addresses as the range
    using the minimum number of networks.
 */
class ip4_net_generator
    : public
    std::iterator<std::forward_iterator_tag, ip4_net>
{
protected:
    ip4_range   m_range;    //!< remaining range
    ip4_net     m_data;     //!< client data
public:
    typedef ip4_net_generator self; //!< Self reference type.

    //! Default constructor.
    ip4_net_generator()
    {}

    //! Construct from a range.
    ip4_net_generator(ip4_range const& r)
        : m_range(r)
    {
        ++*this;
    }

    /** Pre-increment.
        The generator is updated to contain the next network.
     */
    self& operator ++ ()
    {
        m_range.extract_next_network(m_data);
        return *this;
    }

    /** Post-increment.
        The generator is updated to contain the next network.
     */
    self operator ++ (int)
    {
        self zret(*this);
        ++*this;
        return zret;
    }

    /** Dereference.
        @return The current network.
     */
    ip4_net const& operator * () const
    { return m_data; }

    /** Pointer.
        @return A pointer to the current network.
        */
    ip4_net const* operator -> () const
    { return &m_data; }

    friend bool operator == (self const& lhs, self const& rhs);
};


/** Equality.
    @relates ip4_net_generator
 */
inline bool
operator == (ip4_net_generator const& lhs, ip4_net_generator const& rhs)
{
    return lhs.m_data == rhs.m_data && lhs.m_range == rhs.m_range;
}

/** Inequality.
    @relates ip4_net_generator
 */
inline bool
operator != (ip4_net_generator const& lhs, ip4_net_generator const& rhs)
{
    return !(lhs == rhs);
}
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/** A class to store an address / network pair.
    This stores an address and a network mask. This implies a network as well,
    which by definition will contain the address. The class supports a user
    conversion to an @c ip4_net which will contain the implied network. The
    class also has user conversions to an @c ip4_mask and @c ip4_addr which do
    which simply return the member values.
 */
class API ip4_pepa
    : public
    boost::totally_ordered<ip4_pepa>
{
public:
    typedef ip4_pepa self; //!< Self reference type

    //! Separator character for address and mask.
    static char const SEPARATOR = '/';

    //! @name Constructors
    //@{
    /** Default constructor.
        The address is 0 and the network is empty.
     */
    ip4_pepa() { }

    /** Construct from an address and a mask.
     */
	ip4_pepa(
        ip4_addr const& addr,   //!< address for the PEPA
        ip4_mask const& mask    //!< mask for the PEPA
        )
        : _addr(addr), _mask(mask)
    {}

    //! Construct from string.
    ip4_pepa(std::string const& s);

    //@}

    //! Reset the PEPA values.
    self& set(
        ip4_addr const& addr,   //!< new network address
        ip4_mask const& mask    //!< new network mask
        )
    {
        _addr = addr;
        _mask = mask;
        return *this;
    }

    //! @name Implicit conversions
    //@{
    //! Convert to an address by returning the end point address.
    operator ip4_addr() const
    { return _addr; }
    //! Convert to a mask by returning the mask.
    operator ip4_mask() const
    { return _mask; }
    //! Convert to a network by returning the network that contains the address with the same mask.
    operator ip4_net() const
    {
        ip4_net net(_addr, _mask);
        return net;
    }
    //@}

	//@{ @name Explicit conversions
    //! End point address.
    ip4_addr addr() const
    { return _addr; }
    //! End point network mask.
    ip4_mask mask() const
    { return _mask; }
    /** Host address.
        @return The portion of the address that is not part of the network address.
     */
	ip4_addr host_addr() const
    { return _addr & ~_mask; }
    //! The network address.
    ip4_addr net_addr() const
    { return _addr & _mask; }
    //! The network.
    ip4_net net() const
    {
        ip4_net net(_addr, _mask);
        return net;
    }
    //@}

private:
    ip4_addr _addr;	//!< The address.
    ip4_mask _mask;	//!< The mask.

};

/** Equality.
    @relates ip4_pepa
 */
inline bool
operator == (ip4_pepa const& lhs, ip4_pepa const& rhs)
{
    return lhs.addr() == rhs.addr() && lhs.mask() == rhs.mask();
}

/** Ordering.
    PEPAs are ordered primarily by address and secondarily by mask.
    - Addresses are ordered by @c ip4_addr ordering.
    - Masks are ordered by @c ip4_mask ordering.
    @relates ip4_pepa
 */
inline bool operator < (ip4_pepa const& lhs, ip4_pepa const& rhs)
{
    return lhs.addr() == rhs.addr()
        ? lhs.mask() < rhs.mask()
        : lhs.addr() < rhs.addr();
}

/** Read the PEPA from a stream.
    The input should be of the form "ADDR / MASK" where
    @c ADDR is an IPv4 address in octet form and
    @c MASK is an IPv4 mask in either octet or CIDR form.
    @relates ip4_pepa
 */
API std::istream& operator >> (std::istream& s, ip4_pepa& p);
/** Write the PEPA to a stream.
    The output is of the form "ADDR/MASK" where
    @c ADDR is an IPv4 address in octet form and
    @c MASK is an IPv4 mask in CIDR form.
    @relates ip4_pepa
 */
API std::ostream& operator << (std::ostream& s, ip4_pepa const& p);
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/** ICMP message type value.
    The encodes a message type for the ICMP IP protocol.
    
    This class is @ref totally_ordered.
 */
class API icmp_type
    : public
    boost::totally_ordered<icmp_type>
{
public:
    typedef icmp_type self; //!< Self reference typedef.
    typedef int host_type; //!< Native storage type.
    typedef ngeo::lexicon<self> lexicon_type; ///< Lexicon localized for this type.

    /// @name Predefine values.
    /// This includes all officially defined ICMP message types.
    //@{
    static self const INVALID;
    static self const ECHO_REPLY;
    static self const UNREACHABLE;
    static self const SOURCE_QUENCH;
    static self const REDIRECT;
    static self const ALTERNATE_ADDRESS;
    static self const ECHO;
    static self const ROUTER_ADVERTISEMENT;
    static self const ROUTER_SOLICITATION;
    static self const TIME_EXCEEDED;
    static self const PARAMETER_PROBLEM;
    static self const TIME_STAMP_REQUEST;
    static self const TIME_STAMP_REPLY;
    static self const INFO_REQUEST;
    static self const INFO_REPLY;
    static self const ADDR_MASK_REQUEST;
    static self const ADDR_MASK_REPLY;
    static self const TRACEROUTE;
    static self const CONVERSION_ERROR;
    static self const MOBILE_REDIRECT;
    static self const MIN; ///< Smallest valid value
    static self const MAX; ///< Largest valid value
    //@}

    /// Mark as having static @c MIN and @c MAX.
    struct static_MIN_MAX_tag;

    icmp_type() : _value(INVALID._value) { } //!< Default constructor.
    icmp_type(self const& that) : _value(that._value) { } //!< Copy constructor.
    icmp_type(host_type c) : _value(c) { } //!< Construct from native type.
    icmp_type(std::string &str) {
    	std::istringstream istr(str);
		istr >> *this;
    }

    operator host_type () const { return _value; } //!< User conversion to underlying enum.
    /// Explicit request for underlying host value.
    host_type host_order() const { return _value; }

    /// Test if a value if valid.
    static bool is_valid(host_type v) { return MIN._value <= v && v <= MAX._value; }
    //! Test if @c this contains a valid value.
    bool is_valid() const { return is_valid(_value); }

    //! Test if the value is a defined value.
    bool is_defined() const;

    //! Pre-increment
    self& operator ++  ()
    {
        if (MIN._value <= _value && _value < MAX._value) ++_value;
        return *this;
    }
    
    //! Pre-decrement
    self& operator --  ()
    {
        if (MIN._value < _value && _value <= MAX._value) --_value;
        return *this;
    }    

    /** Return the name of the message type.
        If this value has a defined name, that is returned.
        Otherwise, the character encoding of the numeric value is returned.
        All strings returned by this method will be parsed by conversion from string to this type.
     */
    std::string get_name() const;
    /// Accessor for lexicon (name definitions).
    static lexicon_type& get_lexicon();

    /** Output stream operator.
        @return The output stream @a s.
     */
    friend API std::ostream& operator << (
        std::ostream& s, //!< Output stream.
        self const& t //!< Instance to output.
        )
    { return s << t.get_name(); }

    /** Input stream operator.
        If the input is not a valid name for an ICMP message type, @a t is
        unchanged and the state of @a s is set to failed.
        @return The input stream @a s.
     */
    friend API std::istream& operator >> (
        std::istream& s, //!< Input stream.
        self& t //!< Target instance.
        );

    //! Equality operator.
    friend bool operator == (self const& x, self const& y) { return x._value == y._value; }
    //! Ordering by enumeration value.
    friend bool operator <  (self const& x, self const& y) { return x._value <  y._value; }

private:
    static host_type bound(host_type x) { return MIN._value <= x && x <= MAX._value ? x : INVALID._value; } //!< normalize input to acceptable value

    host_type _value; //!< The raw value.
};

// Make the MS compiler happy.
template class API numeric_type<unsigned char, struct icmp_code_tag>;
/** ICMP Message code.
    This is just an 8 bit value, no special properties.
 */
typedef numeric_type<unsigned char, struct icmp_code_tag> icmp_code;
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/** An ICMP message.
    This consists of a @a type and @a code. Currently no checking is done to
    verify that the code is consistent with the message type.

    This class is @ref totally_ordered.
 */
class API icmp
    : public
    boost::totally_ordered<icmp>
{
public:
    typedef icmp self; //!< Self reference type.

    static self const MIN; //!< Instance with minimum value.
    static self const MAX; //!< Instance with maximum value.
    /// Mark as having built in MIN/MAX members.
    struct static_MIN_MAX_tag;

    /// Default constructor.
    icmp() {}
    /// Copy constructor.
    icmp(self const& that) : m_type(that.m_type), m_code(that.m_code) {}
    /// Construct from message type @a t only, message code is not initialized.
    icmp(icmp_type const t) : m_type(t) {}
    /// Construct from a type @a t and a code @a c.
    icmp(icmp_type const t, icmp_code const c) : m_type(t), m_code(c) {}

    //! User conversion to ICMP type.
    operator icmp_type () const { return m_type; }
    //! User conversion to ICMP code.
    operator icmp_code () const { return m_code; }

    //! Access method for type.
    icmp_type type () const { return m_type; }
    icmp_code code () const { return m_code; }

    //! Increment operator.
    self& operator ++ ();
    //! Decrement operator.
    self& operator -- ();

    /// @cond NOT_DOCUMENTED
    friend bool operator == (self const& lhs, self const& rhs) { return lhs.m_type == rhs.m_type /* && lhs.m_code == rhs.m_code */; }
    friend bool operator <  (self const& lhs, self const& rhs)
    {
        return lhs.m_type < rhs.m_type /* ? true
             : lhs.m_type > rhs.m_type ? false
             : lhs.m_code < rhs.m_code
             */
             ;
    }
    /// @endcond

private:
    icmp_type m_type; ///< Type.
    icmp_code m_code; ///< Code.
};

/** Write to stream.
    This currently prints only the message type, not the code.
    @return @a s
    @relates icmp
 */
inline
std::ostream& operator <<
    ( std::ostream& s ///< Output stream
    , icmp const& i ///< ICMP data
    )
{
    return s << i.type();
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/** This represents the protocol field in an IP header.
    This will store any valid (0..255) protocol value.

    This class is @ref totally_ordered.
 */
class API ip_protocol
    : public
    boost::totally_ordered<ip_protocol>
{
public:
    typedef ip_protocol self; //!< Self reference type.
    typedef int host_type;  //!< Internal storage type.
    typedef lexicon<self> lexicon_type;

    //! @c host_type value for minimum protocol value.
    static host_type const HOST_MIN = 0;
    //! @c host_type value for maximum protocol value.
    static host_type const HOST_MAX = 255;

    static self const MIN; ///< Minimum protocol value.
    static self const MAX; ///< Maximum protocol value.
    /// Mark as having built in MIN/MAX members.
    struct static_MIN_MAX_tag;

    /** @name Predefined protocol values.
        This is not the entire IANA set but just
        those that have additional data associated with them or were
        specifically requested by clients. The full list is at
        <a href="http://www.iana.org/assignments/protocol-numbers">IANA</a>.

        Each of these is paired, with a value for the host_type and
        another for the host_type wrapped in this type. This parallels
        the pairing for MIN and MAX.

        @note These are just for coding. The string conversion names can
        be adjusted at run time via the lexicon.
        @see @c get_lexicon
     */
    //@{

    /** Internal Control Message Protocol.
        @see icmp.
     */
    static self const ICMP;
    static host_type const HOST_ICMP = 1;
    //! Transmission Control Protocol.
    static self const TCP;
    static host_type const HOST_TCP = 6;
    //! Unreliable Datagram Protocol.
    static self const UDP;
    static host_type const HOST_UDP = 17;
    //! All IP protocols.
    static self const IP;
    static host_type const HOST_IP = HOST_MAX+1;
    //! Invalid protocol value.
    static self const INVALID;
    static host_type const HOST_INVALID = -1;

    //@}

    //! @name Basic Operations
    //@{
    ip_protocol() : _value(INVALID) { } //!< Default constructor.
    ~ip_protocol()  {} ///< Destructor.
    ip_protocol(self const& x) : _value(x._value) { } //!< Copy constructor.
    ip_protocol(host_type x) : _value(bound(x)) { } //!< Construct from native type.
    /** Construct from string.
        This can be numeric in the range 0..255, or one of the predefined names.
     */
    explicit ip_protocol
        (std::string const &str ///< Text representation of the protocol
        )
    {
        std::istringstream istr(str);
        istr >> *this;
    }
    self& operator = (self const& x) { _value = x; return *this; } //!< Self assignment.
    self& operator = (host_type x) { _value = bound(x); return *this; } //!< Assignment from host type, to match constructor.
    //@}

    //! Pre-increment
    self& operator ++  () {
        if (_value >= HOST_MIN && _value < HOST_MAX)
            ++_value;
        return *this;
    }
    
    //! Pre-decrement
    self& operator --  () {
        if (_value > HOST_MIN && _value <= HOST_MAX)
            --_value;
        return *this;
    }

    /// Test if a @c host_type value is a valid @c ip_protoco value.
    static bool is_valid(host_type p) { return (HOST_MIN <= p && p <= HOST_MAX) || HOST_IP == p; }
    /// Test if @c this contains a valid @c ip_protocol value.
    bool is_valid() const { return is_valid(_value); }

    //! Automatic conversion to host type.
    operator host_type () const { return _value; }
    /// Explicit request for host type data.
    host_type host_order() const { return _value; }

    /** @name Ancillary data methods.
        These provide meta-data about what kind of additional information is
        associated with a protocol.
     */
    //@{
    /// Currently supported types of ancillary data.
    /// @internal These are tuned to match the which() values in ip_service::data.
    typedef enum { DATA_NONE = -1 //!< No ancillary data.
                 , DATA_ICMP = 0//!< ICMP data (@c icmp)
                 , DATA_PORT = 1//!< Port data (@c ip_port)
    } data_type;
    
    /// Test for ancillary data.
    /// @return @c true if the protocol value in this instance has ancillary data,
    /// @c false otherwise.
    bool has_ancillary_data() { return this->get_data_type() != DATA_NONE; }

    //! What type of data should this protocol have?
    data_type get_data_type () const {
        return _value == HOST_ICMP ? DATA_ICMP
            :  _value == HOST_TCP ? DATA_PORT
            :  _value == HOST_UDP ? DATA_PORT
            : DATA_NONE;
    }
    //@}

    /** Return the name of the protocol.
        @return If the protocol is a common one, the common string name (e.g., "TCP" for 6).
        Otherwise, the character encoding of the numeric value is returned.
        @note All strings returned by this method will be parsed by conversion from string to this type.
     */
    std::string get_name() const;
    /// Get list of all named protocols.
    /// This data can be modified to change the set of names recognized by this class.
    /// @see @c Lexicon
    static lexicon_type& get_lexicon();

private:
    host_type _value; /// Protocol value

    static host_type bound(host_type x) { return HOST_MIN <= x && x <= (HOST_MAX+1) ? x : HOST_INVALID; } //!< normalize input to acceptable value

public:
    /// @cond NOT_DOCUMENTED
    friend bool operator == ( self const& lhs, self const& rhs ) { return lhs._value == rhs._value; }
    friend bool operator <  ( self const& lhs, self const& rhs ) { return lhs._value <  rhs._value; }
    /// @endcond

    /// Read from stream.
    /// @return @a s
    friend API std::ostream& operator <<
        ( std::ostream& s ///< Input stream
        , self const& p ///< Storage for instance
        )
    { return s << p._value; }
    /// Write to stream.
    /// @return @a s
    friend API std::istream& operator >>
        ( std::istream& s ///< Output stream
        , self & p ///< Protocol to write
        );
};

//! Currently IPv4 protocols are common across IP versions.
typedef ip_protocol ip4_protocol;

/* ------------------------------------------------------------------------ */
} // namespaces

//! @cond DO_NOT_DOCUMENT
namespace std
{
// Define numeric limits for the integral types.
template <> class numeric_limits<ngeo::ip_port>
    : public numeric_limits<ngeo::ip_port::host_type>
{ };
template <> class numeric_limits<ngeo::ip4_addr>
    : public numeric_limits<ngeo::ip4_addr::host_type>
{ };
template <> class numeric_limits<ngeo::ip_protocol>
    : public numeric_limits<ngeo::ip_protocol::host_type>
{
public:
    static ngeo::ip_protocol const& min() { return ngeo::ip_protocol::MIN; }
    static ngeo::ip_protocol const& max() { return ngeo::ip_protocol::MAX; }
    static bool has_quiet_NaN() { return true; }
    static ngeo::ip_protocol const& quiet_NaN() { return ngeo::ip_protocol::INVALID; }
    static int const digits = 8;
};

template <> class numeric_limits<ngeo::icmp_code> : public numeric_limits<ngeo::icmp_code::host_type>
{
};

template <> class numeric_limits<ngeo::icmp_type> : public numeric_limits<ngeo::icmp_type::host_type>
{
public:
    static ngeo::icmp_type const& min() { return ngeo::icmp_type::MIN; }
    static ngeo::icmp_type const& max() { return ngeo::icmp_type::MAX; }
    static bool has_quiet_NaN() { return true; }
    static ngeo::icmp_type const& quiet_NaN() { return ngeo::icmp_type::INVALID; }
    static int const digits = 8;
};

}// namespace std

namespace boost
{
    extern size_t hash_value(unsigned int); // must guarantee this declared or port hashes will recurse
    inline size_t hash_value(ngeo::ip_port const& p) { return hash_value(p.host_order()); }
} // namespace boost

//! @endcond
/* ------------------------------------------------------------------------ */
# undef API
