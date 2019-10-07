// SPDX-License-Identifier: Apache-2.0
// Copyright 2001 Network Geographics
#pragma once
/* ------------------------------------------------------------------------ */
# if !defined(ISMG_CN_IP_BASE_HEADER)
# define ISMG_CN_IP_BASE_HEADER
/* ------------------------------------------------------------------------ */
# if defined(ISMG_CN_IP_INTERNAL)
#   define EXPORT _declspec(dllexport)
# else
#   define EXPORT _declspec(dllimport)
# endif

# include <stl/vector>
# include <stl/iostream>
# include <stl/limits>
/* ------------------------------------------------------------------------ */
namespace Cn
{
/* ------------------------------------------------------------------------- */
# pragma warning(push)
# pragma warning(disable:4251)	/* disable non dll-interface for IpMap class */
# pragma warning(disable:4275)	/* disable non dll-interface for IpMap class */
# pragma warning(disable:4786)	/* disable identifier too long */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------ */
class IpMask;	// used in IpAddr ctor
class IpNet;	// declared friend in IpMask
class IpPepa;	// declared friend in IpMask
/* ------------------------------------------------------------------------ */
/* All internal data is stored in host order. This can cause problems when
 * used for data in external situations, but overall it was considered
 * easier to do those conversions when necessary. The methods "ntoh" and
 * "hton" are provided to do these conversions, along with ctors from the
 * underlying type and GetRaw() to access the internal data. So to use a
 * port in a socket call, it would be "port.hton(port.GetRaw())"
 */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
class EXPORT IpPort
{
public:
	typedef IpPort self;
	typedef unsigned short Type;	// Ip port number type.
	enum
	{
		// WIDTH is the IPv4 defined size of a port. It should be the case
		// that (std::numeric_limits<IpPort::Type>::digits==IpPort::WIDTH)
		// but there is no way to verify that at compile time.
		WIDTH = 16,
		HALF_MASK= ~(~static_cast<Type>(0) << (WIDTH/2))
	};

	self() : m_port(0) { }
	self(Type port) : m_port(port) { }	//Constructor from unsigned short.
	self(std::string const& s);
	// use compiler generated copy and assignment

	Type GetRaw() const;
	void Set(Type p);

	static self Min();
	static self Max();

	// Host <-> network conversion operators
	static Type ntoh(Type x);
	static Type hton(Type x);

	Type GetType() const; // DEPRECATED, use GetRaw()

protected:
	Type m_port;

	friend bool operator == (self const&, self const&);
	friend bool operator <  (self const&, self const&);

	friend std::ostream& operator << (std::ostream&, self const&);
	friend std::istream& operator >> (std::istream&, self&);
};

inline IpPort::Type IpPort::GetType() const { return m_port; }
inline IpPort::Type IpPort::GetRaw() const { return m_port; }
inline void IpPort::Set(Type p) { m_port = p; }
inline IpPort IpPort::Min() { return std::numeric_limits<Type>::min(); }
inline IpPort IpPort::Max() { return std::numeric_limits<Type>::max(); }

# if defined(_M_IX86)
inline IpPort::Type IpPort::ntoh(Type x) { return static_cast<Type>( ((x >> (WIDTH/2)) & HALF_MASK) | ((x & HALF_MASK) << (WIDTH/2)) ); }
inline IpPort::Type IpPort::hton(Type x) { return ntoh(x); }
# else
inline IpPort Type IpPort::ntoh(Type x) { return x; }
inline IpPort Type IpPort::hton(Type x) { return x; }
# endif

// Stupid language. Because of the way C++ is structured, if you add two shorts and assign the result
// back to a short, a cast is required (!) Unbelievable.
inline IpPort operator + (IpPort const& x, IpPort const& y) { return static_cast<IpPort::Type>(x.GetRaw() + y.GetRaw()); }
inline IpPort operator + (IpPort const& x, IpPort::Type n)  { return static_cast<IpPort::Type>(x.GetRaw() + n); }
inline IpPort operator - (IpPort const& x, IpPort const& y) { return static_cast<IpPort::Type>(x.GetRaw() - y.GetRaw()); }
inline IpPort operator - (IpPort const& x, IpPort::Type n)  { return static_cast<IpPort::Type>(x.GetRaw() - n); }

// fundamental compares
inline bool operator == (const IpPort& x, const IpPort& y) { return x.m_port == y.m_port; }
inline bool operator <  (const IpPort& x, const IpPort& y)  { return x.m_port < y.m_port;}
// derivative compares
inline bool operator >  (const IpPort& x, const IpPort& y) { return y < x; }
inline bool operator <= (const IpPort& x, const IpPort& y) { return !(y < x); }
inline bool operator >= (const IpPort& x, const IpPort& y) { return !(x < y); }
inline bool operator != (const IpPort& x, const IpPort& y) { return !(x == y); }

inline std::ostream& operator << (std::ostream& s, IpPort const& p) {  s << p.m_port; return s; }
inline std::istream& operator >> (std::istream& s, IpPort& p) {  s >> p.m_port; return s; }

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

class EXPORT IpPortRange
{
public:
	typedef IpPortRange self;

	IpPortRange() : m_low(IpPort::Min()), m_high(IpPort::Max()) { }
	IpPortRange(IpPort const& lower, IpPort const& upper)
		: m_low( std::min(lower, upper) ), m_high( std::max(lower, upper) )
	{ }
	IpPortRange(IpPort const& port) : m_low(port), m_high(port) { }
	IpPortRange(std::string const& str);
	// use compiler generated self-copy and self-assignment

	static char const SEPARATOR; // the character used between the address and mask
	operator std::string () const; // implicit conversion
	std::string ntoa() const; // explicit conversion
	void ntoa(std::string& str) const; // conversion in place - appends to string

	bool IsCompatible(IpPort const& port) const; // port is element of range
	bool IsSingleton() const; // range is of size 1

	void Set(std::string const& text);
	void Set(IpPort const& lower, IpPort const& upper);
	void Set(IpPort const& port);
	bool SetUpper(IpPort port); // false if would result in illegal range
	bool SetLower(IpPort lower); // false if would result in illegal range

	IpPort GetUpper() const;
	IpPort GetLower() const;
	
	// Overlap means the intersetion is non-empty
	bool HasOverlap(self const& other) const; // test only
	bool CalcOverlap(self& out, self const& src) const; // test and compute

	/* A union can only be created if the union of the two ranges
	 * is itself continguous.
	 */
	bool HasUnion(self const& src) const; // test only
	bool CalcUnion(self& out, self const& src) const; // test and compute
	
	/* Indicates whether a range is adjacent to this range.
	 * Adjacent ranges have no overlap.
	 */
	bool IsAdjacentTo(self const& other) const; 
	bool IsSubset(self const& other) const; // strict subset: *this is subset of other

	// These are deprecated because "UpperBound" is not the STL standard
	// upper bound because this range is closed, not half open
	IpPort GetUpperBound() const;
	IpPort GetLowerBound() const;

private:
	IpPort m_low, m_high;  /* Two ends of a range of Ip ports */

	friend EXPORT std::ostream& operator << (std::ostream&, self const&);
	friend EXPORT std::istream& operator >> (std::istream&, self&);

	// It's not clear what we want here, but this comparison at least is a
	// complete ordering, which is useful for various STL containers.
	friend bool operator <  (self const& x, self const& y);
	friend bool operator == (self const& x, self const& y);
};

inline std::string IpPortRange::ntoa() const { return *this; }

inline bool IpPortRange::IsCompatible(IpPort const& port) const { return port <= m_high && m_low <= port; }
inline bool IpPortRange::IsSingleton() const { return m_low == m_high; }
inline IpPort IpPortRange::GetUpper() const { return m_high; } 
inline IpPort IpPortRange::GetLower() const { return m_low;  }
inline void IpPortRange::Set(IpPort const& port) { m_low = m_high = port; }
inline void IpPortRange::Set(IpPort const& lower, IpPort const& upper)
{ 
	m_low = std::min(lower, upper);
	m_high = std::max(lower, upper);
}
inline bool IpPortRange::SetLower(IpPort port) { return port <= m_high ? (m_low = port , true ) : false; }
inline bool IpPortRange::SetUpper(IpPort port) { return port >= m_low ? (m_high = port , true ) : false; }

inline bool operator <  (IpPortRange const& x, IpPortRange const& y) { return x.m_low < y.m_low || ( x.m_low == y.m_low && x.m_high < y.m_high); }
inline bool operator == (IpPortRange const& x, IpPortRange const& y) { return x.m_low == y.m_low && x.m_high == y.m_high; }

inline bool operator >  (IpPortRange const& x, IpPortRange const& y) { return y < x; }
inline bool operator <= (IpPortRange const& x, IpPortRange const& y) { return !(y < x); }
inline bool operator >= (IpPortRange const& x, IpPortRange const& y) { return !(x < y); }
inline bool operator != (IpPortRange const& x, IpPortRange const& y) { return !(x == y); }

// Deprecated methods
inline IpPort IpPortRange::GetUpperBound() const { return m_high; } 
inline IpPort IpPortRange::GetLowerBound() const { return m_low;  }
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

class EXPORT IpAddr
{
public:
	typedef IpAddr self;
	typedef unsigned int Type;

	enum
	{
		// WIDTH is the IPv4 defined size of an address. It should be the
		// case that (std::numeric_limits<IpAddr::Type>::digits==IpAddr::WIDTH)
		// but there is no way to verify that at compile time.
		WIDTH = 32,
		HALF_MASK= ~(~static_cast<Type>(0) << (WIDTH/2))
	};

	self() : m_addr(0) { }
	self(Type t) : m_addr(t) { }
	self(IpMask const& mask);
	self(std::string const& s);
	// use compiler copy constructor and assignment

	Type GetRaw() const;

	static IpAddr Min();
	static IpAddr Max();

	// Lots of variants for converting to strings.
	operator std::string () const;		// implicit conversion
	std::string ntoa() const;			// explicit conversion
	std::string ntoa(int w) const;		// explicit conversion with fixed width octets
	void ntoa(std::string& str) const;	// place into existing string
	void ntoa(std::string& str, int) const; // ^ w/ fixed width octets

	//Network  addrss <-> Host address conversion
	static Type ntoh(Type x);
	static Type hton(Type x);

	Type GetAddress() const;	// DEPRECATED, use GetRaw()
	static self aton(std::string const& str); // DEPRECATED, use constructor
	void ntoa(int w, std::string& str) const; // DEPRECATED, use ntoa(string, int)

private:
	Type m_addr;

	friend bool operator == (self const&, self const&);
	friend bool operator <  (self const&, self const&);

	friend self operator - (const self&, unsigned int);
	friend self operator + (const self&, unsigned int);
		
	friend EXPORT std::ostream& operator << (std::ostream& s, const self& a);
	friend EXPORT std::istream& operator >> (std::istream& s, self& a);
};

inline std::string IpAddr::ntoa() const { return *this; }

inline IpAddr IpAddr::Min() { return std::numeric_limits<Type>::min(); }
inline IpAddr IpAddr::Max() { return std::numeric_limits<Type>::max(); }

inline bool operator == (IpAddr const& x, IpAddr const& y) { return x.m_addr == y.m_addr; }
inline bool operator <  (IpAddr const& x, IpAddr const& y) { return  x.m_addr < y.m_addr; }

inline bool operator >  (IpAddr const& x, IpAddr const& y) { return y < x; }
inline bool operator <= (IpAddr const& x, IpAddr const& y) { return !(y < x); }
inline bool operator >= (IpAddr const& x, IpAddr const& y) { return !(x < y); }
inline bool operator != (IpAddr const& x, IpAddr const& y) { return !(x == y); }

// no wrap checks
inline IpAddr operator - (IpAddr const& src, unsigned int i) { return src.m_addr - i; }
inline IpAddr operator + (IpAddr const& src, unsigned int i) { return src.m_addr + i; }

inline IpAddr::Type IpAddr::GetRaw() const { return m_addr; }

# if defined(_M_IX86)
// Abuse the fact that an address is exactly twice as wide as a port
inline IpAddr::Type IpAddr::ntoh(Type x) { return IpPort::ntoh(static_cast<IpPort::Type>(x >> IpPort::WIDTH)) | (IpPort::ntoh(static_cast<IpPort::Type>(x & HALF_MASK)) << IpPort::WIDTH); }
inline IpAddr::Type IpAddr::hton(Type x) { return ntoh(x); }
# else
inline IpAddr::Type IpAddr::ntoh(Type x) { return x; }
inline IpAddr::Type IpAddr::hton(Type x) { return x; }
# endif

// DEPRECATED methods
inline IpAddr::Type IpAddr::GetAddress() const { return m_addr; }
inline IpAddr IpAddr::aton(std::string const& str) { return IpAddr(str); }
inline void IpAddr::ntoa(int w, std::string& s) const { this->ntoa(s, w); }

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

class EXPORT IpMask
{
public:
	typedef IpMask self;
	typedef unsigned int Type;
	enum { WIDTH = IpAddr::WIDTH };

	IpMask() : m_mask(0) { }
	IpMask(Type count) : m_mask(std::min(static_cast<Type>(WIDTH), count)) { }
	IpMask(std::string const& s);
	// use compiler copy constructor and assignment

	operator std::string () const;	// implicit conversion
	std::string ntoa() const;		// explicit conversion
	void ntoa(std::string& str) const; // put in existing string
	
	int ValidCount(IpAddr addr); // -1 on invalid mask, otherwise mask width

	friend self operator<< (self const& m, unsigned int n);
	friend self operator>> (self const& m, unsigned int n);
	self& operator<<= (unsigned int n);
	self& operator>>= (unsigned int n);
	friend bool operator == (const self&, const self&);
	friend bool operator <  (const self&, const self&);

	friend std::ostream& operator << (std::ostream&,  const self&);
	friend EXPORT std::istream& operator >> (std::istream& s, self& a);

	// These return the bitcount. To get it as bit mask, use the
	// IpAddr ctor.
	Type GetMask() const { return m_mask; } // DEPRECATED, use GetRaw()
	Type GetRaw() const { return m_mask; }

	static self Min();
	static self Max();

	static self aton(const std::string& str); // DEPRECATED - use ctor
	static std::string ntoa(const self& mask); // DEPRECATED

private:
	Type m_mask;

	// special purpose for use in mixed operators. Clients should use
	// IpAddr ctor
	IpAddr::Type GetRawAddr() const;

	// other classes allowed to reach inside
	friend IpAddr;
	friend IpNet;
	friend IpPepa;

	// operators that need GetRawAddr()
	friend IpAddr operator & (IpAddr const& x, self const& y);
	friend IpAddr operator & (self const& x, IpAddr const& y);
	friend IpAddr operator | (IpAddr const& x, self const& y);
	friend IpAddr operator | (self const& x, IpAddr const& y);
	friend IpAddr operator ~ (self const& x);
};

inline IpMask IpMask::Min() { return 0; }
inline IpMask IpMask::Max() { return WIDTH; }

inline std::ostream& operator << (std::ostream& s,  const IpMask& a) { s << a.m_mask; return s; }
inline std::string IpMask::ntoa() const { return *this; }

// These are clipping shifts - shifts that would cause the mask to go outside the range [0..WIDTH] are
// clipped to become 0 or WIDTH.
inline IpMask operator>> (IpMask const& m, unsigned int n) { return m.m_mask + std::min(n, IpMask::WIDTH - m.m_mask); }
inline IpMask operator<< (IpMask const& m, unsigned int n) { return m.m_mask - std::min(n, m.m_mask); }
inline IpMask& IpMask::operator>>= (unsigned int n) { m_mask += std::min(n, IpMask::WIDTH - m_mask); return *this; }
inline IpMask& IpMask::operator<<= (unsigned int n) { m_mask -= std::min(n, m_mask); return *this; }

inline bool operator == (const IpMask& x, const IpMask& y) { return x.m_mask == y.m_mask; }
inline bool operator <  (const IpMask& x, const IpMask& y) { return x.m_mask < y.m_mask; }

inline bool operator >  (IpMask const& x, IpMask const& y) { return y < x; }
inline bool operator <= (IpMask const& x, IpMask const& y) { return !(y < x); }
inline bool operator >= (IpMask const& x, IpMask const& y) { return !(x < y); }
inline bool operator != (IpMask const& x, IpMask const& y) { return !(x == y); }

// Address and Mask bitwise operators
inline IpAddr::Type IpMask::GetRawAddr() const { return m_mask ? ~static_cast<IpAddr::Type>(0) << (IpAddr::WIDTH - m_mask) : 0; }
inline IpAddr operator & (const IpAddr& x, const IpMask& y) { return x.GetRaw() & y.GetRawAddr(); }
inline IpAddr operator & (const IpMask& x, const IpAddr& y) { return x.GetRawAddr() & y.GetRaw(); }
inline IpAddr operator & (const IpAddr& x, const IpAddr& y) { return x.GetRaw() & y.GetRaw(); }
inline IpAddr operator | (const IpAddr& x, const IpAddr& y) { return x.GetRaw() | y.GetRaw(); }
inline IpAddr operator | (const IpAddr& x, const IpMask& y) { return x.GetRaw() | y.GetRawAddr(); }
inline IpAddr operator | (const IpMask& x, const IpAddr& y) { return x.GetRawAddr() | y.GetRaw(); }
inline IpAddr operator ~ (const IpMask& x) { return ~x.GetRawAddr(); }

// DEPRECATED methods
inline IpMask IpMask::aton(const std::string& str) { return IpMask(str); }
inline std::string IpMask::ntoa(self const& mask) { return mask; }

// need to delay this definition so that the IpMask method is defined
inline IpAddr::IpAddr(IpMask const& m) : m_addr(m.GetRawAddr()) { }
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

class EXPORT IpNet
{
public:
	IpNet() { }
	IpNet(const IpAddr& addr, const IpMask& mask) : m_addr(addr & mask), m_mask(mask) { }
	IpNet(const IpNet& net) : m_addr(net.m_addr), m_mask(net.m_mask) { }
	IpNet(std::string const& s);
	// use compiler generated copy and assign	

	operator IpAddr() const { return m_addr; }
	operator IpMask() const { return m_mask; }

	IpAddr GetIpAddr() const { return m_addr; } // DEPRECATED, use GetAddr()
	IpMask GetIpMask() const { return m_mask; } // DEPRECATED, use GetMask()
	IpAddr GetAddr() const { return m_addr; }
	IpMask GetMask() const { return m_mask; }

	IpAddr GetLower() const { return m_addr; }
	IpAddr GetUpper() const { return m_addr | ~m_mask; }
	IpAddr GetLowerBound() const { return m_addr; }	// DEPRECATED, use GetLower()
	IpAddr GetUpperBound() const { return m_addr | ~m_mask; } // DEPRECATED, use GetUpper()

	/* Compatible means that the address would fit into the network for this object */
	bool IsCompatible(const IpAddr& addr) const	{ return (addr & m_mask) == m_addr; }
	// computes this is strict subset of other
	bool IsSubset(const IpNet& other) const
	{ return ((m_addr & other.m_mask) == other.m_addr) && (other.m_mask < m_mask); }


	// Overlap means the intersection of the two networks is non-empty
	bool HasOverlap(IpNet const& net) const
	{
		IpMask m(std::min(m_mask, net.m_mask));
		return (m_addr & m) == (net.m_addr & m);
	}

	void Set(const IpAddr& addr, const IpMask& mask) { m_addr = addr & mask;  m_mask = mask; }

	static char const SEPARATOR; // the character used between the address and mask

	operator std::string () const;	// implicit
	std::string ntoa() const;		// explicit
	// the address width is per octet, the mask width for the bit count
	std::string ntoa(int addr_width, int mask_width) const;

	static std::string ntoa(IpNet const& net); // DEPRECATED
	static IpNet aton(std::string const& str); // DEPRECATED - use ctor

private:
	IpAddr m_addr;	/* the network address */
	IpMask m_mask;	/* the network mask */

	friend bool operator == (const IpNet& , const IpNet&);
	friend bool operator < (const IpNet& , const IpNet&);
	friend EXPORT std::istream& operator >> (std::istream& , IpNet& );
	friend EXPORT std::ostream& operator << (std::ostream& , const IpNet& );
};

inline std::string IpNet::ntoa() const { return *this; }

inline bool operator == (const IpNet& x, const IpNet& y) { return x.m_addr == y.m_addr && x.m_mask == y.m_mask;  }
// inverse ordering on masks, but that makes this compatible with the equivalent ranges
inline bool operator <  (const IpNet& x, const IpNet& y) { return (x.m_addr < y.m_addr) || (x.m_addr == y.m_addr && y.m_mask < x.m_mask); }

inline bool operator >  (IpNet const& x, IpNet const& y) { return y < x; }
inline bool operator <= (IpNet const& x, IpNet const& y) { return !(y < x); }
inline bool operator >= (IpNet const& x, IpNet const& y) { return !(x < y); }
inline bool operator != (IpNet const& x, IpNet const& y) { return !(x == y); }

// DEPRECATED methods
inline IpNet IpNet::aton(std::string const& str) { return IpNet(str); }
inline std::string IpNet::ntoa(IpNet const& net) { return net; }

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

class EXPORT IpRange
{
protected:
	IpAddr m_low, m_high;

public:
	IpRange() : m_low(IpAddr::Min()), m_high(IpAddr::Max()) { }
	IpRange(IpAddr lower, IpAddr upper)
		: m_low(std::min(lower, upper))
		, m_high(std::max(lower, upper))
	{ }
	IpRange(IpAddr const& addr) : m_low(addr), m_high(addr) { } // singleton range
	IpRange(IpNet const& net):m_low(net.GetLower()), m_high(net.GetUpper()) { }
	IpRange(std::string const& str);
	// use compiler generated copy and assign	

	static char const SEPARATOR;	// character used between range end values

	operator std::string () const;	// implicit conversion
	std::string ntoa() const;	// explicit conversion
	std::string ntoa(int width) const; // explicit w/fixed width octets

	/* Indicates whether an address is in the range. */
	bool IsCompatible(IpAddr addr) const;

	/* Indicates whether the range is a single address. */
	bool IsSingleton() const;

	/* Set the range to have the specified low and high addresses. 
	 * The addresses are sorted before being used. */
	void Set(IpAddr lower, IpAddr upper);
	void Set(IpAddr addr);
	void Set(std::string const& text);
	bool SetUpper(IpAddr upper); // false if would make illegal range
	bool SetLower(IpAddr lower); // false if would make illegal range

	IpAddr GetUpper() const;
	IpAddr GetLower() const;

	// Deprecated, because of semantic conflict with STL GetUpperBound()
	IpAddr GetUpperBound() const;
	IpAddr GetLowerBound() const;

	// Network generation from a range. We do the general case first which
	// works for any container that supports insert iterators

	// base class used by internal logic
	class NetInserter {	public:	virtual void operator () (IpNet const& n) = 0; };

	// template subclass instantiated by template method
	template <typename I>
	class TypedNetInserter : public NetInserter
	{
	public:
		TypedNetInserter(I const& i) : m_i(i) { }
		virtual void operator () (IpNet const& n) { m_i = n; ++m_i; }
	private:
		I m_i;
	};

	// the actual template method. It constructs the subclass of NetInserter,
	// initializes it with the provided insert iterator and invokes the
	// internal, generic logic.
	template <typename I>
	int GenerateNetworks(I const& i) const
	{
		TypedNetInserter<I> inserter(i);
		return this->GenerateNetworksImpl(inserter);
	}

	/* Create a vector of networks that exactly covers the range and append it to the argument. 
	 * Return the number of networks generated.
	 * DEPRECATED - use 'range.GenerateNetworks(std::back_inserter(networks))'
	 */
	int GenerateNetworks(std::vector<IpNet> & networks) const;

	/* Calculate the overlap of the source range with this range. 
	 * The one argument form returns only whether the two ranges overlap (intersect). 
	 * the two argument form places the overlap (if any) in the second argument.
	 */
	bool HasOverlap(const IpRange& other) const;
	bool CalcOverlap(IpRange& out, const IpRange& src) const;

	/* Calculate the union of the source range with this range. 
	 * This succeeds only if the two ranges overlap or are adjacent. 
	 * The one argument form returns whether a union could be calculated. 
	 * The two argument form returns whether the union could be calculated and if possible, the union.
	 */
	bool HasUnion(const IpRange& src) const ;
	bool CalcUnion(IpRange& out, const IpRange& src) const;

	
	/* Indicates whether a range is adjacent to this range. 
	 * Ranges are adjacent if one range has an upper bound that is exactly  
	 * one less than the lower bound of the other range.
	 */
	bool IsAdjacentTo(const IpRange& other) const;

	/* Check if *this is the subset of another IpRange */
	bool IsSubset(const IpRange& other) const;

	static IpRange aton(const std::string& str); // DEPRECATED - use string ctor
	static std::string ntoa(const IpRange& net); // DEPRECATED - when would this be used?

private:
	int GenerateNetworksImpl(NetInserter& i) const;

	friend bool operator == (IpRange const& x, IpRange const& y);
	friend bool operator <  (IpRange const& x, IpRange const& y);
	friend EXPORT std::istream& operator >> (std::istream& , IpRange& );
	friend EXPORT std::ostream& operator << (std::ostream& , const IpRange& );
};

inline bool IpRange::IsCompatible(IpAddr addr) const { return m_low <= addr && addr <= m_high; }
inline bool IpRange::IsSingleton() const { return m_low == m_high; } 
inline IpAddr IpRange::GetLower() const { return m_low; }
inline IpAddr IpRange::GetUpper() const { return m_high; }

inline void IpRange::Set(IpAddr addr) { this->Set(addr, addr); }
inline void IpRange::Set(IpAddr low, IpAddr high) { m_low = std::min(low, high); m_high = std::max(low, high); }
inline bool IpRange::SetLower(IpAddr a) { return a <= m_high ? (m_low = a , true ) : false; }
inline bool IpRange::SetUpper(IpAddr a) { return a >= m_low ? (m_high = a , true ) : false; }

inline bool operator == (IpRange const& x, IpRange const& y) { return x.m_low == y.m_low && x.m_high == y.m_high; }
inline bool operator <  (IpRange const& x, IpRange const& y) { return x.m_low < y.m_low || (x.m_low == y.m_low && x.m_high < y.m_high); }

inline bool operator >  (IpRange const& x, IpRange const& y) { return y < x; }
inline bool operator <= (IpRange const& x, IpRange const& y) { return !(y < x); }
inline bool operator >= (IpRange const& x, IpRange const& y) { return !(x < y); }
inline bool operator != (IpRange const& x, IpRange const& y) { return !(x == y); }

inline std::string IpRange::ntoa() const { return *this; }

// DEPRECATED methods
inline std::string IpRange::ntoa(IpRange const& r) { return r; }
inline IpRange IpRange::aton(std::string const& str) { return IpRange(str); }
inline IpAddr IpRange::GetLowerBound() const { return m_low; }
inline IpAddr IpRange::GetUpperBound() const { return m_high; }
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/* Pepa means Protocol end point address */

class EXPORT IpPepa
{
public:
	IpPepa() { }
	IpPepa(IpAddr const& addr, IpMask const& mask) : m_addr(addr), m_mask(mask) { }
	IpPepa(std::string const& s);
	// user compiler generated copy and assignment

	static char const SEPARATOR;	// character used between addr and mask
	
	// Implicit conversions
	operator IpAddr() const;
	operator IpMask() const;
	operator IpNet()  const;
	operator std::string() const;

	// Explicit conversions
	IpAddr GetAddr() const { return m_addr; }
	IpMask GetMask() const { return m_mask; }
	IpAddr GetHostAddr() const { return m_addr & ~m_mask; } // host portion of address
	IpAddr GetNetAddr() const { return m_addr & m_mask; }
	IpNet GetNet() const { return IpNet(m_addr & m_mask, m_mask); }

	void Set(IpAddr const& addr, IpMask const& mask);

	std::string ntoa() const; // for explicit conversion
	std::string ntoa(int addr_width, int mask_width) const; // control element widths

	// DEPRECATED methods
	static std::string ntoa(IpPepa const& pepa); // I don't know why anyone would use this in the first place
	static IpPepa aton(std::string const& str); // use ctor

private:
	IpAddr m_addr;	// end point address
	IpMask m_mask;	// mask for network enclosing address

	friend bool operator == (IpPepa const& ,IpPepa const&);
	friend bool operator < (IpPepa const& ,IpPepa const&);
	friend EXPORT std::istream& operator >> (std::istream& , IpPepa& );
	friend EXPORT std::ostream& operator << (std::ostream& , IpPepa const& );
};

inline bool operator == (const IpPepa& x, const IpPepa& y) { return	x.m_addr == y.m_addr && x.m_mask == y.m_mask; }
inline bool operator <  (const IpPepa& x, const IpPepa& y) { return x.m_addr < y.m_addr || ( x.m_addr == y.m_addr && y.m_mask < x.m_mask); }

inline bool operator >  (IpPepa const& x, IpPepa const& y) { return y < x; }
inline bool operator <= (IpPepa const& x, IpPepa const& y) { return !(y < x); }
inline bool operator >= (IpPepa const& x, IpPepa const& y) { return !(x < y); }
inline bool operator != (IpPepa const& x, IpPepa const& y) { return !(x == y); }

inline IpPepa::operator IpAddr() const { return m_addr; }
inline IpPepa::operator IpMask() const { return m_mask; }
inline IpPepa::operator IpNet() const { return IpNet(m_addr, m_mask); }

inline void IpPepa::Set(IpAddr const& a, IpMask const& m) { m_addr = a; m_mask = m; }

inline std::string IpPepa::ntoa() const { return *this; }

// DEPRECATED methods
inline IpPepa IpPepa::aton(std::string const& s) { return s; }
inline std::string IpPepa::ntoa(IpPepa const& pepa) { return pepa; }

/* ------------------------------------------------------------------------ */
} /* namespace Cn */
/* ------------------------------------------------------------------------ */
# pragma warning(pop)
# undef EXPORT
# endif /* inclusion protection */
