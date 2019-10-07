// SPDX-License-Identifier: Apache-2.0
// Copyright 2001 Network Geographics
/* ------------------------------------------------------------------------ */
# pragma warning(disable:4786)	/* disable identifier too long */
# include <cnip.h>
/* ------------------------------------------------------------------------ */
namespace Cn
{
/* ------------------------------------------------------------------------ */

void 
IpSet::Insert(const IpAddr& addr)
{
	IpRange range(addr);
	Insert(range);	
}	

void
IpSet::Insert(const IpRange& range)
{
	iterator iter = m_set.lower_bound(range);
	IpRange tmpRange;
	
	if( range.HasOverlap(* --iter) )
		tmpRange = * iter;
	else 
		tmpRange = * ++iter;

	while(iter != end() && range.HasOverlap(*iter))
		m_set.erase(iter);

	m_set.insert( IpRange( std::min(tmpRange.GetLowerBound(), range.GetLowerBound()) ,
		          std::max( range.GetUpperBound(), (* --iter).GetUpperBound() ) ));

	Coalesce();
}

void 
IpSet::Insert(IpSet set)
{
	iterator iter = set.begin();
	
	while( iter != set.end() )
	{
		Insert(*iter);
		++iter;
	}
}

void 
IpSet::Remove(const IpAddr& addr)
{
	IpRange range(addr);
	Remove(range);
}

void 
IpSet::Remove(const IpRange& range)
{
	iterator iter = m_set.begin();
	while( iter != m_set.end() )
	{
		if( (*iter).HasOverlap(range) )
		{

			IpRange tmpRange =  *iter, overlap;
			iter = m_set.erase(iter);
			tmpRange.CalcOverlap(overlap, range);

			if( tmpRange.GetLowerBound() <= (overlap.GetLowerBound() - 1) )
			{
				iter = m_set.insert(iter, IpRange(tmpRange.GetLowerBound(), overlap.GetLowerBound() - 1));
				++iter;
			}

			if( iter != m_set.end() && (overlap.GetUpperBound() + 1) <= tmpRange.GetUpperBound() )
			{
				iter = m_set.insert(iter, IpRange(tmpRange.GetLowerBound(), overlap.GetLowerBound() - 1));
			}
		}
		else
			++iter;
	}
}

void
IpSet::Remove(IpSet set)
{
	iterator iter = set.begin();
	while(iter != set.end())
		Remove(*iter++);
}

IpSet 
IpSet::CalcOverlap(IpSet set)
{
	IpSet tmpSet;
	iterator iter = set.begin();
	while(iter != set.end() )
		tmpSet.Insert(CalcOverlap(*iter));

	return tmpSet;
}

IpSet 
IpSet::CalcOverlap(const IpRange& range)
{
	IpSet set;
	iterator iter = begin();
	while(iter != end())
	{
		IpRange overlap, tmpRange;
		range.CalcOverlap(overlap, *iter);

		if(overlap != tmpRange)
			set.Insert(overlap);
	}
	return set;
}

bool   /* If the given range is totally involved in a 
single range in IpSet, this function will retrue true. */
IpSet::Contains(const IpRange& range)
{
	iterator iter = begin();
	while( iter != end() && !range.IsSubset(*iter) )
		++iter;
	
	if(iter == end())
		return false;
	else 
		return true;
}

bool 
IpSet::IsMember(const IpRange& range)
{
	iterator iter = m_set.lower_bound(range);
	
	return range == (*iter);
}

void 
IpSet::Coalesce()
{
	iterator iter = begin();
	IpRange former, later;
	
	if(iter != end())
		former = *iter++;

	while( iter != end() )
	{
		later = *iter;

		if( later.GetLowerBound() -1 == former.GetUpperBound() )
		{
			iter = m_set.erase(--iter);
			iter = m_set.erase(iter);
			former = IpRange(former.GetLowerBound(), later.GetUpperBound() );
			m_set.insert( former );

			++iter;
		}
		else
		{
			former = later;
			later = * ++iter;
		}
	}
}


bool 
IpGroup::Insert(const IpAddr& addr)
{ 
	if(IsCompatible(addr))
	{
		m_addrs.push_back(addr);
		return true;
	}
	else
		return false;
}


bool 
IpGroup::Remove(const IpAddr& addr)
{ 
	iterator it = begin();
	while ( it != end() && (IpAddr) (*it) != addr )
	{ ++it; }

	if(it != end())
	{
		m_addrs.erase(it.m_iter); 
		return true;
	}
	else
		return false;
}


void 
IpGroup::RemoveAll()
{
	iterator it = begin();
	while( it != end() )
		m_addrs.erase(it.m_iter);
}


bool 
IpGroup::Contains(const IpAddr& addr)
{
	iterator it = begin();
	while( it!= end() && (IpAddr) (*it) != addr )
	{ ++it; }
	
	return it!= end();
}



bool 
IpCluster::Insert(const IpNet& net)
{
	IpGroup group(net);
	return Insert(group);
}


bool 
IpCluster::Insert(const IpAddr& addr)
{
	IpCluster::iterator pos = begin();
	while ( pos != end() && !(*pos).IsCompatible(addr) )
	{ ++pos; }
	
	if( pos != end() )
		return (*pos).Insert(addr); 
	else
		return false;
}


/* lower_bound(sth) returns the iterator which refers to 
 * the first element which is not less than "sth". so to 
 * judge if the new group disjoint to those in the cluster,
 * we only need to look at the element which the iterator 
 * points to and the element before it.
 */
bool  
IpCluster::Insert(const IpGroup& group)
{
	IpCluster::iterator pos = begin();
	while ( pos != end() && !(*pos).HasOverlap(group) )
	{ ++pos; }
	
	if( pos != end() )
		return false;  /* New group is not compatible for the cluster. */
	else
	{
		/* New group is compatible to the cluster, so insert the group. */
		m_groups.push_back(group); 
		return true;
	}
}



bool 
IpCluster::Remove(const IpGroup& group)
{
	iterator pos = begin();
	while ( pos != end() && (*pos) != group )
	{ ++pos; }

	if( pos != end() )
	{
		m_groups.erase(pos);
		return true;
	}
	else
		return false;
}


void
IpCluster::RemoveAll()
{
	iterator pos = begin();
	while ( pos != end() )
		m_groups.erase(pos);
}


bool
IpCluster::Contains(const IpGroup& group)
{
	IpCluster::iterator pos = begin();
	while ( pos != end() && (*pos) != group )
	{ ++pos; }

	if( pos != end() )
		return true;
	else
		return false;
}


bool
IpCluster::Contains(const IpAddr& addr)
{
	iterator pos = begin();
	while ( pos != end() && !(*pos).IsCompatible(addr) )
	{ ++pos; }

	if(pos != end())
		return (*pos).Contains(addr);
	else
		return false;

}



/*---------------------------------------------------------------*/
}  /* end of namespace Cn */
/*---------------------------------------------------------------*/
