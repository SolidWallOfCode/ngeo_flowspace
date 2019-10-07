// SPDX-License-Identifier: Apache-2.0
// Copyright 2001 Network Geographics
/* ------------------------------------------------------------------------ */
# pragma warning(disable:4786)	/* disable identifier too long */
# include "export/cn-ip-base.h"
# include <stl/iomanip>
# include <stl/sstream>
# include <assert.h>
/* ------------------------------------------------------------------------ */

namespace Cn
{
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
// Separating characters for multi-part objects
char const IpNet::SEPARATOR = '/';
char const IpPortRange::SEPARATOR = '-';
char const IpRange::SEPARATOR = '-';
char const IpPepa::SEPARATOR = IpNet::SEPARATOR;
/* ------------------------------------------------------------------------ */
namespace
{
// templating doesn't work because of compiler problems
std::istream& SkipRangeSeparator(std::istream& s) { s.ignore(std::numeric_limits<int>::max(), IpRange::SEPARATOR); return s; }
std::istream& SkipPortRangeSeparator(std::istream& s) { s.ignore(std::numeric_limits<int>::max(), IpPortRange::SEPARATOR); return s; }
std::istream& SkipNetSeparator(std::istream& s) { s.ignore(std::numeric_limits<int>::max(), IpNet::SEPARATOR); return s; }
std::istream& SkipPepaSeparator(std::istream& s) { s.ignore(std::numeric_limits<int>::max(), IpPepa::SEPARATOR); return s; }
}
/* ------------------------------------------------------------------------ */
IpPort::IpPort(std::string const& s)
{
	std::istringstream strm(s);
	strm >> *this;
}
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
IpPortRange::IpPortRange(std::string const& str)
{
	std::istringstream s(str);
	s >> *this;
}

IpPortRange::operator std::string () const
{
	std::ostringstream s;
	s << *this;
	return s.str();
}

/* Proof - (for you, Dr. Zivaljevic)

For two ranges RA and RB, we define overlap to mean that there is a non
empty intersection.

Definition: min(R) == minimal value in R.
			max(R) == maximal value in R

It must be the case that either
Case 1) min(RA) <= min(RB)
or
Case 2)min(RB) < min(RA).

Case 1 -
A)	if min(RB) is a member of RA then the intersection consists of at least min(RB)
	and therefore is non-empty.
B)	if there is a value N that is a member of RB and RA, then by definition
	it must be that min(RA) <= N <= max(RA) and min(RB) <= N <= max(RB). We
	assumed that min(RA) <= min(RB) so we have min(RA) <= min(RB) <= N <= max(RA)
	therefore if RA overlaps RB then min(RB) is an element of RA.

Therefore if min(RA) <= min(RB) there is overlap iff min(RB) is a member of RA

Case 2 -
The same logic applies as in Case (1) by exchanging RA and RB, therefore
if min(RB) < min(RA) there is overlap iff min(RA) is a member of RB.

Combining the two cases, we have that in the general case, there is overlap iff
min(RA) is an element of RB _OR_ min(RB) is an element of RA.

*/
bool 
IpPortRange::HasOverlap(const IpPortRange& src) const
{
	return src.IsCompatible(m_low) || this->IsCompatible(src.m_low);
}


bool 
IpPortRange::CalcOverlap(IpPortRange& out, const IpPortRange& src) const
{
	if( HasOverlap(src) )
	{
		out.m_high = std::min(m_high, src.m_high);
		out.m_low = std::max(m_low, src.m_low);
		return true;
	}
	return false;
}


bool 
IpPortRange::HasUnion(const IpPortRange& src) const
{
	return IpPortRange::HasOverlap(src) || IpPortRange::IsAdjacentTo(src);
}


bool 
IpPortRange::CalcUnion(IpPortRange& out, const IpPortRange& src) const
{
	if(IpPortRange::HasUnion(src))
	{
		out.m_low = std::min(src.m_low, m_low) ;
		out.m_high= std::max(m_high, src.m_high);
		return true;
	}
	return false;
}


bool 
IpPortRange::IsAdjacentTo(const IpPortRange& other) const
{ 
	return m_high.GetRaw() + 1 == other.m_low.GetRaw() 
		|| other.m_high.GetRaw() + 1 == m_low.GetRaw();  
}

bool 
IpPortRange::IsSubset(const IpPortRange& other) const
{ return m_high.GetRaw() <= other.m_high.GetRaw() 
	  && m_low.GetRaw() >= other.m_low.GetRaw();}



std::ostream& operator << (std::ostream& s,  const IpPortRange& p)
{ 
	return s << p.m_low << IpPortRange::SEPARATOR << p.m_high;
}

std::istream& operator >> (std::istream& s, IpPortRange& p)
{ 
	IpPort l, h;	// temporaries

	if (s >> std::ws)
	{
		if (IpPortRange::SEPARATOR == s.peek()) // of the form "-###"
		{
			l = IpPort::Min();
			s.get(); // skip separator
			s >> h;
		}
		else if (s >> l) // no leading separator
		{
			s >> SkipPortRangeSeparator;
			if (s.eof())
			{
				h = l; // assume it's a singleton
				s.clear(s.rdstate() & ~std::ios::eofbit);
			}
			else if (s.peek() == EOF) h = IpPort::Max();
			else s >> h;
		}

		p.Set(l, h);	// handles flip if needed
	}

	return s;
}

void
IpPortRange::Set(std::string const& text)
{
	std::istringstream s(text);
	s >> *this;
}
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
// Read from a stream, presuming it to be in octet form. Return false
// if impropertly formatted. In this case a is left with the partial
// result. The IpMask logic depends on this.
namespace { // mark these as file local

bool 
ReadOctetsFrom(std::istream& s, IpAddr::Type& a)
{
	IpAddr::Type u;
	int c;
	a = 0;			// set to known state
	
	for (int i = 0 ; i < 3 ; ++i )	// get four octets
	{
		s >> u;			// read next octet
		c = s.get();	// eat separator
		a <<= 8;		// shift current value
		a += u;			// add in current value
		if (c != '.' || u > 0xFF) // verify separator, octet
		{
			s.setstate(std::ios::failbit);
			return false;
		}
	} 


	s >> u;			// read the last octet

	assert(u <= 0xFF);
	a <<= 8;	// shift current value
	a += u;	// add in current value

	return true;
}

bool ReadOctetsFrom(std::string const& text, IpAddr::Type& a)
{
	std::istringstream s(text);
	return ReadOctetsFrom(s, a);
}


void
WriteOctetsTo(std::ostream& s, IpAddr::Type a)
{
	s	<< ((a>>24)&0xFF) << "." << ((a>>16)&0xFF) << "."
		<< ((a>>8)&0xFF)  << "." << (a&0xFF);
}

void 
WriteOctetsTo(std::ostream& s, IpAddr::Type a, int w)
{
	s	<< std::setw(w) << std::setfill(' ') << ((a>>24)&0xFF) << "." 
		<< std::setw(w) << std::setfill(' ') << ((a>>16)&0xFF) << "."
		<< std::setw(w) << std::setfill(' ') << ((a>>8)&0xFF)  << "." 
		<< std::setw(w) << std::setfill(' ') << (a&0xFF);
}

void
WriteOctetsTo(std::string& str, IpAddr::Type a)
{
	std::ostringstream s(str);
	WriteOctetsTo(s, a);
}

void
WriteOctetsTo(std::string& str, IpAddr::Type a, int w)
{
	std::ostringstream s(str);
	WriteOctetsTo(s, a, w);
}

} // nameless namespace
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
IpAddr::IpAddr(std::string const& s)
{
	if (!ReadOctetsFrom(s, m_addr))
		m_addr = 0;
}

IpAddr::operator std::string () const
{
	std::ostringstream os;
	WriteOctetsTo(os, m_addr);
	return os.str();
}

void 
IpAddr::ntoa(std::string& str) const
{
	WriteOctetsTo(str, m_addr);
}


std::string 
IpAddr::ntoa(int w) const
{
	std::ostringstream os;
	WriteOctetsTo(os, m_addr, w);
	return os.str();
}

void 
IpAddr::ntoa(std::string& str, int w) const
{
	WriteOctetsTo(str, m_addr, w);
}

std::istream& operator >> (std::istream& s, IpAddr& a)
{
	ReadOctetsFrom(s, a.m_addr);
	return s;
}

std::ostream& operator << (std::ostream& s,  const IpAddr& a)
{
	WriteOctetsTo(s, a.m_addr);
	return s;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
IpMask::IpMask(std::string const& str)
{
	std::istringstream s(str);
	s >> *this;
}

IpMask::operator std::string () const
{
	std::ostringstream s;
	s << *this;
	return s.str();
}

void 
IpMask::ntoa(std::string& str) const
{
	std::ostringstream os(str);
	os << static_cast<IpMask> (*this);
}

std::istream& operator >> (std::istream& s, IpMask& m)
{
	IpAddr::Type u;
	if (ReadOctetsFrom(s, u))	// it's in octet form
	{
		int width(m.ValidCount(u));
		m.m_mask = width < 0 ? 0 : width;
	}
	else if (u <= IpMask::WIDTH) // not, but first number is valid mask value
	{
		m.m_mask = u;
		s.clear(s.rdstate() & ~(std::ios::failbit | std::ios::eofbit));
	}
	else	// just not right.
		m.m_mask = 0;
	return s;
}

// returns negative if the mask is invalid
int
IpMask::ValidCount(IpAddr addr)
{
	IpAddr::Type a = addr.GetRaw();
	IpAddr::Type m = ~static_cast<IpAddr::Type>(0);	// all ones

	for ( int i = WIDTH ; i >= 0 && a != m ; --i, m <<= 1 )
		;

	return i;
}
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
IpNet::IpNet(std::string const& str)
{
	std::istringstream s(str);
	s >> *this;
}

IpNet::operator std::string () const
{
	std::ostringstream s;
	s << *this;
	return s.str();
}

std::string 
IpNet::ntoa(int addr_width, int mask_width) const
{
	std::ostringstream s;
	s << m_addr.ntoa(addr_width) << IpNet::SEPARATOR << m_mask.ntoa(mask_width);
	return s.str();
}

// need to use temporaries so that we can guarantee that the address and
// mask are compatible.
std::istream& operator >> (std::istream& s, IpNet& net)
{
	IpAddr a;
	IpMask m;
	if (s >> a >> SkipNetSeparator >> m) // validate good read
		net.Set(a, m); // fixup address if necessary
	return s;
}

std::ostream& operator << (std::ostream& s, const IpNet& net)
{
	return s << net.m_addr << IpNet::SEPARATOR << net.m_mask;
}
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

IpRange::IpRange(std::string const& str)
{
	std::istringstream s(str);
	s >> *this;
}

// Compute the unique minimal set of networks that exactly cover the range.
// Return the number of networks generated. Do not assume that the incoming
// vector is initially empty and append our results.
int 
IpRange::GenerateNetworksImpl(NetInserter& insert) const
{
	/* special case which can not be handled in the do-while loop
	 * because size wraps (goes to 0) in this case.
	 */
	if (m_low == IpAddr::Min() && m_high == IpAddr::Max())
	{
		insert(IpNet(0,0));
		return 1;
	}

	// =high= and =low= are used to track the part of the range
	// not yet covered.
	IpAddr::Type high = this->GetUpperBound().GetRaw();
	IpAddr::Type low = this->GetLowerBound().GetRaw();
	IpAddr::Type size = high - low + 1;		// # of addresses in the range [low, high]

	IpNet net;	// most recently generated net
	int count = 0;	// # of generated networks

	// Each iteration we compute =k= and =n=.
	// =k= is the largest value such as 2^k is no larger than
	// the size of [low, high]
	// =n= is the smallest value such that there is a valid network
	// starting at =low= with a mask of =n= bits.
	// We generate a network from the smaller of these and remove
	// it from the remaining range by bumping up =low=.
	do	{

		// find the bit index of the highest order set bit of =size=
		assert(size);
		int k = -1;
		while(size != 0)
		{
			size >>= 1;
			++k;
		}

		// find the bit index of the lowest order set bit of =low=
		int n = IpAddr::WIDTH;
		IpAddr::Type base(low);	// destroyed, so must use copy
		while(base)
		{
			base <<= 1;
			--n;
		}

		// The best we can do is the smaller of the maximum size
		// and largest valid network
		net.Set(low, IpMask(IpAddr::WIDTH - std::min(n, k)));
		insert(net);
		++count;

		// update the remaining range and its size
		low = (net.GetUpperBound() + 1).GetAddress();
		size = high - low + 1;

		// need to be very careful on the end condition, because we can
		// run up against the max address. This check tests whether the
		// last address (=high=) was included in the last generated network.
		// If so, we're done. =low= and =size= can get scrozzled if
		// the generated network includes the max address, but in that
		// case this condition will always be false so we won't use those
		// bogus values.
	} while(net.GetUpperBound() < this->GetUpperBound());

	return count;
}

int
IpRange::GenerateNetworks(std::vector<IpNet>& networks) const
{
	return this->GenerateNetworks(std::back_inserter(networks));
}


bool 
IpRange::CalcOverlap(IpRange& out, const IpRange& src) const
{
	if(HasOverlap(src))
	{
		out.m_high = std::min(m_high, src.m_high);
		out.m_low = std::max(src.m_low, m_low);
		return true; 
	}
	else
		return false;
}


bool 
IpRange::HasUnion(const IpRange& src) const
{
	return IpRange::HasOverlap(src) || IpRange::IsAdjacentTo(src);
}


bool 
IpRange::CalcUnion(IpRange& out, const IpRange& src) const
{
	if(HasUnion(src))
	{
		out.m_low = std::min(src.m_low, m_low);
		out.m_high= std::max(m_high, src.m_high);
		return true;
	}
	else
		return false;
}


bool 
IpRange::IsAdjacentTo(const IpRange& other) const
{ return m_high + 1 == other.m_low || other.m_high + 1 == m_low;  }


// see proof for IpPortRange::HasOverlap
bool 
IpRange::HasOverlap(const IpRange& src) const
{
	return src.IsCompatible(m_low) || this->IsCompatible(src.m_low);
}


bool 
IpRange::IsSubset(const IpRange& other) const
{
	return m_high <= other.m_high && m_low >= other.m_low;
}

void
IpRange::Set(std::string const& text)
{
	std::istringstream s(text);
	s >> *this;
}

IpRange::operator std::string() const
{
	std::ostringstream s;
	s << *this;
	return s.str();
}

std::string 
IpRange::ntoa(int width) const
{
	std::ostringstream s;
	// must use ntoa() in order to propagate the width
	s << m_low.ntoa(width) << SEPARATOR << m_high.ntoa(width);
	return s.str();
}

std::istream& operator >> (std::istream& s, IpRange& r)
{
	IpAddr l, h;	// temporaries

	if (s >> std::ws) // skip if stream in bad state
	{
		if (IpRange::SEPARATOR == s.peek()) // of the form "-###"
		{
			l = IpAddr::Min();
			s.get(); // skip separator
			s >> h;
		}
		else if (s >> l)// no leading separator
		{
			s >> SkipRangeSeparator;
			if (s.eof())
			{
				h = l; // assume it's a singleton
				s.clear(s.rdstate() & ~std::ios::eofbit);
			}
			else if (s.peek() == EOF) h = IpAddr::Max();
			else s >> h;
		}

		r.Set(l, h);	// handles flip if needed
	}

	return s;
}


std::ostream& operator << (std::ostream& s, const IpRange& r)
{
	return s << r.GetLower() << IpRange::SEPARATOR << r.GetUpper();
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

IpPepa::IpPepa(std::string const& str)
{
	std::istringstream s(str);
	s >> *this;
}

IpPepa::operator std::string() const
{
	std::ostringstream s;
	s << m_addr << IpPepa::SEPARATOR << m_mask;
	return s.str();
}

std::string 
IpPepa::ntoa(int addr_width, int mask_width) const
{
	std::ostringstream s;
	s << m_addr.ntoa(addr_width) << IpPepa::SEPARATOR << m_mask.ntoa(mask_width);
	return s.str();
}

std::istream& operator >> (std::istream& s, IpPepa& pepa)
{
	return s >> pepa.m_addr >> SkipPepaSeparator >> pepa.m_mask;
}


std::ostream& operator << (std::ostream& s, IpPepa const& pepa)
{
	return s << pepa.m_addr << IpPepa::SEPARATOR << pepa.m_mask;
}
/* ------------------------------------------------------------------------ */
# if defined(_DEBUG)
// This class just does various numeric checking
class ValidateNumerics
{
public:
	ValidateNumerics()
	{
		// check that IpPort fundamental types has the right # of bits
		assert(std::numeric_limits<IpPort::Type>::digits == IpPort::WIDTH);
		// check that IpAddr fundamental types has the right # of bits
		assert(std::numeric_limits<IpAddr::Type>::digits == IpAddr::WIDTH);
	}
};

namespace { ValidateNumerics Validation; }
# endif
/* ------------------------------------------------------------------------ */
} /* namespace Cn */
/* ------------------------------------------------------------------------ */
