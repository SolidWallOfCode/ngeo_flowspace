/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

# pragma once

# include <local/boost_intrusive_ptr.hpp>

#   if NG_STATIC
#       define API
#   else
#       if defined(_MSC_VER)
#           if defined(NETWORK_GEOGRAPHICS_FLOWSPACE_API)
#               define API _declspec(dllexport)
#           else
#               define API _declspec(dllimport)
#           endif
#           pragma warning(push)
#           pragma warning(disable:4275)
#       else
#           define API
#       endif
#   endif

/** @file
    Internal nodes used by a flowspace.
 */

namespace ngeo { namespace flowspace { namespace imp {

/** Red/Black tree base node.
    This implements node operations that are not payload dependent.
 */
class API node_base : public  boost::reference_counter<node_base>
{
public:
    typedef node_base self; //!< self reference type

    /*	Instantiate this in the library.
        @note ICE if we use "handle" here instead of the explicit template.
        @note I'm not sure this really helps other than shutting up the compiler.
      */
# if defined(_MSC_VER)
    template class API boost::intrusive_ptr<node_base>;
#endif

    /*! Standard parameter type */
    typedef boost::intrusive_ptr<self> handle;

    //! Node colors
    typedef enum { RED, BLACK } color;

    //! Directional constants
    typedef enum { NONE, LEFT, RIGHT } direction;

    // Methods
    /* We don't initialize the handles because they take of themselves */
    node_base() : m_color(RED), m_parent(0), m_next(0) { }; //!< Default constructor
    /* Need a virtual destructor because we have virtual methods */
    virtual ~node_base() { }

    //! Rotate the subtree rooted at this node
    /** The node is rotated in to the position of one of its children.
        Which child is determined by the direction parameter @a d. The
        child in the other direction becomes the new root of the subtree.

        If the parent pointer is set, then the child pointer of the original
        parent is updated so that the tree is left in a consistent state.

        @note If there is no child in the other direction, the rotation
        fails and the original node is returned. It is @b not required
        that a child exist in the direction specified by @a d.

        @return The new root node for the subtree.
     */
    handle rotate(
        direction d //!< The direction to rotate
        );

    //! Get a handle to the child node in the specified direction
    /** @return The child in the direction @a d. If there is no
        child in that direction a @c nil handle is returned.
     */
    handle get_child(
        direction d //!< The direction of the desired child
        );

    //! Set the child of node.
    /** The @a d child is set to the node @a n. The pointers in this
        node and @a n are set correctly. This can only be called if
        there is no child node already present.

        @return A handle to inserted child node.
     */
    handle set_child(
        handle const& n, //!< The node to set as the child
        direction d //!< The direction of the child
        );

    /** Insert a new node as a child.
        This sets the child node and inserts the child in to the threaded list.
        It is an error to call this if the target location is not empty.
        @return A handle to the inserted child node.
     */
    handle insert_child(
        handle const& n, //!< The node to insert as the child
        direction d //!< The direction the child should be from the parent
        );

    /** Remove this node from the tree.
        @return The new root node.
     */
    handle remove();

    /** Determine which child a node is
        @return @c LEFT if @a n is the left child,
        @c RIGHT if @a n is the right child,
        @c NONE if @a n is not a child
     */
    direction get_child_direction(
        handle const& n //!< The presumed child node
        )
    {
        return (n == m_left) ? LEFT : (n == m_right) ? RIGHT : NONE;
    }

    /** Get the left most descendant of this node.
        @return The left most descendant of this node
     */
    handle get_leftmost_descendant();

    /** Get the parent node.
        @return A handle to the parent node or a @c nil handle if no parent.
     */
    handle get_parent() { return handle(m_parent); }

    void clear_parent() { m_parent = 0; }

    void clear_child(direction dir)
    {
        assert(NONE != dir);
        dir == LEFT ? m_left = 0 : m_right = 0;
    }

    color get_color() const { return m_color; }

    void set_color(color new_color) { m_color = new_color; }

    /** Get next node for in-order traversal.
        @note This is a constant time operation.
        @return A handle to next in-order node or @c NIL if this is the last node.
     */
    handle get_next() const { handle n(m_next); return n; }

    /** Get the previous node for in-order traversal.
        @note This is a log(N) time operation.
        @return A handle to the previous in-order node or @c NIL if this is the first node.
     */
    handle get_prev() const;

    //! Reverse a direction
    /** @return @c LEFT if @a d is @c RIGHT, @c RIGHT if @a d is @c LEFT,
        @c NONE otherwise.
     */
    direction flip(direction d) { return LEFT == d ? RIGHT : RIGHT == d ? LEFT : NONE; }

    /** @name Subclass hook methods */
    //@{
    //! Structural change notification
    /** This method is called if the structure of the subtree rooted at
        this node was changed.
		
	@note The base method is empty so that subclasses
	are not required to use this hook.
     */
    virtual void structure_fixup() {}

    /** Called from @c validate to perform any additional validation checks.
        @return @c true if the validation is successful, @c false otherwise.
        @note The base method simply returns @c true.
     */
    virtual bool structure_validate() { return true; }
    //@}

    /** Perform internal validation checks.
        @return 0 on failure, black height of the tree on success.
     */
    int validate();

    /** Iterator over nodes.
        The iteration is over all nodes, regardless of which node is used to create
        the iterator. The node passed to the constructor just sets the current
        location.
     */
    class API iterator : public std::iterator<std::forward_iterator_tag, node_base, int>
    {
    public:
        iterator(); //!< Default constructor, iterator with undefined value
        //! Iterator referring to node @a n
        iterator(
            handle const& n //!< the node for the iterator to reference
            );
        //! Copy constructor
        iterator(
            iterator const& src //!< source iterator to copy
            );

        reference operator* (); //!< value operator
        pointer operator -> (); //!< dereference operator
        iterator& operator++(); //!< next node (prefix)
        iterator operator++(int); //!< next node (postfix)

        /** Equality.
            @return @c true if the iterators refer to the same node.
         */
        bool operator == (iterator const& rhs) const;
        /** Inequality.
            @return @c true if the iterators refer to different nodes.
         */
        bool operator != (iterator const& rhs) const { return ! (*this == rhs); }
    private:
        handle m_node; //!< node referenced by the iterator
    };

protected:

    color m_color; //!< node color

    /*	Need to be a little careful here, because we're using reference
        counting pointers and the parent pointer creates a cycle.
        For that reason the parent and next pointers are raw pointers.
     */
    handle m_left; //!< left child
    handle m_right; //!< right child
    self* m_parent; //!< parent node (needed for rotations)
    self* m_next;   //!< next node in order traversal

    /** Replace this node with another node.
        This is presumed to be non-order modifying so the next reference
        is @b not updated.
     */
    void replace_with(
        handle const& n //!< Node to put in place of this node.
        );

    //! Rebalance the tree starting at this node
    /** The tree is rebalanced so that all of the invariants are
        true. The (potentially new) root of the tree is returned.

        @return The root node of the tree after the rebalance.
     */
    handle rebalance_after_insert();
	
    /** Rebalance the tree after a deletion.
        Called on the lowest modified node.
        @return The new root of the tree.
     */
    handle rebalance_after_remove(
        color c, //!< The color of the removed node.
        direction d //!< Direction of removed node from parent
        );

    //! Invoke @c structure_fixup() on this node and all of its ancestors.
    handle ripple_structure_fixup();
};

/** Convenience equality operators for handling NIL node cases.
    These treat NIL nodes as having the color BLACK.
 */
inline bool operator == ( node_base::handle const& n, node_base::color c )
{
    return c == ( n ? n->get_color() : node_base::BLACK);
}
inline bool operator == ( node_base::color c, node_base::handle const& n )
{
    return n == c;
}

}}} // namespace

# undef API
# if defined(_MSCVER)
#   pragma warning(pop)
# endif
