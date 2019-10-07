// SPDX-License-Identifier: Apache-2.0
// Copyright 2001 Network Geographics
#pragma once
/* ------------------------------------------------------------------------ */
# if !defined(ISMG_CN_IP_MAP_HEADER)
# define ISMG_CN_IP_MAP_HEADER
/* ------------------------------------------------------------------------ */
# include <cn-ip-base.h>
# include <stl/map>
# include <assert.h>
# include <core/cn-sharedhandle.hpp>
/* ------------------------------------------------------------------------- */
namespace Cn
{
/* ------------------------------------------------------------------------ */
# if defined(ISMG_CN_IP_INTERNAL)
#   define EXPORT _declspec(dllexport)
# else
#   define EXPORT _declspec(dllimport)
# endif
/* ------------------------------------------------------------------------- */
# pragma warning(push)
# pragma warning(disable:4251)	/* disable non dll-interface for IpMap class */
# pragma warning(disable:4275)	/* disable non dll-interface for IpMap class */
# pragma warning(disable:4786)	/* disable identifier too long */
/* ------------------------------------------------------------------------- */

/* In fact, class Color is a client defined class.*/
class EXPORT Color : public SharedHandleCounter
{
protected: 
	int m_color;

public:
	typedef Cn::SharedHandle<Color> Handle;

	Color() : m_color(0) {};	
	Color(int src) : m_color(src) {};

	virtual int GetColor()
	{ return m_color; }

/* "+" and "-" operation is used for Blend and Unblend functions respectively. */
	friend Handle operator+ (const Handle& handle1, const Handle& handle2);
	friend Handle operator- (const Handle& handle1, const Handle& handle2);

	virtual Handle operator += (const Handle& color)
	{ 
		m_color += color->m_color; 
		return Handle(this); 
	}

	virtual Handle operator -= (const Handle& color)
	{ 
		m_color -= color->m_color;
		return Handle(this);
	}


	friend bool operator== (const Color& x, const Color& y);
	friend bool operator < (const Color& x, const Color& y);

	friend class IpMap;
};

inline bool operator== (const Color& x, const Color& y)
{ return x.m_color == y.m_color; }

inline bool operator < (const Color& x, const Color& y)
{ return x.m_color < y.m_color; }

inline Color::Handle operator+ (const Color::Handle& handle1, const Color::Handle& handle2)
{
	Color::Handle zret = new Color(handle1->m_color);
	(* zret) += handle2;
	return zret;
}

inline Color::Handle operator- (const Color::Handle& handle1, const Color::Handle& handle2)
{
	Color::Handle zret = new Color(handle1->m_color);
	(* zret) -= handle2;
	return zret;
}


/* Colorhe key of IpMap is IpRange, and the element of the IpMap is template class Color. */
class EXPORT IpMap
{
protected:
	typedef Color::Handle Handle;
	typedef std::map<IpRange, Handle> Container;
	typedef std::pair<IpRange, Handle> Pair;
	
	Container m_map;

private:
	typedef Container::iterator iterator;


public:
	IpMap() { }

	typedef void (IpMap::*left_func)(const IpRange& range, const Handle& color, iterator& iter, std::pair<IpRange, Handle>& tmpPair);
	typedef void (IpMap::*midd_func)(const IpRange& range, const Handle& color, iterator& iter, std::pair<IpRange, Handle>& tmpPair, bool& isLeftSpan);
	typedef void (IpMap::*right_func)(const IpRange& range, const Handle& color, iterator& iter, std::pair<IpRange, Handle>& tmpPair, bool& isLeftSpan);

	
	/* Paint a certain range with color, no matter what the original color is. */
	void Paint(const IpRange& range, const Handle& color)
	{ 
		//MapOperator mapopt(Pt_left, Pt_middle, Pt_right);
		//Do(range, color, mapopt);

		iterator pos = m_map.lower_bound(range);
		bool isLeftSpan = true;
		Pair mapUnit;

		left_func l = Pt_left;
		midd_func m = Pt_middle;
		right_func r= Pt_right;

		(this->* l)(range, color, pos, mapUnit); 
		(this->* m)(range, color, pos, mapUnit, isLeftSpan); 
		(this->* r)(range, color, pos, mapUnit, isLeftSpan); 

		Coalesce();
	}

	
	/* Paint ranges of spans in the Container of src with corresponding colors */
	void Paint(IpMap src);	
	
	/* Paint ranges of spans in the Container of src with the given color */
	void Paint(IpMap src, const Handle& color);


	/* In the specified range, if there exists an IpRange 
	 * (or part of IpRange lies in the specified range) 
	 * which has the given color, the IpRange or the part 
	 * of the IpRange within the specified range will be deleted.
	 */
	void Unpaint(const IpRange& range, const Handle& color)
	{
		//MapOperator mapopt(Pt_left, Upt_middle, Upt_right);
		//Do(range, color, mapopt);
		iterator pos = m_map.lower_bound(range);
		bool isLeftSpan = true;
		Pair mapUnit;

		left_func l = Upt_left;
		midd_func m = Upt_middle;
		right_func r= Upt_right;

		(this->* l)(range, color, pos, mapUnit); 
		(this->* m)(range, color, pos, mapUnit, isLeftSpan); 
		(this->* r)(range, color, pos, mapUnit, isLeftSpan); 
	}
	


	/* Colorry all ranges of spans in the Container of src and corresponding colors as specified range and color, 
	 * then repeat the procedure of Unpaint(IpRange range, Color color) */
	void Unpaint(IpMap src);
	

	/* Colorry all ranges of span in the Container of src and the specified color as specified range and color, 
	 * then repeat the procedure of Unpaint(IpRange range, Color color)*/
	void Unpaint(IpMap src, const Handle& color);


	/* Make the color zero for parts of ranges which are inside the specified range*/
	void UnColor(const IpRange& range)
	{ 
		//MapOperator mapopt(Pt_left, Pt_middle, Upt_right);
		Handle color;
		//Do(range, color, mapopt);
		iterator pos = m_map.lower_bound(range);
		bool isLeftSpan = true;
		Pair mapUnit;

		left_func l = Pt_left;
		midd_func m = Pt_middle;
		right_func r= Upt_right;

		(this->* l)(range, color, pos, mapUnit); 
		(this->* m)(range, color, pos, mapUnit, isLeftSpan); 
		(this->* r)(range, color, pos, mapUnit, isLeftSpan); 
	}
	
	
	/* Use each range of a span in the Container of src as specified range, 
	 * and repeat the procedure of UnColor(IpRange range) */
	void UnColor(IpMap src);
	
	
	/* add the color to the original colors in the given range.*/
	void Blend(const IpRange& range, const Handle& color)
	{ 
		//MapOperator mapopt(Bd_left, Bd_middle, Bd_right);
		//Do(range, color, mapopt);

		iterator pos = m_map.lower_bound(range);
		bool isLeftSpan = true;
		Pair mapUnit;

		left_func l = Bd_left;
		midd_func m = Bd_middle;
		right_func r= Bd_right;

		(this->* l)(range, color, pos, mapUnit); 
		(this->* m)(range, color, pos, mapUnit, isLeftSpan); 
		(this->* r)(range, color, pos, mapUnit, isLeftSpan); 

		Coalesce();
	}
	
	
	/* use each span which contains a pair of range and color in the Container of src 
	 * as the argument for Blend(IpRange range, Color color) */
	void Blend(IpMap src);
	
	
	/* use a range of each span in the Container of src and the specified color, 
	 * repeat the prodedure of Blend(IpRange range, Color color) */
	void Blend(IpMap src, const Handle& color);


	/* subtract the given color from the original colors in the given range.*/
	void Unblend(const IpRange& range, const Handle& color)
	{ 
		//MapOperator mapopt(Ubd_left, Ubd_middle, Ubd_right);
		//Do(range, color, mapopt);

		iterator pos = m_map.lower_bound(range);
		bool isLeftSpan = true;
		Pair mapUnit;

		left_func l = Ubd_left;
		midd_func m = Ubd_middle;
		right_func r= Ubd_right;

		(this->* l)(range, color, pos, mapUnit); 
		(this->* m)(range, color, pos, mapUnit, isLeftSpan); 
		(this->* r)(range, color, pos, mapUnit, isLeftSpan); 

		Coalesce();
	}
	
	/* use each span which contains a pair of range and color in the Container of src 
	 * as the argument for Unblend(IpRange range, Color color) */
	void Unblend(IpMap src);
	
	/* use a range of each span in the Container of src and the specified color, 
	 * repeat the prodedure of Unblend(IpRange range, Color color) */
	void Unblend(IpMap src, const Handle& color);

	iterator begin() { return m_map.begin(); }
	iterator end()	 { return m_map.end(); }


private:
	/* If two ranges are adjacent and have the same color, combine them as a single range with the color */
	void Coalesce();


	bool InsertToMap(IpAddr lowAddr, IpAddr highAddr, const Handle& color, Container& spanMap, iterator& pos);


	/* left part of paint/unpaint/uncolor function */
	void Pt_left(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair);


	/* middle part of paint/uncolor function */
	void Pt_middle(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair, bool& isLeftSpan);


	/* right part of paint function */
	void Pt_right(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair, bool& isLeftSpan);
	

	/*left part of unpaint function */
	void Upt_left(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair);


	/* middle part of unpaint function */
	void Upt_middle(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair, bool& isLeftSpan);


	/* right part of unpaint/uncolor function */
	void Upt_right(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair, bool& isLeftSpan);

	
	/* left part of blend function */
	void Bd_left(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair);


	/* middle part of blend function */
	void Bd_middle(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair, bool& isLeftSpan);


	/* right part of blend function */
	void Bd_right(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair, bool& isLeftSpan);
	
	
	/* the left part of unblend function */
	void Ubd_left(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair);


	/* middle part of blend function */
	void Ubd_middle(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair, bool& isLeftSpan);


	/* right part of blend function */
	void Ubd_right(const IpRange& range, const Handle& color, iterator& pos, std::pair<IpRange, Handle>& tmpPair, bool& isLeftSpan);
	

	void LeftSkew(std::pair<IpRange, Handle>& tmpPair, iterator& pos, const IpRange& range);

};

/* --------------------------------------------------------------------------- */
}  // end of namespace Cn.
/* --------------------------------------------------------------------------- */
# pragma warning(pop)
# undef EXPORT
# endif /* inclusion protection */
