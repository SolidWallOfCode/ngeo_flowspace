// SPDX-License-Identifier: Apache-2.0
// Copyright 2001 Network Geographics
/* ------------------------------------------------------------------------ */
# if !defined(ISMG_CN_IP_HEADER)
# define ISMG_CN_IP_HEADER
/* ------------------------------------------------------------------------ */
# include <stl/vector>
# include <stl/set>
# include <cn-ip-base.h>
/* ------------------------------------------------------------------------ */
# if defined(ISMG_CN_IP_INTERNAL)
#   define EXPORT _declspec(dllexport)
# else
#   define EXPORT _declspec(dllimport)
# endif
/* ------------------------------------------------------------------------ */
namespace Cn
{
class IpCluster;  //forward declare
/* ------------------------------------------------------------------------- */
# pragma warning(push)
# pragma warning(disable:4251)	/* disable non dll-interface for IpMap class */
# pragma warning(disable:4275)	/* disable non dll-interface for IpMap class */
# pragma warning(disable:4786)	/* disable identifier too long */
/* ------------------------------------------------------------------------- */
class EXPORT IpProtocol
{
protected:
	char* m_proto;
	IpPort m_defaultPort;

public:
	IpProtocol() : m_proto(0) { }
};


class EXPORT IpService
{
protected:
	IpPortRange m_ports;
	IpProtocol m_proto;
};



class EXPORT IpSet
{
protected:
	typedef std::set<IpRange> Container;
	Container m_set;

	void Coalesce();

public:
	typedef Container::iterator iterator;

	IpSet() { }
	
	void Insert(const IpAddr& addr);
	void Insert(const IpRange& range);
	void Insert(IpSet set);

	void Remove(const IpAddr& addr);
	void Remove(const IpRange& range);
	void Remove(IpSet set);
	
	IpSet CalcOverlap(IpSet set);
	IpSet CalcOverlap(const IpRange& range);

	/* Check if there is a single range in the IpSet which contains the given range. */
	bool Contains(const IpRange& range);

	/* Check if there is a range in the IpSet which is exactly the given range. */
	bool IsMember(const IpRange& range);

	iterator begin() { return m_set.begin(); }
	iterator end() 	{ return m_set.end(); }
};



/* This contains an IP network and a set of addresses 
 * that are compatible with that network */
class EXPORT IpGroup 
{
protected:
	typedef std::vector<IpAddr> Container;
	Container m_addrs;
	IpNet m_net;

public:
	class iterator 
	{
	private:
		Container::iterator m_iter;
		IpPepa m_data;

		iterator(Container::iterator iter, IpMask mask) 
			: m_iter(iter), m_data(0, mask) { } 
		
	public:
		iterator() { }

		/* Iteration over Pepa */
		const IpPepa& operator *() 
		{ 
			m_data.Set(*m_iter, (IpMask) m_data);
			return m_data; 
		}

		const IpPepa* operator ->()
		{	return &m_data; }

		const iterator& operator ++() { ++m_iter; return *this; }
		const iterator& operator --() { --m_iter; return *this; }

		iterator operator ++ (int) { iterator tmp(*this); ++*this; return tmp; }
		iterator operator -- (int) { iterator tmp(*this); --*this; return tmp; }

		friend IpGroup;
		friend bool operator== (const iterator& iter1, const iterator& iter2 );
	};


	IpGroup(const IpNet& net) : m_net(net) { }

	const IpGroup& operator+= (const IpAddr& addr)
	{
		Insert(addr);
		return *this;
	}

	const IpGroup& operator-= (const IpAddr& addr)
	{
		Remove(addr);
		return *this;
	}

	friend IpGroup operator+(const IpGroup& group1, const IpGroup& group2);
	friend IpGroup operator-(const IpGroup& group1, const IpGroup& group2);
	friend bool operator == (const iterator& iter1, const iterator& iter2);
	friend bool operator == (const IpGroup& group1, const IpGroup& group2);
	friend bool operator <  (const IpGroup& group1, const IpGroup& group2);
	friend IpCluster;

	/* cast operator */
	operator IpNet() { return m_net; }
	

	iterator begin() { return iterator(m_addrs.begin(), (IpMask) m_net); }
	iterator end() { return iterator(m_addrs.end(), (IpMask) m_net); }

	Container::iterator addr_begin() { return m_addrs.begin(); }
	Container::iterator addr_end() { return m_addrs.end(); }
	
	/* Return the number of addresses in the container */
	int GetCount() const { return m_addrs.size(); }

	/* Insert an address into the bag. The insert fails 
	 * if the address is not compatible with the bag network */
	bool Insert(const IpAddr& addr);

	/* Remove an address from the bag. The remove fails 
	 * if the address is not present.*/
	bool Remove(const IpAddr& addr);

	/* Remove all address from the container. */
	void RemoveAll();

	/* Indicates whether an address is present in the container. */
	bool Contains(const IpAddr& addr);

	/* Indicates whether an address is valid to insert into the bag.*/
	bool IsCompatible(const IpAddr& addr) const
	{	return m_net.IsCompatible(addr); }

	/* This method checks if the network presented by this group is an subset of 
	 * the network presented by the given group. */
	bool IsSubset(const IpGroup& group) const
	{   return m_net.IsSubset(group.m_net); }

	bool HasOverlap(const IpGroup& group) const
	{	return m_net.HasOverlap(group.m_net); }

};

inline bool operator == (const IpGroup::iterator& iter1, const IpGroup::iterator& iter2)
{	return iter1.m_iter == iter2.m_iter && iter1.m_data == iter2.m_data; }

inline bool operator != (const IpGroup::iterator& iter1, const IpGroup::iterator& iter2)
{	return !(iter1 == iter2); }

inline
bool operator== (const IpGroup& group1, const IpGroup& group2)
{ return group1.m_net == group2.m_net && group1.m_addrs == group2.m_addrs; }

inline
bool operator < (const IpGroup& group1, const IpGroup& group2)
{ return group1.m_net < group2.m_net; }

inline
bool operator!= (const IpGroup& group1, const IpGroup& group2)
{ return !(group1 == group2); }


inline
IpGroup operator+(const IpGroup& group1, const IpGroup& group2)
{
	IpGroup group(group1);
	/* need tempGroup which is not const because const IpGroup object cannot call begin() and end(). */
	IpGroup tmpGroup(group2);
	
	for(IpGroup::iterator iter = tmpGroup.begin(); iter != tmpGroup.end(); ++iter)
		group.Insert( (IpAddr) (*iter) );
	
	return group;
}


inline
IpGroup operator-(const IpGroup& group1, const IpGroup& group2)
{
	IpGroup group(group1);
	/* need tempGroup which is not const because const IpGroup object cannot call begin() and end(). */
	IpGroup tmpGroup(group2);
	
	for(IpGroup::iterator iter = tmpGroup.begin(); iter != tmpGroup.end(); ++iter)
		group.Remove(*iter);
	
	return group;
	
}


/* A set of IpGroup objects such that all of the networks 
 * implied by the IpGroup objects are disjoint. */
class EXPORT IpCluster
{
protected:
	typedef std::vector<IpGroup> Container;
	Container m_groups;

public:
	typedef Container::iterator iterator;


	class pepa_iterator
	{
	private :
		IpGroup::iterator m_inner;
		iterator m_outer;
		const iterator m_begin;
		iterator m_end;
		pepa_iterator(const iterator& begin, const iterator& end) : 
		m_inner(begin->begin()), m_begin(begin), m_outer(begin), m_end(end) { }

	public :
		pepa_iterator() { }

		/* Iteration over addresses in gruops in the cluster */
		const IpPepa& operator *() 
		{	return *m_inner; }

		const pepa_iterator& operator ++() 
		{ 
			if( ++m_inner == m_outer->end() )
			{
				if(++m_outer != m_end)
					m_inner = m_outer->begin();
			}
		
			return *this; 
		}
		
		const pepa_iterator& operator --() 
		{
			if( m_inner == m_outer->begin() )
			{
				if(m_outer != m_begin)
					m_inner = (--m_outer)->end();
			}
			
			--m_inner;
			return *this; 
		}

		pepa_iterator operator ++ (int) { pepa_iterator tmp(*this); ++*this; return tmp; }
		pepa_iterator operator -- (int) { pepa_iterator tmp(*this); --*this; return tmp; }

		friend IpCluster;
		friend bool operator== (const pepa_iterator& iter1, const pepa_iterator& iter2);
	};

	
	IpCluster() {}
	

	/* Accumulate operator. The given group is added to 
	 * the target cluster if non-conflicting. 
	 */
	const IpCluster& operator+= (const IpGroup& group)
	{ Insert(group);  return *this; }


	const IpCluster& operator-= (const IpGroup& group)
	{ Remove(group);  return *this; }

	friend IpCluster operator+(const IpCluster& group1, const IpCluster& group2);
	friend IpCluster operator-(const IpCluster& group1, const IpCluster& group2);
	friend bool operator== (const pepa_iterator& iter1, const pepa_iterator& iter2);

	
	/* Insert a network (and implied group) into the cluster */
	bool Insert(const IpNet& net);

	/* Insert an address into the cluster. 
	 * It is placed in the compatible group, if any. */
	bool Insert(const IpAddr& addr);

	/* Insert a group into the cluster. If the network is non-conflicting 
	 * it and all of its addresses are added to the cluster. */
	bool Insert(const IpGroup& group);

	bool Remove(const IpGroup& group);
	
	void RemoveAll();

	bool Contains(const IpGroup& group);

	bool Contains(const IpAddr& addr);


	/* Return an iterator to the first group in the cluster */
	iterator begin()
	{ return m_groups.begin(); }

	/* Return an iterator to one past the last group in the cluster */
	iterator end()
	{ return m_groups.end(); }

	IpGroup::iterator pepa_begin()
	{ return m_groups.begin() -> begin(); }

	IpGroup::iterator pepa_end()
	{ return m_groups.end() -> end(); }
};



inline
IpCluster operator+(const IpCluster& cluster1, const IpCluster& cluster2)
{
	IpCluster cluster(cluster1);

	/* need tempGroup which is not const because const IpCluster object cannot call begin() and end(). */
	IpCluster c(cluster2);
	
	for(IpCluster::iterator iter = c.begin(); iter != c.end(); ++iter)
		cluster.Insert(*iter);
	
	return cluster;
}



inline
IpCluster operator-(const IpCluster& cluster1, const IpCluster& cluster2)
{
	IpCluster cluster(cluster1);
	/* need tempGroup which is not const because const IpCluster object cannot call begin() and end(). */
	IpCluster c(cluster2);
	
	for(IpCluster::iterator iter = c.begin(); iter != c.end(); ++iter)
		cluster.Remove(*iter);
	
	return cluster;	
}


inline bool operator== (const IpCluster::pepa_iterator& iter1, const IpCluster::pepa_iterator& iter2)
{
	return iter1.m_inner == iter2.m_inner && iter1.m_outer == iter2.m_outer 
        && iter1.m_begin == iter2.m_begin && iter1.m_end == iter2.m_end;
}


inline bool operator!= (const IpCluster::pepa_iterator& iter1, const IpCluster::pepa_iterator& iter2)
{
	return !(iter1 == iter2);
}

/* ------------------------------------------------------------------------ */
} /* namespace Cn */
/* ------------------------------------------------------------------------ */
# undef EXPORT
# pragma warning(pop)
# endif /* inclusion protection */
