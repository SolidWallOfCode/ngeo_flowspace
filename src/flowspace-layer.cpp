/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */
# include <flowspace.h>

template class boost::intrusive_ptr<ngeo::flowspace::imp::node_base>;

namespace ngeo { namespace flowspace {

/**
 * Performs a rotation to rebalance the tree
 */
imp::node_base::handle
imp::node_base::rotate(direction d)
{
    handle parent(m_parent); // cache this because we'll overwrite the member
    handle child(this); // default return - rotation failed
    direction child_dir = (parent) ? parent->get_child_direction(child) : NONE;
    
    // Need to be careful that we don't drop all pointers to a node
    // during the rotation and have it evaporate on us
    assert (d != NONE);
    direction other_dir = this->flip(d);

    if (d != NONE && this->get_child(other_dir)) {
        child = this->get_child(other_dir);
        this->clear_child(other_dir);
        this->set_child(child->get_child(d), other_dir);
        child->clear_child(d);
        child->set_child(this, d);
        child->structure_fixup();
        this->structure_fixup();
        if (parent) {
	        parent->clear_child(child_dir);
	        parent->set_child(child, child_dir);
        } else {
	    child->clear_parent();
        }
    }
    return child;
}

imp::node_base::handle
imp::node_base::get_child(direction d)
{
	return d == RIGHT ? m_right
		: d == LEFT ? m_left
		: handle(0)
		;
}

imp::node_base::handle
imp::node_base::set_child(handle const& n, direction d)
{
    assert(d != NONE);

    if (n)
        n->m_parent = this;

    if (d == RIGHT) {
        assert(!(n && m_right));
        m_right = n;
    }
    else if (d == LEFT) {
        assert(!(n && m_left));
        m_left = n;
    }
    return n;
}

imp::node_base::handle
imp::node_base::get_leftmost_descendant()
{
    self* child = this;
    while (child->m_left)
        child = child->m_left.get();
    return handle(child);
}

// Returns the root node
imp::node_base::handle
imp::node_base::ripple_structure_fixup()
{
    handle root(this); // last node seen, root node at the end
    handle p(this);
    while (p) {
        p->structure_fixup();
        root = p;
        p = p->get_parent();
    }
    return root;
}

imp::node_base::handle
imp::node_base::get_prev() const
{
    handle n;
    /*  The rule is, if there is a left child, the predecessor is
        the rightmost child of that left child.
        Otherwise, the predecessor is the first ancestor that this
        node is to the right of.
     */
    if (m_left) {
        n = m_left;
        while (n->m_right)
            n = n->m_right;
    } else {
        handle c(const_cast<self*>(this));
        n = m_parent;
        while ( n && RIGHT != n->get_child_direction(c))
            c = n, n = n->m_parent;
    }
    assert(!n || n->m_next == this);
    return n;
}

void
imp::node_base::replace_with(handle const& n)
{
    handle protect(this); // protect this node from GC

    n->set_color(m_color);

    // Need to be careful on all of these to check for
    // n and this being adjacent via the relationship
    // we're updating. If it is, we want to leave that
    // pointer NIL.
    if (m_parent) {
        direction d = m_parent->get_child_direction(this);
        m_parent->set_child(0, d);
        if (m_parent != n) m_parent->set_child(n, d);
    } else {
        n->clear_parent();
    }

    n->m_left = n->m_right = 0;
    if (m_left != n) n->set_child(m_left, LEFT);
    if (m_right != n) n->set_child(m_right, RIGHT);
    m_left = m_right = 0;
}

imp::node_base::handle
imp::node_base::insert_child(handle const& n, direction d)
{
    /*  Set the parent / child links and then do the threaded list. */
    this->set_child(n, d);
    if (RIGHT == d) {
        // =this= is the predecessor, so splicing is easy
        n->m_next = m_next;
        m_next = n.get();
    } else if (LEFT == d) {
        n->m_next = this; // =this= is the success of the child.
        /*  Now we have to find the predecessor for the child.
            Because we always insert at a leaf, the predecessor node is
            always an ancestor. So we search up until we find the node that
            points to this one and switch to point to the new child node.
            If we don't find one, the child is the first element, so
            we do nothing.
         */
        self * p;
        for ( p = m_parent ; p && p->m_next != this ; p = p->m_parent )
            ;
        if (p)
            p->m_next = n.get();
    }
    return n->rebalance_after_insert();
}

/* Rebalance the tree. This node is the unbalanced node. */
imp::node_base::handle
imp::node_base::rebalance_after_insert()
{
    self* x(this); // the node with the imbalance
    // A subtlety -- we can potential rotate the old root away leaving the new
    // root with no references, causing the entire tree to get GC'd.
    // By caching the result of the rotate in this variable, we prevent that.
    handle rotate_return;

    assert(x->m_color == RED);
	
    while (x && x->m_parent == RED) {
        direction child_dir = NONE;
        
        if (x->m_parent->m_parent)
    	    child_dir = x->m_parent->m_parent->get_child_direction(x->m_parent);
        else
	    break;
        direction other_dir(flip(child_dir));
        
        handle y = x->m_parent->m_parent->get_child(other_dir);
        if (y == RED) {
    	    x->m_parent->m_color = BLACK;
    	    y->m_color = BLACK;
    	    x = x->m_parent->m_parent;
    	    x->m_color = RED;
        } else {
    	    if (x->m_parent->get_child(other_dir) == x) {
	        x = x->m_parent;
	        rotate_return = x->rotate(child_dir);
    	    }
    	    x->m_parent->m_color = BLACK;
    	    x->m_parent->m_parent->m_color = RED;
    	    rotate_return = x->m_parent->m_parent->rotate(other_dir);
        }
    }
    
    // every node above this one has a subtree structure change,
    // so notify it. serendipitously, this makes it easy to return
    // the new root node.
    handle root(this->ripple_structure_fixup());
    root->m_color = BLACK;

    return root;
}


// Returns new root node
imp::node_base::handle
imp::node_base::remove()
{
    handle protect(this); // protect this node from GC
    handle root; // new root node, returned to caller

    // Only one next pointer needs to be updated, that in the predecessor of
    // this node, the node that is being truly removed from the tree.
    handle prev_node = this->get_prev();
    if (prev_node) prev_node->m_next = this->m_next;

    /** Handle two special cases first.
        - This is the only node in the tree, return a new root of NIL
        - This is the root node with only one child, return that child as new root
     */
    if (!m_parent && !(m_left && m_right)) {
        if (m_left) {
            m_left->m_parent = 0;
            root = m_left;
            root->m_color = BLACK;
        } else if (m_right) {
            m_right->m_parent = 0;
            root = m_right;
            root->m_color = BLACK;
        }
        return root;
    }

    /** The node to be removed from the tree.
        If this (the target node) has both children, we remove
        its successor, which will cannot have a left child, and
        put that node in place of the target node.Otherwise this
        node has at most one child, so we can remove it.
        Note that the successor of a node with a right child is always
        a right descendant of the node. Therefore, remove_node
        is an element of the tree rooted at this node.
        Because of the initial special case checks, we know
        that remove_node is @b not the root node.
     */
    handle remove_node(m_left && m_right ? m_next : this);

    // This is the color of the node physically removed from the tree.
    // Normally this is the color of @a remove_node
    color remove_color = remove_node->get_color();
    // Need to remember the direction from @a remove_node to @a splice_node
    direction d(NONE);

    // The child node that will be promoted to replace the removed node.
    // The choice of left or right is irrelevant, as remove_node has at
    // most one child (and splice_node may be NIL if remove_node has no children).
    handle splice_node(remove_node->m_left ? remove_node->m_left : remove_node->m_right);

    if (splice_node) {
        // @c replace_with copies color so in this case the actual color lost is that
        // of the splice_node.
        remove_color = splice_node->get_color();
        remove_node->replace_with(splice_node);
    } else {
        // No children on remove node so we can just clip it off the tree
        // We update splice_node so that after this if, it is always the
        // node where the physical removal occurred.
        splice_node = remove_node->m_parent;
        d = splice_node->get_child_direction(remove_node); // The direction is by rebalance
        splice_node->set_child(0, d);
    }

    // If the node to pull out of the tree isn't this one, 
    // then replace this node in the tree with that removed
    // node in liu of copying the data over.
    if (remove_node.get() != this) {
        // Don't leave @a splice_node referring to a removed node
        if (splice_node == this) splice_node = remove_node;
        this->replace_with(remove_node);
    }

    root = splice_node->rebalance_after_remove(remove_color, d);
    root->m_color = BLACK;
    return root;
}

/** Helper function to protect the tree during rotations.
    It copies the new top node for the rotation in to the
    handle if that top is a new root for the tree.
 */
imp::node_base::handle
protected_rotate(imp::node_base::handle const& n, imp::node_base::direction d, imp::node_base::handle& sanctuary)
{
    imp::node_base::handle tmp(n->rotate(d));
    if (!tmp->get_parent())
        sanctuary = tmp;
    return tmp;
}

/**
 * Rebalance tree after a deletion
 * Called on the spliced in node or its parent, whichever is not NIL.
 * This modifies the tree structure only if @a c is @c BLACK.
 */
imp::node_base::handle
imp::node_base::rebalance_after_remove(
    color c, //!< The color of the removed node
    direction d //!< Direction of removed node from its parent
    )
{
    handle root;
    handle rotate_return;

    if (BLACK == c) { // only rebalance if too much black
        handle n(this);
        handle parent(n->m_parent);

        // If @a direction is set, then we need to start at a leaf psuedo-node.
        // This is why we need @a parent, otherwise we could just use @a n.
        if (NONE != d) {
            parent = n;
            n = 0;
        }

        while (parent) { // @a n is not the root
            // If the current node is RED, we can just recolor and be done
            if (n == RED) {
                n->set_color(BLACK);
                break;
            } else {
                // Parameterizing the rebalance logic on the directions. We write for the
                // left child case and flip directions for the right child case
                direction near(LEFT), far(RIGHT);
                if ((NONE == d && parent->get_child_direction(n) == RIGHT) || RIGHT == d) {
                    near = RIGHT;
                    far = LEFT;
                }

                handle w = parent->get_child(far); // sibling(n)
                assert(w);

                if (w->m_color == RED) {
                    w->m_color = BLACK;
                    parent->m_color = RED;
                    rotate_return = protected_rotate(parent, near, root);
                    w = parent->get_child(far);
                    assert(w);
                }

                if (w->get_child(near) == BLACK && w->get_child(far) == BLACK) {
                    w->m_color = RED;
                    n = parent;
                    parent = n->m_parent;
                    d = NONE; // Cancel any leaf node logic
                } else {
                    if (w->get_child(far) == BLACK) {
                        w->get_child(near)->m_color = BLACK;
                        w->m_color = RED;
//                        rotate_return = w->rotate(far);
                        rotate_return = protected_rotate(w, far, root);
                        w = parent->get_child(far);
                    }
                    w->m_color = parent->m_color;
                    parent->m_color = BLACK;
                    w->get_child(far)->m_color = BLACK;
                    rotate_return = protected_rotate(parent,near,root);
                    break;
                }
            }
        }
    }
    root = this->ripple_structure_fixup();
    return root;
}
//----------------------------------------------------------------------------
/** Ensure that the local information associated with each node is correct globally
    This should only be called on debug builds as it breaks any efficiencies
    we have gained from our tree structure.
 */
int
imp::node_base::validate()
{
    int black_ht = 0;
    int black_ht1, black_ht2;

    assert(m_left != this);
    assert(m_right != this);

    if (m_left) {
        black_ht1 = m_left->validate();
    }
    else
        black_ht1 = 1;

    if (black_ht1 > 0 && m_right)
        black_ht2 = m_right->validate();
    else
        black_ht2 = 1;

    if (black_ht1 == black_ht2) {
        black_ht = black_ht1;
        if (this->m_color == BLACK)
            black_ht++;
        else {	// No red-red
            if (m_left == RED)
                black_ht = 0;
            else if (m_right == RED)
                black_ht = 0;
            if (black_ht == 0)
                std::cout << "Red-red child\n";
        }
    } else {
        std::cout << "Height mismatch " << black_ht1 << " " << black_ht2 << "\n";
    }
    if (black_ht > 0 && !this->structure_validate())
        black_ht = 0;

    return black_ht;
}
//----------------------------------------------------------------------------
imp::node_base::iterator::iterator() { }
imp::node_base::iterator::iterator(iterator const& src) : m_node(src.m_node) { }
imp::node_base::iterator::iterator(handle const& n) : m_node(n) { }

imp::node_base::iterator&
imp::node_base::iterator::operator ++ () // prefix increment
{
    m_node = m_node->m_next;
    return *this;
}

imp::node_base::iterator
imp::node_base::iterator::operator ++ (int) // postfix increment
{
	iterator old(*this);
	++*this;
	return old;
}

bool
imp::node_base::iterator::operator == (iterator const& rhs) const
{
	return m_node == rhs.m_node;
}

imp::node_base::iterator::reference
imp::node_base::iterator::operator * ()
{
	return *m_node;
}

imp::node_base::iterator::pointer
imp::node_base::iterator::operator -> ()
{
	return m_node.get();
}
//----------------------------------------------------------------------------
}} 
// end namespaces
