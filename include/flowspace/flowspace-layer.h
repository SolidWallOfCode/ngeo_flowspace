/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

# pragma once

# include <map>

# include <iostream>
# include <string>
# include <vector>
# include <fstream>
# include <sstream>
# include <boost/tuple/tuple.hpp>
# include <local/boost_intrusive_ptr.hpp>
# include <boost/bind.hpp>
# include <boost/mpl/if.hpp>
# include <boost/mpl/eval_if.hpp>
# include <boost/mpl/apply.hpp>
# include <boost/mpl/identity.hpp>
# include <ngeo/tuple_ostream_operator.hpp>

# include <flowspace/flowspace-tuple.h>
# include <ngeo/interval.hpp>
# include <flowspace/flowspace-node.h>

#   if NG_STATIC
#       define API
#       define LOCAL
#   else
#       if defined(_MSC_VER)
#           if defined(NETWORK_GEOGRAPHICS_CORE_API)
#               define API _declspec(dllexport)
#           else
#               define API _declspec(dllimport)
#           endif
#       define LOCAL
#       else
#           define API __attribute__ ((visibility("default")))
#           define LOCAL __attribute__ ((visibility("hidden")))
#       endif
#   endif

/** @file
    Top level flowspace header file.
 */

namespace ngeo { namespace flowspace {

namespace mpl = boost::mpl;

//! Implementation namespace - not for client use
namespace imp
{
    // Create a maximal region. This presumes the boost::tuple is a flowspace region type.

    // Bottom case, create a maximal interval.
    template < typename H > void maximize_region (
        boost::tuples::cons<H, boost::tuples::null_type> & r
        )
    {
        r.head = H::all();
    }

    // Upper case, set our interval to max and ripple down.
    template < typename H, typename T > void maximize_region (
        boost::tuples::cons<H,T> & r
        )
    {
        r.head = H::all();
        maximize_region(r.tail);
    }

    // Check a region for validity.
    template < typename H > bool is_valid (
        boost::tuples::cons<H, boost::tuples::null_type> const& r
        )
    {
        return !r.head.is_empty();
    }

    template <typename H, typename T> bool is_valid (
        boost::tuples::cons<H,T> const& r
        )
    {
        return !r.head.is_empty() && is_valid(r.tail);
    }

    //! Create the "has_flowspace_tag" metafunction.
    BOOST_MPL_HAS_XXX_TRAIT_DEF(flowspace_tag);
    //! Create the "has_metric_type" metafunction.
    BOOST_MPL_HAS_XXX_TRAIT_DEF(metric_type);

    /** Metafunctions for delayed access to members.
        This is necessary to handle type dependent access to members. Any
        "immediate" symbol look is performed by the compiler, even if it's in a
        template that is not instantiated. So, if we want member M of type T and
        we have T::M, then T must have a member M in all cases, even if the T::M
        is inside an MPL conditional. The workaround for this is to never have
        the explicit T::M, but instead have a metafunction that given a T gets
        the M and then conditionally eval that metafunction. Note that even
        with this, mpl::eval_if is still necessary as mpl::if will trigger
        the "no such member" error.
     */
    //@{
    //! Metafunction class to extract the @c mapped_type member of a type.
    struct member_mapped_type_mf { template < typename T > struct apply { typedef typename T::mapped_type type; }; };
    //! Metafunction class to extract the @c region member of a type.
    struct member_region_mf { template < typename T > struct apply { typedef typename T::region type; }; };
    //! Metafunction class to extract the @c cursor member of a type.
    struct member_cursor_mf { template < typename T > struct apply { typedef typename T::cursor type; }; };
    //! Metafunction class to extract the @c metric_type member of a type.
    struct member_metric_type_mf { template < typename T > struct apply { typedef typename T::metric_type type; }; };
    //@}
    //! @endcond


} // namespace imp

/** Flowspace, a template class used to build n-dimensional interval sets.
    The use of this template creates a single dimension of intervals.

    The template parameters are the @a METRIC and the @a PAYLOAD.
    The @a METRIC is the type that for the elements used for the intervals.
    The @a PAYLOAD is the data associated with each interval. If the @a PAYLOAD
    is an flowspace type, then that flowspace is used as a nested dimension
    of intervals. This allows a flowspace of an arbitrary number of dimensions
    to be constructed.
 */
template <typename METRIC, typename PAYLOAD >
class layer
{
public:
    typedef layer self; //!< Self reference type.

    // forward declarations
    class iterator;

    struct flowspace_tag; //!< Mark this as a flowspace

    /** Compile time constant that indicates whether this instantiation is
        being used as the core layer or an upper layer.
     */
    static bool const IS_UPPER = imp::has_flowspace_tag<PAYLOAD>::value;

    /** Metric and interval types. We compute these because we want to support using an
        existing interval as the METRIC template argument.

        If METRIC isn't an interval, then we want to define the metric type as METRIC and
        the interval type as an interval of that type. Otherwise, we want to use the METRIC
        as the interval type and define our metric type as the internal metric type of the
        interval type METRIC (i.e., METRIC::metric_type).
     */
    //@{
    //! The metric type, which is the data type used as the dimensional type this layer.
    typedef typename mpl::eval_if<imp::has_metric_type<METRIC>
        , mpl::apply<imp::member_metric_type_mf, METRIC>
        , mpl::identity<METRIC>
    >::type metric_type;
    //! The interval type, which is the data type used for represent an interval in this layer.
    typedef typename mpl::if_<imp::has_metric_type<METRIC>, METRIC, interval<METRIC> >::type interval_type;
    //@}

    //! @cond IMPLEMENTATION
    /** This has to be split out to delay evaluation, because the compiler will
        evaluate all type references in a single @c typedef. That means that
        no matter how deeply nested, we can't try to access @c region in
        @c PAYLOAD in the same typedef as the conditional. If we just wanted
        the @c region type directly, we could get by with the member_xxx_mf
        metafunction. Unfortunately, since we need to operate on @c region
        we have to have another insulating metafunction to do that.

        The @c bind creates a metafunction of a single argument @a T that creates a
        @c tuple of @c interval and the components of the @c tuple @c T::region.
     */
    typedef typename mpl::bind<boost::tuples::add_type_mf, interval_type, mpl::bind<imp::member_region_mf, mpl::_1> > calc_region_mf;
    //! @endcond

    /** The type that describes a region for this flowspace.
	It is an N-tuple, the element pairs of metrics, one
	pair for each flowspace layer.
    */
    typedef typename mpl::eval_if_c<IS_UPPER,
        typename mpl::apply<calc_region_mf, PAYLOAD>,   // outer case, prepend interval to inner region
        typename mpl::identity<boost::tuple<interval_type> > // base case, a tuple of just our interval
    >::type region;

    //! @name STL compliance
    //@{
    /** The effective key type for the flowspace.
        @note STL compliance
     */
    typedef region key_type;

    /** The type used to hold the data associated with a key.
        @internal If the payload isn't a flowspace, we just use it directly
        as our payload. If it's a flowspace, then to get the nesting correct
        we want to use the mapped_type of that nested flowspace (rather than
        the flowspace itself) as our mapped_type.
        @note STL compliance
    */
    typedef typename mpl::eval_if_c<IS_UPPER,
        typename mpl::apply<imp::member_mapped_type_mf, PAYLOAD>,
        typename mpl::identity<PAYLOAD>
    >::type mapped_type;

    /** The effective type of values stored in this container.
        @note STL compliance
     */
    typedef std::pair<key_type const, mapped_type> value_type;

    /** Copy of a value.
        This is used in places where a copy of a value in the flowspace is needed.
        Direct references to flowspace values have constant @c key_type elements
        to make sure that the key (i.e., the region) isn't modified. A copy
        does not have this restriction.
     */
    typedef std::pair<key_type, mapped_type> value_copy;
    /** Type for returning a set of copies of values.
        @note This contains @b copies of the values. No modifications will propagate back to the flowspace.
     */
    typedef std::vector<value_copy> value_group;
    //@}

protected:
    /** The underlying tuple implementation type.
        @note Used for chaining between layers.
     */
    typedef typename region::inherited interval_cons;

    /** Forwarding mapped typed.
        This is used in @c value_type_ref.
     */
    typedef mapped_type& mapped_type_ref;

    /** Used by cursor classes to update the iterator mapped_type data.
     */
    typedef mapped_type* payload_ptr;

    /** Forwarding iteration value type.
        This is used as something that functions as a value type but keeps the
        actual client payload elsewhere. This is required so that the iterator can
        synthesize the interval data while providing the client with write access
        to the stored payloads. It mimics @c value_type to simplify client code,
        the difference being that it access the payload stored in the tree elements
        rather than a copy.
     */
    struct value_type_ref
    {
        key_type const first; //!< First element, the region.
        mapped_type_ref second; //!< Second element, the payload.

        //! Construct from just a payload reference (region is default constructed)
        value_type_ref(mapped_type_ref mtf) : second(mtf) { }
        //! Standard constructor.
        value_type_ref
            ( key_type const& one //!< Key value
            , mapped_type_ref two //!< Payload reference
            ) : first(one), second(two)
        {
        }

        //! User conversion to for ease of client use.
        operator value_type () const
        {
            value_type vt(first, second);
            return vt;
        }
        //! User conversion for ease of client use.
        operator value_copy () const
        {
            value_copy vt(first, second);
            return vt;
        }
    };

    /** The nodes used for this instantiation of a layer.
     */
    struct node : public imp::node_base
    {
        typedef node self; //!< Self reference type.
        typedef imp::node_base super; //!< Super class reference type.
        typedef boost::intrusive_ptr<self> handle; //!< Handle to an instance of this class.

        /** The inner set of right end points.
            The actual intervals are not stored. Instead, all intervals
            with the same left endpoint are stored in the same outer
            node. In this node are stored all of the right endpoints
            along with their PAYLOAD. This is the inner_set.
        */
        typedef typename mpl::if_c<IS_UPPER,
                std::map<metric_type, PAYLOAD>,
                std::multimap<metric_type, PAYLOAD>
            >::type inner_set;

        /** Upper layer insert.
            Do the insert in our local, inner tree
            and pass on the insert to the lower layer at that location
            in the tree.
         */
        struct upper_inner_tree_inserter {
            //! Functor operator.
            static void func (value_type const& v, inner_set & c) {
                metric_type i_max(v.first.head.max());
                typename inner_set::iterator spot(c.find(i_max));
                    if (spot == c.end()) {
                        // force inner element for interval maxima
                        spot = c.insert(typename inner_set::value_type(i_max, PAYLOAD())).first;
                    }
                    // ripple insert down
                spot->second.insert(typename PAYLOAD::value_type(v.first.tail, v.second));
            }
        };
    	
        //! @cond IMPLEMENTATION
        /** Bottom layer insert.
        */
        struct bottom_inner_tree_inserter {
            static void func (value_type const& v, inner_set& c) {
                c.insert(typename inner_set::value_type(v.first.head.max(), v.second));
            }
        };
    	
        typedef mpl::if_c< IS_UPPER
                         , upper_inner_tree_inserter
                         , bottom_inner_tree_inserter
                         > inner_inserter;
    	
        metric_type m_metric;    //!< The minima for all intervals in this node.
        inner_set m_maxima;  //!< PAYLOAD keyed by interval maximums
        interval_type m_sti;     //!< Interval hull of the tree rooted here.

        //! Get the left child node, if present.
        self* get_left() { return static_cast<self*>(m_left.get()); }
        //! Get the right child node, if present.
        self* get_right() { return static_cast<self*>(m_right.get()); }
        //! Get the parent node.
        self* get_parent() { return static_cast<self*>(m_parent); }
        //! Get the node next for in order traversal.
        self* get_next() { return static_cast<self*>(m_next); }
        
        self *get_leftmost_descendant() { return static_cast<self*>(node_base::get_leftmost_descendant().get()); }

        /*! Construct a node from a value */
        node(value_type const& v)
            : m_metric(v.first.head.min())
            , m_sti(v.first.head)
        {
            this->inner_insert(v);
        }

        //! Add a region/payload to the node
        void insert (
            value_type const& v //!< The region
	    ) {
            this->inner_insert(v);
        }

        //! Erase an interval.
        void erase(typename inner_set::iterator const& spot) {
            m_maxima.erase(spot);
        }

        //! Test if the node has no intervals associated with it.
        bool is_empty() const {
            return m_maxima.empty();
        }

        /** Get the minimum value for all intervals in this node. */
        metric_type const& get_metric() { return m_metric; }

        /** Compare the left end point in this node to a metric value.
            @return
            - @c NONE if the metrics are the same
            - @c RIGHT if @a m is greater than the local metric
            - @c LEFT if @a m is less than the local metric
        */
        direction compare_metric(
	        metric_type const& m //!< the metric to compare
        ) {
            return m > m_metric ? RIGHT
                : m < m_metric ? LEFT
                : NONE
                ;
        }

 
        //! @endcond

        /** Insert a region in to the inner set.
            If this isn't an outer dimension then we just add the PAYLOAD
            to the inner set, keyed by the right endpoint.
            Otherwise we need to insert the PAYLOAD in to the sub-flowspace
            in the inner set, creating it if necessary.
         */
        void inner_insert (
            value_type const& v	//!< The data to insert
        ) {
            inner_inserter::type::func(v, m_maxima);
            m_sti = interval_type(std::min(m_sti.min(), v.first.head.min()), std::max(m_sti.max(), v.first.head.max()));
        }

        /** Co-variant method */
        handle rebalance_after_insert() {
            return boost::dynamic_pointer_cast<self>(super::rebalance_after_insert());
        }

        /** Co-variant method */
        handle insert_child(handle const& n, direction d) {
            return boost::dynamic_pointer_cast<self>(super::insert_child(n,d));
        }

        /** Co-variant method */
        handle remove() {
            return boost::dynamic_pointer_cast<self>(super::remove());
        }

        //! Access to parent method for clients of this class.
        void ripple_structure_fixup() { this->super::ripple_structure_fixup(); }

        //! Get the convex hull of all intervals in this node.
        interval_type local_interval() const {
            assert(m_maxima.rbegin() != m_maxima.rend());
            // Because all the intervals start at the same place, the
            // hull is just the minima and the last maxima.
            interval_type zret(m_metric, m_maxima.rbegin()->first);
            return zret;
        }

        //! Get the convex hull of all intervals in this node and its descendants
        interval_type const& tree_interval() const {
            return m_sti;
        }

        /** Test the intersection of an interval and the convex hull of all
            intervals in this node.
         */
        bool intersects_local(
            interval_type const& intv    //!< interval to check
        ) {
            return intv ^ this->local_interval();
        }

        /** Test the intersection of an interval and the convex hull of all
            intervals in this node and its descendants.
         */
        bool intersects_tree(
            interval_type const& intv    //!< interval to check
        ) {
            return intv ^ m_sti;
        }

    	/** Fixup non-base structure after a tree operation.
            When a node is inserted in the tree, other nodes can be @em rotated,
            that is locations in the tree and neighbors can change.
            This virtual method invoked by the base class @b after the
            all rotations for this node and all of its (new) children.
            It is possible that additional rotations will occur after this
            method is called, but if so those rotations will not alter
            the relationship of this node to any of its descendants.
         */
    	virtual void structure_fixup() {
            self* lc(this->get_left()); // left child
            self* rc(this->get_right()); // right child

            /*  Compute the new subtree interval. The main issue is to avoid
                bad dereferences for missing children. We start with the
                interval for just this node and then see if we need to
                extend it based on a child subtree interval.
             */
            m_sti = this->local_interval();
            if (lc) m_sti |= lc->tree_interval();
            if (rc) m_sti |= rc->tree_interval();
        }

        /** Locate a node or its prospective parent.
            The functor @a f must be a unary functor that takes a
            @c node::handle argument and compares this node to its internal
            search criteria. @a f should return one of the values
            - @c NONE if the argument node matches the search criteria
            - @c LEFT if the argument node is greater than the search criteria
            - @c RIGHT if the argument node is less than the search criteria

            @return A pair of a node handle and a direction. If the search was
            successful, then the direction will be @c NONE. Otherwise, the node
            returned will be the last node visited (always a leaf) and the direction
            will indicate which child would have been the next to be searched.
            There will never be a child in that location (e.g. a node with a left
            child will never be returned with the direction value @c LEFT).

            @internal I originally had this in @c node_base but there were
            numerous co-variance issues and it's templated anyway, so it
            ended up being much easier to have it here without much
            additional overhead.

        */
        template < typename F > std::pair<handle, direction> search (
            F const& f //!< Search functor
        ) {
            handle n(this);
            direction d = NONE;

            while (n) {
                super::handle child;
                // Compute the child node we should traverse to
                d = f(n);
                if (RIGHT == d) child = n->m_right;
                else if (LEFT == d) child = n->m_left;

                if (child) n = boost::dynamic_pointer_cast<self>(child);
                else break; // no nodes left to search
            }
            return std::make_pair(n, d);
        }

        //@{ Cursor support methods
        /** Get an iterator for the first element not less than @a m.
         */
        typename inner_set::iterator begin(
            metric_type const& m //!< interval minima
        ) {
            return m_maxima.lower_bound(m);
        }

        /** Get an iterator for the first element.
         */
        typename inner_set::iterator begin() { return m_maxima.begin(); }

        /** Get an iterator for one past the last element for the metric @a m.
         */
        typename inner_set::iterator end(
            metric_type const& m //!< interval minima
        ) {
            return m_maxima.upper_bound(m);
        }

        /** Get an iterator past the last element.
         */
        typename inner_set::iterator end() {
            return m_maxima.end();
        }

        //@}

        struct outer_print_payload {
            static void print(std::ostream &s, PAYLOAD &pay, int indent) {
                pay.print(s, indent);
            }
        };

        struct inner_print_payload {
            static void print(std::ostream &s, PAYLOAD pay, int indent) {
                int i;
                for (i = 0; i < indent; i++)
                    s << '*';
                s << pay << '\n';
            }
        };
        typedef mpl::if_c<IS_UPPER, outer_print_payload, inner_print_payload> print_payload;
        
        std::ostream &print(std::ostream &s, int indent, int height, int b_height) {
            if (this->get_color() == BLACK) ++b_height;
            ++height;

            int i = 0;
            for (i = 0; i < indent; i++)
                s << '-';
            s   << "Key=" << this->get_metric() << ' ' << (this->get_color() == node::BLACK ? "BLACK" : "RED  ")
                << " H=" << height << '/' << b_height;
            s   << " L=";
            if (m_left) s << this->get_left()->get_metric();
            else s << '*';
            s   << " R=";
            if (m_right) s << this->get_right()->get_metric();
            else s << '*';
            s << '\n';
//            s << "m_sti=" << m_sti << " metric=" << this->get_metric() << " RE's\n";
//            for (i = 0; i < indent; i++)
//                s << "*";
//            typename inner_set::iterator iter = this->begin();
//            for (; iter != this->end(); ++iter) {
//                s << iter->first << " ";
//            }
//            s << "\n";
//            iter = this->begin();
//            for (; iter != this->end(); ++iter) {
//                print_payload::type::print(s, iter->second, indent+1);
//            }
            if (get_left()) {
//                for (i = 0; i < indent; i++)
//                    s << "*";
//                s << "Left child of metric " << get_metric() << "\n";
                get_left()->print(s, indent+2, height, b_height);
            }
            if (get_right()) {
//                for (i = 0; i < indent; i++)
//                    s << "*";
//                s << "Right child of metric " << get_metric() << "\n";
                get_right()->print(s, indent+2, height, b_height);
            }
            return s;
        }
        
        /** Ensure that the local information associated with each node is correct globally
            This should only be called on debug builds as it breaks any efficiencies
            we have gained from our tree structure.
         */
        bool structure_validate() {
            bool valid = true;

            metric_type max_value = get_metric();
            metric_type min_value = get_metric();			

            typename inner_set::reverse_iterator riter = m_maxima.rbegin();
            if (riter != m_maxima.rend()) max_value = std::max(max_value, riter->first);

            handle child = this->get_left();
            if (child) {
                min_value = std::min(min_value, child->m_sti.min());
                max_value = std::max(max_value, child->m_sti.max());
            }
            child = this->get_right();
            if (child) {
                max_value = std::max(max_value, child->m_sti.max());
            }		
            if (m_sti.min() != min_value || m_sti.max() != max_value) {
                std::cout << "Mismatch between cached value " << m_sti << " and real min max " << min_value << " " << max_value << " metric=" << get_metric() << "\n";
                valid = false;
            }
            return valid;
        }
    }; // struct node

    struct cursor; // must forward declare to get friendship right with gcc.

    /** Interval iteration.
        This iterates over intervals in the current layer.
     */
    class local_iterator : public std::iterator<std::forward_iterator_tag, interval_type>
    {
        typedef local_iterator self; //!< Self reference type

        //! Default constructor
        local_iterator()
        {
        }

        //! Dereference
        interval_type const& operator * () const { return m_value; }
        //! Member access
        interval_type const* operator -> () const { return &m_value; }

        //! Equality
        bool operator == ( self const& that )
        {
            return m_node == that.m_node && (!m_node || m_spot == that.m_spot);
        }

        //! Inequality
        bool operator != ( self const& that )
        {
            return !(*this == that);
        }

    protected:
        typename node::handle m_node; //!< Current node
        typename node::inner_set::iterator m_spot; //!< Inner tree location
        interval_type m_value; //!< Dereference data

        /** Construct for a specific location.
         */
        local_iterator(
            typename node::handle const& n, //!< Iteration node
            typename node::inner_set::iterator spot //!< Inner tree location
            )
            : m_node(n), m_spot(spot), m_value(n->get_metric(), spot->first)
        {
        }

        friend class layer;
        friend struct cursor;
    };

    /** Find an interval in this layer.
        This matches only the exact interval.
     */
    local_iterator find(
        interval_type const& intv //!< Interval to find.
        )
    {
        local_iterator spot;
        if (m_root) {
            // Search for the node with the identical minimum as the interval.
            typename node::handle n;
            typename node::direction d;
            boost::tie(n,d) = m_root->search(boost::bind(&node::compare_metric, _1, intv.min()));
            if (node::NONE == d) { // exact match
                typename node::inner_set::iterator ii(n->begin(intv.max()));
                // Verify that we have an exact match for the maximum.
                if (ii != n->end() && ii->first == intv.max()) {
                    spot = local_iterator(n, ii);
                }
            }
        }
        return spot;
    }
    
    /** Find the first node in the tree that overlaps an interval.
        @return The first node for the interval or @c nil if there is
        no node that intersects @a intv.
     */
    typename node::handle find_intersecting(
        interval_type const& intv    //!< interval to match
        )
    {
        typename node::handle candidate; // The best, if any, we've seen so far
        typename node::handle n(m_root); // current working node
        while (n) {
            if (n->intersects_local(intv)) {
                /*  Best choice so far. Any better choice can only be in the left subtree.
                    This effectively serves as limit to any backtracking because if the backtrack
                    gets here, then this candidate is the best for the entire tree.
                 */
                candidate = n;
                n = n->get_left();
            } else if (n->intersects_tree(intv)) {
                // This node isn't viable, but one of its children might be.
                // Try left if possible, we'll check the right on the backtrack if
                // that doesn't work. Otherwise, go right.
                typename node::handle lc(n->get_left());
                n = lc ? lc : n->get_right();
                // If n has a tree intersection but not a node intersection, there must be
                // a child to contain the range in the tree that's not in this node.
                assert(n);
            } else {
                /*  This subtree contains no candidates, time to backtrack.
                    We ascend the tree until we
                    - encounter the candidate, which is now known to be the result
                    - find a right subtree we haven't explored
                    - fall off the top of the tree, in which case no node overlaps the query region
                 */
                while (n) {
                    typename node::handle child(n); // remember where we came from
                    n = n->get_parent();
                    if (n == candidate) // we're done if we backtrack to the candidate
                        return n;
                    typename node::handle rc(n->get_right());
                    if (rc && child != rc) { // there's a right tree we haven't explored
                        n = rc; // go in to the right subtree
                        break; // drop out of backtrack loop
                    }
                    // else we've moved to the parent, so try that on the next loop.
                }
            }
        }
        return candidate;
    }

    /*  We have paired classes for iteration. There is the standard
        @c iterator class, which presents an STL compliant iterator
        API. Internally, because of the nesting that goes on with the
        flowspace, we don't want to be doing N^2 copies of region data.
        Because Boost.Tuple uses cons style type construction, we can
        pass down references to nested iterators without copying. But,
        that leaves the question of where the master storage for those
        references lives, not to mention that using references violates
        STL compliance. We solve this by having @c iterator store
        the region information and @c cursor to do the actual work.
     */

    /** Cursor classes.
        The inheritance pattern is a diamond, with this as the base, then
        two alternate classes for the bottom and upper layer cases, with a final
        unified, concrete class. This base class lets us factor out things
        that are common between the alternate cursor classes and define the
        API for the concrete class.
     */
    //@{
    /** Base class for all cursors.
        This holds common elements that the variant bases need.
     */
    struct cursor_base {
        typename node::handle m_node;    //!< the current outer node
        typename node::inner_set::iterator m_inner;    //!< the current inner set node

        /** Default constructor.
            Constructs an invalid cursor.
         */
        cursor_base() { }

        /** Standard constructor.
            Sub classes will handle setting the cursor to a valid region.
         */
        cursor_base(
            typename node::handle const& n      //!< initial node for the iteration
            )
            : m_node(n)
        {
            if (n) m_inner = n->end();
        }

        //! Return if this cursor is valid.
        bool is_valid() const
        {
            /*  Validity is tracked by whether the inner tree iterator is valid.
                That in turn is true if the iterator isn't at the end of the inner tree.
             */
            return m_node && m_inner != m_node->end();
        }

        //! Mark cursor as invalid
        void invalidate()
        {
            m_node = 0;
            m_inner = typename node::inner_set::iterator();
        }

        /** Move forward to the next interval that intersects the query region.
            If an interval is found, @c m_node and @c m_inner are set appropriately.
            Otherwise, @c m_node is set to @c NIL.
            @note This always changes the value of @c m_node.
            @return @c true if an interval was found, @c false otherwise.
        */
        bool scan(
            interval_cons const& region //!< [in] Query region
            )
        {
            // We should only call this method if the current node is valid.
            assert(m_node);
            /*  Scan forward in the outer tree. We have to be careful
                because we may hit non-intersecting nodes with
                valid nodes further down the chain.
             */
            m_node = m_node->get_next();
            // loop until we run out of nodes or find an intersecting one
            while (m_node && !m_node->intersects_local(region.head)) {
                if (m_node->get_metric() > region.head.max()) {
                    // all subsequent nodes have a larger minimum, so if that's
                    // already larger than the maximum in the query region,
                    // cancel out of the scan locally and permanently.
                    m_node = 0;
                } else {
                    // Figure out the next node to check.
                    if (!m_node->intersects_tree(region.head)) {
                        // Nothing in this subtree insects the query. Skip it.
                        // We do that by moving to the right most descendant and
                        // moving on from there.
                        // NB: Because of the in-order traversal, any time we
                        // are checking a node the left subtree will have already
                        // been searched.
                        typename node::handle n;
                        while (n = m_node->get_right()) {
                            m_node = n;
                        }
                        // m_node has been moved to its right most descendant.
                    }
                    //! Current node didn't work, move on.
                    m_node = m_node->get_next();
                }
            }

            // Set the inner iterator to the minima for the first intersecting interval.
            if (m_node) {
                m_inner = m_node->begin(region.head.min());
                assert(m_inner != m_node->end());
            }

            return m_node;
        }

        /** Set the inner cursor.
            Use @c is_valid to check for success.
         */
        void fill_inner_cursor(
            interval_cons const& region, //!< [in] Query region
            interval_cons&,              //!< [ignored]
            payload_ptr&             //!< [ignored]
            )
        {
            m_inner = m_node->begin(region.head.min());
        }

        /** Load client data for this layer.
         */
        void load_client_data(interval_cons& location) {
            // Just load the interval data for the layer. Sub classes will handle everything else.
            location.head = interval_type(m_node->get_metric(), m_inner->first);
        }
    };


    /** Cursor variant, bottom layer case.
        This is the cursor for the bottom (non-nested) layer. It interacts directly with
        the payload.
     */
    struct bottom_cursor_variant : public cursor_base {
        typedef cursor_base super; //!< Super class reference type
        typedef bottom_cursor_variant self; //!< Self reference type

        //! Default constructor - refers to nothing.
        bottom_cursor_variant() : super() { }
        //! Construct cursor to refer to node @a n.
        bottom_cursor_variant(
            typename node::handle const& n  //!< initial node for the iteration
            ) : super(n)
        {
        }

        /** Fill the inner cursor with an exact match only.
         */
        void fill_exact(
            key_type const& r,            //!< [in] Target region
            mapped_type const& p,       //!< [in] Target payload
            interval_cons& location,    //!< [out] Storage for current region
            payload_ptr& data           //!< [out] Storage for current payload
            )
        {
            // At the bottom, it's a multimap so we have to check successive
            // elements until 
            // - we run off the end
            // - we go past the matching interval maxima
            // - we find a matching payload
            for ( ; this->is_valid() && super::m_inner->first == r.head.max() ; ++super::m_inner ) {
                if (super::m_inner->second == p) {
                    this->load_client_data(location, data);
                    return;
                }
            }
            this->invalidate();
        }

        /** Load client data for this layer.
         */
        void load_client_data(
            interval_cons& location,//!<[out] Data for current region
            payload_ptr& data       //!<[out] Payload for current region
            )
        {
            this->super::load_client_data(location); // do standard interval data
            data = &super::m_inner->second; // At the bottom, get the payload
        }

        /** Try to make the cursor valid, moving forward as necessary.
            If a valid location is found, the client data is updated.
            @return @c true if the cursor is now valid, @c false otherwise.
            @note This does not advance the cursor if the current location is valid.
            @note This method is expected to fail in normal use, when the
            cursor reaches the end of the valid interval.
        */
        bool validate_forward(
            interval_cons const& region,    //!< [in] Query region
            interval_cons& location,        //!< [out] Current region
            payload_ptr& data               //!< [out] Payload for current region
            )
        {
            bool valid = false;
            if (this->is_valid() || this->scan(region)) {
                valid = true;
                // make sure we have valid data.
                this->load_client_data(location, data);
            }
            return valid;
        }

         /** Move cursor to next region.
         */
        void next(
            interval_cons const& region,    //!< [in] Query region
            interval_cons& location,        //!< [out] Current region
            payload_ptr& data               //!< [out] Payload for current region
            )
        {
            if (this->is_valid()) {
                ++super::m_inner;
                this->validate_forward(region,location,data);
            }
        }

        /** Delete the element to which the cursor refers.
            @return The new root node after the erase, or NIL if the root node didn't change.
            @internal Ugly style of return, but we don't have good access to the root node
            from here.
         */
        std::pair<typename node::handle, bool> erase () const
        {
            bool valid = false;
            typename node::handle root;
            if (this->is_valid()) {
                super::m_node->erase(super::m_inner);
                if (super::m_node->is_empty()) {
                    root = super::m_node->remove();
                    valid = true;
                } else {
                    super::m_node->ripple_structure_fixup();
                }
            }
            return std::make_pair(root, valid);
        }
        /** Equality operator.
            @internal Have to check everything because MSVC 8.0 verifies iterator containers.
         */
        bool operator == (self const& rhs) const {
            return super::m_node == rhs.m_node && ( !super::m_node || super::m_inner == rhs.m_inner);
        }
    };

    /** Upper case for cursor variant class.
        This is the more interesting case, as it needs to handle cursors for lower
        layers as well as the inner tree. The tricky part is that even if the node
        in this layer is valid for the layer interval, it may not be suitable because
        there is no lower layer node that intersects the query region.
     */
    struct upper_cursor_variant : public cursor_base {
        typedef cursor_base super; //!< Super class reference type
        typedef upper_cursor_variant self; //!< Self reference type

        //! Default constructor
        upper_cursor_variant() : super() { }
        //! Construct to refer to a specific node.
        upper_cursor_variant(
            typename node::handle const& n          //!< initial node for the iteration
            ) : super(n)
        {
        }

        // @cond IMPLEMENTATION
        //! Type of the cursor for the next lower (nested) layer.
      	typedef typename mpl::apply<imp::member_cursor_mf, PAYLOAD>::type lower_cursor_type;
        // @endcond
        
        lower_cursor_type m_lower; //!< cursor for next lower layer

        /** Load the lower cursor with valid data from the current node, if possible.
            The caller must verify that the cursor is valid for this layer.
         */
        void fill_lower_cursor(
            interval_cons const& r,    //!< [in] Query region
            interval_cons& location,        //!< [out] Current region
            payload_ptr& data               //!< [out] Payload for current region
            )
        {
            m_lower = util::make_lower_cursor(super::m_inner->second, r, location, data);
        }

        /** Load the inner and lower cursors, if possible.
            @note Caller must verify success via @c is_valid.
        */
        void fill_inner_cursor(
            interval_cons const& r,     //!< [in] Query region
            interval_cons& location,    //!< [out] Current region
            payload_ptr& data           //!< [out] Payload for current region
            )
        {
            /*  Do the standard fill, which loads up the inner tree iterators
                that cover the max endpoints for the outer node. If there are
                inner nodes that intersect, pass the request on down to the next
                inner layer to get its cursor in to a good state.
             */
            this->super::fill_inner_cursor(r, location, data);
            if (this->super::is_valid())
                this->fill_lower_cursor(r, location, data);
        }

        /** Load the inner and lower cursors, if possible, matching the region exactly.
         */
        void fill_exact(
            key_type const& r,              //!< [in] Target region
            mapped_type const& p,           //!< [in] Target payload
            interval_cons& location,        //!< [out] Storage for current region
            payload_ptr& data               //!< [out] Storage for current payload
            )
        {
            if (this->is_valid()) {
                // This layer is OK, ripple.
                m_lower = util::make_lower_cursor_exact(super::m_inner->second, r, p, location, data);
                // Check just the lower layer to see if the ripple worked.
                if (m_lower.is_valid()) { // yes, load up the layer interval data.
                    this->load_client_data(location);
                } else { // no, invalidate this layer as well.
                    this->invalidate();
                }
            }
        }

        /** Try to make the cursor valid, moving forward as necessary.
            Load client data if successful.
            @return @c true if the cursor is now valid, @c false otherwise.
            @note This method is expected to fail in normal use, when the
            cursor reaches the end of the valid interval.
        */
        bool validate_forward(
            interval_cons const& region,    //!< [in] Query region
            interval_cons& location,        //!< [out] Current region
            payload_ptr& data               //!< [out] Payload for current region
            )
        {
            // Scan while we still have nodes left in this layer.
            while (super::m_node && !m_lower.is_valid()) {
                bool should_do_fill = false;
                // If @a fill gets set to @true in either clause, then it should
                // be the case that @a m_inner is valid but @a m_lower is bad.
                // If @a fill is left @c false, then the first case falls in to the second
                // and the second clears @a m_node, dropping out of the loop.
                if (this->is_valid()) {
                    /*  It's critical to note that for this layer, we will always
                        iterate to the end of the inner tree because the interval
                        represented by a later inner node is a superset of all previous
                        inner nodes.
                    */
                    ++super::m_inner;
                    // Set @a should_do_fill if the increment didn't move past the last interval in this node.
                    should_do_fill = this->is_valid();
                } else {
                    // Exhausted inner tree at this node. Find an interval in another node.
                    // Fill if that works, or drop out of the loop if not
                    // (because @a m_node is set to @c NIL in that case)
                    should_do_fill = this->scan(region);
                }
                // For efficiency, we unify the fill logic here.
                if (should_do_fill)
                    this->fill_lower_cursor(region, location, data);
            }

            // We can do the inexpensive validity check because when the previous loop
            // exits either everything is valid or this layer is not valid.
            if (this->is_valid()) {
                this->load_client_data(location);
                return true;
            }
            return false;
        }

        /** Move cursor to next intersecting region.
            @internal This is an external API method.
         */
        void next(
            interval_cons const& region,    //!< [in] Query region
            interval_cons& location,        //!< [out] Current region
            payload_ptr& data           //!< [out] Payload for current region
            )
        {
            if (this->is_valid()) {
                /*  This layer is valid. Ripple the request down the layers,
                    which do their own validity checks.
                 */
                m_lower.next(region.tail,location.tail,data);
                /*  Having guaranteed that we have advanced at least one region,
                    continue advancing (if necessary) until we have a valid
                    region or there are no more valid regions.
                 */
                this->validate_forward(region,location,data);
            }
        }

        /** Delete the element to which the cursor refers.
            @return A pair of a node and a flag. If the flag is set, the node is the new root node.
            @internal Ugly style of return, but we don't have good access to the root node
            from here.
         */
        std::pair<typename node::handle, bool> erase () const
        {
            bool valid = false;
            typename node::handle root;
            if (this->super::is_valid()) {
                util::erase_lower(super::m_inner->second, m_lower);
                // clean up if necessary
                if (super::m_inner->second.is_empty()) {
                    super::m_node->erase(super::m_inner);
                    if (super::m_node->is_empty()) {
                        root = super::m_node->remove();
                        valid = true;
                    } else {
                        super::m_node->ripple_structure_fixup();
                    }
                }
            }
            return std::make_pair(root, valid);
        }

        /** Equality operator.
            @internal Have to check everything because MSVC 8.0 verifies iterator containers.
         */
        bool operator == (self const& rhs) const {
            return super::m_node == rhs.m_node && ( !super::m_node || ( super::m_inner == rhs.m_inner && m_lower == rhs.m_lower));
        }
    };
    //@}

    /** @cond IMPLEMENTATION
        This needs to be typedef'd outside of the class so that we can define
        cursor::super correctly. We can't use the 'typedef self super;' trick because
        the compiler doesn't seem to recognize "self" as a type inside cursor, even
        if prefixed with 'typename'.
     */
    typedef mpl::if_c< IS_UPPER
		     , upper_cursor_variant
		     , bottom_cursor_variant
		     > cursor_super;
    /** This is the internal implementation class for iteration over the
        flowspace. Clients will find its API difficult and tricky to use.
        Clients should use the STL standard @c iterator class instead.

        @internal @c m_region is the query region for cursor iteration.
        Only elements with regions that intersect the query region show
        up in the iteration. @c m_location is where the cursor actually is,
        i.e. the region of the current element of the flowspace.
        
        Invariants:
        - @c m_region is never modified
        - @c m_location intersects @c m_region @b OR the cursor is == to end()
        - @c m_location is identical to the region in the @c value_type when @c insert was called for the current element.
     */
    struct cursor : public cursor_super::type 
    {
        typedef typename cursor_super::type super; //!< Parent type.
        typedef typename super::super base; //!< Base cursor type.
        typedef cursor self; //! Self reference type.

        //! Default constructor.
        cursor() : super() { }

        /** Standard constructor.
         */
        cursor(
            typename node::handle const& n, //!< initial node for the iteration
            interval_cons const& region,    //!< the iteration region
            interval_cons& location,        //!< where to store current location data
            payload_ptr& data           //!< where to store client data
            )
            : super(n)
        {
            if (super::m_node) {
                this->fill_inner_cursor(region, location, data);
                this->validate_forward(region, location, data);
            }
            // otherwise the cursor is left in the invalid state
        }

        /** Exact region constructor.
         */
        cursor(
            local_iterator const& spot,     //!< matching interval in this layer
            key_type const& r,              //!< [in] Target region
            mapped_type const& p,           //!< [in] Target payload
            interval_cons& location,        //!< [out] Storage for current region
            payload_ptr& data           //!< [out] Storage for current payload
            )
            : super(spot.m_node)
        {
            if (super::m_node) {
                super::m_inner = spot.m_spot;
                this->fill_exact(r, p, location, data);
            }
            // otherwise the cursor is left in the invalid state
        }
        /** Inequality operator.
            Two cursors are not equal if they refer to different regions.
         */
        bool operator != ( self const& rhs) const { return !(*this == rhs); }

    };
    //! @endcond

    /** Utility class set that handles the bottom and upper cases of a layer.
        These exist to avoid duplicating the meta-programming for bottom and
        upper layers. Instead, any methods that need to be differentiated
        can be placed in these classes and then accessed generically through
        the @c util typedef.

        This indirection also provides de facto conditional compilation.
        Only methods in one of the base util classes are compiled, depending
        on whether this is a bottom or upper layer.

        Because of access issues, all methods here should be static and not
        require privileged access to this layer (any such elements needed
        for the method should be passed in). @c upper_util is used to bypass
        security on the next lower layer so that ancillary classes can get
        in without making that API public and thereby confusing to clients.

        I need to move additional functionality in to these, such as printing.
     */
    //!@{

    //! Utility class for bottom layer.
    struct bottom_util
    {
    };

    //! Utility class for upper layers.
    //  This inherits from the next lower layer to provide privileged access to its methods.
    //  We never instantiate the class so the memory footprint overhead is irrelevant.
    struct upper_util : public PAYLOAD
    {
        // Must use PAYLOAD here and not any typedefs, because the inheritance
        // can make them mean the wrong thing. gcc doesn't seem to pick up on
        // super class typedefs, while MSC does, making any dependence on that
        // not feasible.

        //! Ripple an erase request to lower layers.
        static void erase_lower(
            PAYLOAD& space, //!< Next lower space
            typename PAYLOAD::cursor const& cursor //!< Target element
            )
        {
            static_cast<upper_util&>(space).erase(cursor);
        }

        //! Create a cursor in the next lower layer.
        static typename PAYLOAD::cursor make_lower_cursor(
            PAYLOAD& space,             //!< [in] Flowspace for the cursor
            typename layer::interval_cons const& r,     //!< [in] Query region
            typename layer::interval_cons& location,    //!< [out] Storage for location of the current element
            payload_ptr& data           //!< [out] Payload of the current element
            )
        {
            typename PAYLOAD::cursor lc;
            lc = static_cast<upper_util&>(space).make_cursor(r.tail, location.tail, data);
            return lc;
        }

        //! Create a cursor in the next lower layer with an exact match.
        static typename PAYLOAD::cursor make_lower_cursor_exact(
            PAYLOAD& space,             //!< [in] Flowspace for the cursor
            typename layer::interval_cons const& r,     //!< [in] Query region
            typename layer::mapped_type const& p,       //!< [in] Target payload
            typename layer::interval_cons& location,    //!< [out] Storage for location of the current element
            payload_ptr& data           //!< [out] Payload of the current element
            )
        {
            typename PAYLOAD::cursor lc;
            lc = static_cast<upper_util&>(space).make_cursor_exact(r.tail, p, location.tail, data);
            return lc;
        }
    };
    //!@}

    //! The effective utility class.
    typedef typename mpl::if_c<IS_UPPER, upper_util, bottom_util>::type util;
    
    /** Make a cursor for this layer that intersects a region.
        @note Internal use only, because some of the argument types
        are not public.
     */
    cursor make_cursor(
        interval_cons const& r,     //!< [in] Target region
        interval_cons& l,           //!< [out] Storage for current region
        payload_ptr& d          //!< [in,out] Reference to payload
        )
    {
        cursor spot(this->find_intersecting(r.head), r, l, d);
        return spot;
    }

    /** Make a cursor for this layer that exactly matches a region.
     */
    cursor make_cursor_exact(
        key_type const& r,          //!< [in] Target region
        mapped_type const& p,       //!< [in] Target payload
        interval_cons& location,    //!< [out] Storage for current region
        payload_ptr& data       //!< [in,out] Reference to payload
        )
    {
        cursor spot(this->find(r.head), r, p, location, data);
        return spot;
    }

    //! Erase the element indicated by @a spot.
    void erase
        (cursor const& spot //!< Cursor referring to target element.
        )
    {
        typename node::handle r;
        bool flag;
        boost::tie(r, flag) = spot.erase();
        if (flag) m_root = r;
    }
	
public:
    /** Iterator for region queries.
        The value type for the iterator is a pair.
        The @c first element is the stored region (not the query region).
        The @c second element is the value stored with that region.
     */
    class iterator : public std::iterator<std::forward_iterator_tag, value_type_ref> {	
    private:
        region m_region;    //!< The query region
        value_type_ref m_data;  //!< Client data object for dereference
        mapped_type m_default_payload; //!< Data for dereference in invalid iterators.
        payload_ptr m_ptr; //!< Member for cursor logic to update the current payload
        cursor m_cursor;    //!< internal iterator implementation object

        // The value type needs to get passed down split because as we descend layers,
        // the key_type changes but the mapped_type doesn't. Without the split,
        // the metric data would have to be copied at every layer.
        // Additionally, the mapped_type is passed down as a pointer to a reference,
        // because the reference itself in @a m_data needs to be
        // updated, not @a m_default_data (its default referent).

	    /** Construct to refer the specified node.
         */
    	iterator(
            typename node::handle const& n, //!< Starting node
            region const& r                //!< the region over which to iterate
	    )
            : m_region(r)
            , m_data(m_default_payload)
            , m_ptr(&m_default_payload)
            , m_cursor(n, r, const_cast<region&>(m_data.first), m_ptr)
        {
            this->update_payload_reference(m_data.first); // region is set by m_cursor constructor.
        }

        /** Constructor for an exact region.
         */
        iterator(
            local_iterator const& spot, //!< Local interval
            typename layer::value_type const& v //!< Target element data
        )
            : m_region(layer::all())
            , m_data(m_default_payload)
            , m_ptr(&m_default_payload)
            , m_cursor(spot, v.first, v.second, const_cast<region&>(m_data.first), m_ptr)
        {
            this->update_payload_reference(m_data.first); // regiion is set by m_cursor constructor.
        }

        /** Rewrite reference in @a m_data to refer to the same instance as @a m_ptr.
         */
        void update_payload_reference(region const& r) {
            assert(m_ptr);
            /*  The reference in @a value_type needs to be modified to
             *  refer to the current client payload (so the client has
             *  write access to the payload). Standard assignment doesn't
             *  work, as that would copy the payload, not modify the reference.
             *  Instead, placement new is used to re-write the @a m_data object
             *  Because @c value_type_ref is POD, no destructor is needed.
             *  All of this cleverness avoids an allocate and free for each
             *  iterator increment. The region data ends up being copied on
             *  to itself, but that seems unavoidable (omitting it leads to
             *  it being reset to default constructed intervals). That copy
             *  should be much cheaper than a pair of heap operations. It is not
             *  permitted to directly construct the reference member so we
             *  can't avoid the copy that way either.
             */
            new (&m_data) value_type_ref(r, *m_ptr);
        }

    public:
        typedef iterator self; //!< Self reference type

        //! Default constructor.
        iterator() : m_data(m_default_payload), m_ptr(&m_default_payload) {
        }

        //! Prefix increment operator
        iterator& operator ++ () {
            region r;
            m_ptr = &m_default_payload;
            m_cursor.next(m_region, r, m_ptr);
            this->update_payload_reference(r);

            return *this;
        }

        //! Postfix increment operator
        iterator& operator ++ (int)
        {
            iterator old(*this);
            ++*this;
            return old;
        }
        
        //! Assignment
        self& operator = ( self const& that )
        {
            m_region = that.m_region;
            m_cursor = that.m_cursor;
            // If the other payload pointer refers to the default payload,
            // don't copy that, use our default instead.
            m_ptr = that.m_ptr == &that.m_default_payload ? &m_default_payload : that.m_ptr;
            this->update_payload_reference(that.m_data.first);

            return *this;
        }

        //! Equality operator
        bool operator == (iterator const& rhs) const { return m_cursor == rhs.m_cursor; }
        //! Inequality operator
        bool operator != (iterator const& rhs) const { return m_cursor != rhs.m_cursor; }

	    //! Value operator
	    value_type_ref const & operator * () const { return m_data; }
        //! Dereference operator
        value_type_ref const * operator -> () const { return &m_data; }

        /** Print text describing the iterator state on a stream.
         */
        std::ostream& print(std::ostream& s) const {
            return s << "< " << m_data.first << " : " << m_data.second << " >";
        }

        //! Our containing layer gets privileged access.
    	friend class layer;
    };

    //! Iterator for const flowspace.
    class const_iterator : public std::iterator<std::forward_iterator_tag, value_type_ref const>
    {
    private:
        typename layer::iterator m_spot; //!< Real iterator.
    public:
        typedef const_iterator self; //!< Self reference type.

        //! Default constructor.
        const_iterator() { }
        //! Copy constructor.
        const_iterator(self const& that)
            : m_spot(that.m_spot)
        {
        }
        //! Construct from non-const iterator
        const_iterator(typename layer::iterator const& iter)
            : m_spot(iter)
        {
        }

        //! Assignment.
        self& operator = ( self const& that )
        {
            m_spot = that.m_spot;
            return *this;
        }
        //! Cross assignment.
        self& operator = ( typename layer::iterator const& iter)
        {
            m_spot = iter;
            return *this;
        }
        //! pre-Increment
        self& operator++ ()
        {
            ++m_spot;
            return *this;
        }
        //! post-increment
        self operator++ (int)
        {
            self copy(*this);
            ++m_spot;
            return copy;
        }
        //! Equality
        bool operator == ( self const& that ) const
        {
            return m_spot == that.m_spot;
        }
        //! Inequality
        bool operator != ( self const& that ) const
        {
            return !(*this == that);
        }
        //! Equality with non-const iterator
        bool operator == ( typename layer::iterator const& iter ) const
        {
            return m_spot == iter;
        }
        //! Inequality with non-const iterator
        bool operator != ( typename layer::iterator const& iter ) const
        {
            return m_spot != iter;
        }

	    //! Value operator
	    value_type_ref const& operator * () { return *m_spot; }
        //! Dereference operator
        value_type_ref const* operator -> () { return m_spot.operator->(); }

        /** Print text describing the iterator state on a stream.
         */
        std::ostream& print(std::ostream& s) const {
            return m_spot.print(s);
        }

    };

    //! Default constructor
    /*! Constructs an empty layer. */
    layer()
    {
    }

    //! Destructor
    ~layer()
    {
    }

    /** Return a region that covers the entire flowspace */
    static key_type all()
    {
        key_type r;
        imp::maximize_region(r);
        return r;
    }

    //! Check if the flowspace is empty.
    bool is_empty() const
    {
        return !m_root;
    }

    /** Standard iterator.
        @note There is no difference between this iterator and the region query
        iterator. This simply uses a query that is the entire flowspace.
     */
    iterator begin()
    {
        return this->begin(this->all());
    }
    //! Overload for user covenience (const version).
    const_iterator begin() const
    {
        return const_iterator(const_cast<self*>(this)->begin());
    }
	
    /** Region query iterator.
        @a r is the @em query @em region. Given a query region, there exists
        a (possibly empty) set of regions stored in the flowspace that intersect
        the query region. This set is lexicographically ordered and this method
        returns an iterator that refers to the first region in that set.
     */
    iterator begin(region const& r)
    {
        typename node::handle n = this->find_intersecting(r.head);
        return iterator(n,r);
    }
    //! Overload for user convenience (const version).
    const_iterator begin(region const& r) const
    {
        return const_iterator(const_cast<self*>(this)->begin(r));
    }

    /** Query region iterator.
        This returns a query region iterator that is past the end of the
        query region set.
        @note All query region end iterators are equal to each other and
        to the default constructed iterator.
     */
    iterator end()
    {
        return iterator();
    }
    //! Overload for user covenience (const version)
    const_iterator end() const
    {
        return const_iterator();
    }

    /** Query region iterator.
        This returns a query region iterator that is past the end of the
        query region set.
        @note All end iterators are equal to each other and
        to the default constructed iterator. Therefore the argument to
        this method is ignored. It is purely for client convenience.
     */
    iterator end(region const&)
    {
        return iterator();
    }
    //! Overload for user covenience (const version)
    const_iterator end(region const&) const
    {
        return const_iterator();
    }

    /** Locate a specific value in the flowspace.
        This finds an exact region match. To find intersecting regions,
        use the region iterator.
     */
    iterator find
        ( value_type const& v //!< Element data to find
        )
    {
        return iterator(this->find(v.first.head), v);
    }
    //! Overload for user covenience (const version)
    const_iterator find
        ( value_type const& v //!< Element data to find
        ) const
    {
        return const_iterator(const_cast<self*>(this)->find(v));
    }
    
    /** Add an interval with data to the flowspace.
        @return Indeterminate value.
     */
    bool insert(
        value_type const& v //!< The region to insert
        )
    {
        assert(imp::is_valid(v.first));
        if (! m_root) {
            m_root = new node(v);
            m_root->set_color(node::BLACK);
        } else {
            typename node::handle n;
            typename node::direction d;

            // Find insert location
            boost::tie(n, d) = m_root->search(boost::bind(&node::compare_metric, _1, v.first.head.min()));
            if (node::NONE == d) { // already an outer tree node for this metric, add this value to that node.
                n->insert(v);
                n->ripple_structure_fixup();
            } else { // Not in the tree, but this node should be the parent
                m_root = boost::dynamic_pointer_cast<node>(n->insert_child(new node(v), d));
            }
        }

        return true;
    }

    void erase(iterator const& spot)
    {
        this->erase(spot.m_cursor);
    }

    //! Write iterator to stream.
    friend LOCAL std::ostream& operator << (std::ostream& s, iterator const& i)
    {
        return i.print(s);
    }

    // SKH 6/12/06 - Moved to gcc 4.1 and this operator is no longer found.
    //! Write value_type to stream.
    friend LOCAL std::ostream& operator << (std::ostream& s, value_type const& vt) 
    {
    	return s << vt.first << " = " << vt.second;
    }

    friend LOCAL std::ostream& operator << (std::ostream& s, value_type_ref const& vt)
    {
        return s << vt.first << " = " << vt.second;
    }

    std::ostream& print(std::ostream & s, int indent)
    {
    	return m_root->print(s, indent, 0, 0);
    }
    
    bool validate()
    {
        return !m_root || m_root->validate();
    }

protected:
    typename node::handle m_root; //!< The root of the tree

    // Try to declare all other layer instantiations as friends of this one.
    template < typename T > friend struct imp::member_cursor_mf::apply;
};

template < typename METRIC, typename PAYLOAD >
inline bool operator == ( typename layer<METRIC,PAYLOAD>::iterator const& lhs, typename layer<METRIC,PAYLOAD>::const_iterator const& rhs )
{
    return rhs == lhs;
}
template < typename METRIC, typename PAYLOAD >
inline bool operator != ( typename layer<METRIC,PAYLOAD>::iterator const& lhs, typename layer<METRIC,PAYLOAD>::const_iterator const& rhs )
{
    return rhs != lhs;
}

}} // namespace flowspace, ngeo

# undef API
