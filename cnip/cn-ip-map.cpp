// SPDX-License-Identifier: Apache-2.0
// Copyright 2001 Network Geographics
/* --------------------------------------------------------------------------- */
# pragma warning(disable:4786)	/* disable identifier too long */
/* --------------------------------------------------------------------------- */
# include <cn-ip-map.h>
# include <cn-ip-base.h>
/* --------------------------------------------------------------------------- */


namespace Cn
{
/* implementation of left part of paint/unpaint/uncolor function */
void
IpMap::Pt_left(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair)
{
	if( pos == m_map.begin() ) { /* Do nothing in this case. */ }
	else if( range.HasOverlap((--pos)->first) )
		LeftSkew(tmpPair, pos, range);
	else
		++pos;
}


/* implementation of middle part of paint/uncolor function */
void
IpMap::Pt_middle(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair, bool& isLeftSpan)
{
	while( pos != m_map.end() && (pos->first).IsSubset(range) )
		pos = m_map.erase(pos);

	if( range.HasOverlap(pos->first) )
	{
		tmpPair = *pos;
		isLeftSpan = false;
	}
}


/* implementation of right part of paint function */
void 
IpMap::Pt_right(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair, bool& isLeftSpan)
{
	if( tmpPair != Pair() )
	{
		if(!isLeftSpan)   /* in this case, right end of span overlaps with a span in m_spans */
			pos = m_map.erase(pos);
			
		pos = m_map.insert(pos, std::make_pair(range, color));	/* paint the range with color */
			
		/* insert the right half span into m_spans */
		InsertToMap( range.GetUpperBound() + 1, (tmpPair.first).GetUpperBound(), tmpPair.second, m_map, ++pos);
	}
	else	/* in this case, span is not overlap with any span in m_spans at both sides */
		m_map.insert(pos, std::make_pair(range, color));
}


/* implementation of left part of unpaint function */
void
IpMap::Upt_left(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair)
{
	if( pos == m_map.begin() ) { /* Do nothing in this case. */ }
	else if( range.HasOverlap((--pos)->first) && *(pos->second) == *color )
		LeftSkew(tmpPair, pos, range);
	else
		++pos;
}


/* implementation of middle part of unpaint function */
void 
IpMap::Upt_middle(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair, bool& isLeftSpan)
{
	while( pos != m_map.end() && (pos->first).IsSubset(range))
	{
		if( *(pos->second) == *color )
			pos = m_map.erase(pos);
		else
			++pos;
	}

	if( range.HasOverlap(pos->first) )
	{
		tmpPair = *pos;
		isLeftSpan = false;
	}
}


/* implementation of right part of unpaint/uncolor function */
void 
IpMap::Upt_right(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair, bool& isLeftSpan)
{
	if( tmpPair != Pair() )
	{
		if(!isLeftSpan)   /* in this case, right end of span overlaps with a span in m_spans */
			pos = m_map.erase(pos);
			
		/* insert the right half span into m_spans */
		InsertToMap( range.GetUpperBound() + 1, tmpPair.first.GetUpperBound(), tmpPair.second, m_map, ++pos);
	}
}


/* implementation of left part of blend function */
void 
IpMap::Bd_left(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair)
{
	if( pos == m_map.begin() ) { }
	else if( range.HasOverlap( (--pos)->first) )
	{   
		LeftSkew(tmpPair, pos, range);
		
		InsertToMap(range.GetLowerBound(), std::min(range.GetUpperBound(), (tmpPair.first).GetUpperBound()), (tmpPair.second) + color, m_map, pos);
		++pos;
	}
	else
		++pos;
}


/* implementation of middle part of blend function */
void 
IpMap::Bd_middle(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair, bool& isLeftSpan)
{
	Pair lastPair = tmpPair;

	/* deal with the spans which totally reside in the range */
	while( pos != m_map.end() && (pos->first).IsSubset(range) )
	{
		bool insert = false;
		if(lastPair == Pair())
			insert = InsertToMap( range.GetLowerBound(), (pos->first).GetLowerBound() - 1, color, m_map, pos);
		else
			insert = InsertToMap( (lastPair.first).GetUpperBound() + 1, (pos->first).GetLowerBound() - 1, color, m_map, pos);
			
		if(insert)
				++pos;

		lastPair = *pos;
		pos = m_map.erase(pos);

		InsertToMap( (lastPair.first).GetLowerBound(), (lastPair.first).GetUpperBound(), (lastPair.second) + color, m_map, pos);
		++pos;
	}

	if( range.HasOverlap(pos->first) )
	{
		tmpPair = *pos;
		isLeftSpan = false;
		
		if( pos!= m_map.begin() )
		{
			InsertToMap( (lastPair.first).GetUpperBound() + 1, (pos->first).GetLowerBound() - 1, color, m_map, pos);
		}
	}
	else
		InsertToMap( (lastPair.first).GetUpperBound() + 1, range.GetUpperBound(), color, m_map, pos);
}



/* implementation of right part of blend function */
void 
IpMap::Bd_right(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair, bool& isLeftSpan)
{
	/* deal with the right overlap */
	if( tmpPair != Pair() )
	{
		if(!isLeftSpan)   /* in this case, right end of span overlaps with a span in m_spans */
		{
			pos = m_map.erase(pos);

			InsertToMap( (tmpPair.first).GetLowerBound(), range.GetUpperBound(), (tmpPair.second) + color, m_map, ++pos);
		}

		/* insert the right half span into m_spans */
		if( range.GetUpperBound() < (tmpPair.first).GetUpperBound() )
			InsertToMap( range.GetUpperBound() + 1, ((tmpPair.first)).GetUpperBound(), tmpPair.second, m_map, ++pos);
	}
	else	/* in this case, span is not partially overlap with any span in m_spans */
		m_map.insert(pos, std::make_pair(range, color));
}
	
	
/* implementation of the left part of unblend function */
void 
IpMap::Ubd_left(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair)
{
	if( pos == m_map.begin() ) { }
	else if( range.HasOverlap( (--pos)->first) )
	{   
		bool insert = false;

		LeftSkew(tmpPair, pos, range);

		insert = InsertToMap(range.GetLowerBound(), std::min(range.GetUpperBound(), (tmpPair.first).GetUpperBound()), tmpPair.second - color, m_map, pos);
			
		if(insert)
			++pos;
	}
	else
		++pos;
}


/* implementation of middle part of blend function */
void 
IpMap::Ubd_middle(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair, bool& isLeftSpan)
{
	//Pair lastPair = * pos;

	/* deal with the spans which totally reside in the range */
	while( pos != m_map.end() && (pos->first).IsSubset(range) )
	{
		Pair lastPair = * pos;
		pos = m_map.erase(pos);

		InsertToMap( (lastPair.first).GetLowerBound(), (lastPair.first).GetUpperBound(), lastPair.second - color, m_map, pos);
		++ pos;
		//lastPair = * pos++;
	}

	if( range.HasOverlap(pos->first) )
	{
		tmpPair = *pos;
		isLeftSpan = false;
	}
}


/* implementation of right part of blend function */
void 
IpMap::Ubd_right(const IpRange& range, const Handle& color, iterator& pos, Pair& tmpPair, bool& isLeftSpan)
{
	/* deal with the right overlap */
	if( tmpPair != Pair() )
	{
		if(!isLeftSpan)   /* in this case, right end of span overlaps with a span in m_spans */
		{
			pos = m_map.erase(pos);

			InsertToMap( (tmpPair.first).GetLowerBound(), range.GetUpperBound(), (tmpPair.second) - color, m_map, ++pos);
		}

		/* insert the right half span into m_spans */
		if( range.GetUpperBound() < (tmpPair.first).GetUpperBound() )
			InsertToMap( range.GetUpperBound() + 1, (tmpPair.first).GetUpperBound(), tmpPair.second, m_map, ++pos);
	}
}


void 
IpMap::Paint(IpMap src)
{
	for(iterator iter=src.m_map.begin(); iter != src.m_map.end(); iter++)
		Paint( iter->first, iter->second );
}


void 
IpMap::Paint(IpMap src, const Handle& color)
{
	for(iterator iter=src.m_map.begin(); iter != src.m_map.end(); iter++)
		Paint( iter->first, color );
}



void 
IpMap::Unpaint(IpMap src)
{
	for(iterator iter=src.m_map.begin(); iter != src.m_map.end(); iter++)
		Unpaint( iter->first, iter->second );
}



void 
IpMap::Unpaint(IpMap src, const Handle& color)
{
	for(iterator iter=src.m_map.begin(); iter != src.m_map.end(); iter++)
		Unpaint( iter->first, color );
}


void
IpMap::UnColor(IpMap src)
{
	for(iterator iter=src.m_map.begin(); iter != src.m_map.end(); ++iter)
		UnColor( iter->first );
}



void 
IpMap::Blend(IpMap src)
{
	for(iterator iter=src.m_map.begin(); iter != src.m_map.end(); ++iter)
		Blend( iter->first, iter->second );
}


void 
IpMap::Blend(IpMap src, const Handle& color)
{
	for(iterator iter=src.m_map.begin(); iter != src.m_map.end(); ++iter)
		Blend( iter->first, color );
}



void 
IpMap::Unblend(IpMap src)
{
	for(iterator iter=src.m_map.begin(); iter != src.m_map.end(); ++iter)
		Unblend( iter->first, iter->second );
}


void 
IpMap::Unblend(IpMap src, const Handle& color)
{
	for(iterator iter=src.m_map.begin(); iter != src.m_map.end(); ++iter)
		Unblend( iter->first, color );
}



void
IpMap::Coalesce()
{
	iterator pos, next_pos, tmp_pos, begin_pos;
	next_pos = begin();
	pos = next_pos++;
	IpAddr low, high;

	while( pos != end() )
	{
		begin_pos = pos;
		low = pos->first.GetLowerBound();
		while ( pos->first.GetUpperBound() == (next_pos->first).GetLowerBound() - 1  
	  	     && *(pos->second) == *(next_pos->second) )
		{ 	
			high = (next_pos->first).GetUpperBound();
			pos = next_pos ++;
		}

		if(begin_pos != pos)
		{
			Handle tmpColor = pos->second;
			++pos;
			tmp_pos = m_map.erase(begin_pos, next_pos);

			IpRange tmpRange(low, high);
				
			m_map.insert(tmp_pos, std::make_pair(tmpRange, tmpColor) );
		}

		pos = next_pos++;
	}
}



bool
IpMap::InsertToMap(IpAddr lowAddr, IpAddr highAddr, const Handle& color, Container& spanMap, iterator& pos)
{
	if(lowAddr <= highAddr)
	{
		IpRange tmpRange(lowAddr, highAddr);
		Handle tmpColor(color);
		
		pos = spanMap.insert( pos, std::make_pair(tmpRange, tmpColor) );
		return true;
	}
	else
		return false;
}


void 
IpMap::LeftSkew(Pair& tmpPair, iterator& pos, const IpRange& range)
{   
	tmpPair = *pos;
	pos = m_map.erase(pos);

	/* This boolean varible test if the insertion succeed or not.
	 * If the lower bound of the range larger than the upper bound, 
	 * InsertToMap method return false.
	 */
	bool insert = false;  
	insert = InsertToMap((tmpPair.first).GetLowerBound(), range.GetLowerBound() - 1, tmpPair.second, m_map, pos);
	
	if(insert)
		++pos;
}


/* --------------------------------------------------------------------------- */
}  // end of namespace Cn.
/* --------------------------------------------------------------------------- */
