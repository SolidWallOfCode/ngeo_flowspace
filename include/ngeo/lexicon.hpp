/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

# pragma once

/** @file
    Lexicon header and implementation. Lexicon is a class to provide naming services.
    @note Header only library.
 */

# include <set>
# include <string>
# include <stdexcept>
# include <boost/bind.hpp>
# include <boost/variant.hpp>
# include <boost/function.hpp>
# include <boost/multi_index_container.hpp>
# include <boost/multi_index/member.hpp>
# include <boost/multi_index/hashed_index.hpp>
# include <boost/multi_index/ordered_index.hpp>
# include <boost/multi_index/random_access_index.hpp>
# include <boost/foreach.hpp>
# include <ngeo/string_util.hpp>
# include <local/boost_format.hpp>

namespace ngeo {

namespace bmi = boost::multi_index;

/** A collection of values with associated names.
    @c Lexicon is the primary container for naming values. It contains a set of
    values, and for each value a set of names and a mark indicating which name
    is the @em primary name. During parsing, any name is valid, but during write
    the primary name is used.

    The named type @a T is presumed to be cheaply copyable.

    All names must unique (ignoring case) over the entire @c Lexicon.

    A lexicon supports default names and values. A default name is used whenever a
    name is requested for a value but no name is associated with that value. Similarly,
    a default value is returned whenever a value is requested for a name but that
    name is not defined. If there are illegal values this is handy for simplifying
    the processing of names and / or values.

    The defaults can also be functors. The default name functor is invoked in the same
    situations as the default name, except that the corresponding value is passed
    to the functor, which is expected to return the appropriate string. Similarly,
    the default value functor is passed the name from the request and must return
    a value, which is then returned to the client.

    For example, to use the string "RESERVED" for every value that doesn't have an
    associated name -
    @code
    lexicon.set_default_name("RESERVED");
    @endcode
    To use the numeric string for every value that doesn't have an associated name
    in a @c Lexicon<int> -
    @code
    lexicon.set_default_name(boost::bind(&boost::lexical_cast<std::string, int>, _1));
    @endcode

    @c Lexicon also provides a convenient mechanism for initialization in a
    declaration via the nested @c init type, thereby avoiding having to
    arrange for initialization code to run to load data.

    @note Iteration over all name / value associations is supported, but only constant
    iteration. All changes to the set of associations must be done explicitly via
    methods, never by modifying the target values of an iterator.
 */
template < typename K ///< Type of which to name instances
         >
class lexicon
{
public:
    typedef lexicon self; ///< Self reference type
    typedef K key_type; ///< First template parameter
    typedef std::string name_type; ///< Type used for names

protected:
    struct BY_KEY; ///< Tag for access by native type.
    struct BY_NAME; ///< Tag for access by name
    struct BY_POS; ///< Tag for access by index.

public:
    struct init; // forward declare

    /// A value / name association.
    class value_type {
    public:
        /// Get the key for this association.
        key_type key() const { return _key; }
        /// Get the name for this association.
        name_type const& name() const { return _name; }
        /// Test if this is the primary association for the value.
        bool is_primary() const { return _primary; }
        /// Comparison for ordering by key
        bool operator < (value_type const& that) const { return _key < that._key; }
        /// Functor for compare against a key.
        struct key_compare
        {
            bool operator () ( value_type const& lhs, value_type const& rhs ) const { return lhs.key() < rhs.key(); }
            bool operator () ( value_type const& lhs, key_type rhs ) const { return lhs.key() < rhs; }
            bool operator () ( key_type lhs, value_type const& rhs ) const { return lhs < rhs.key(); }
        };
    private:
        key_type _key;     ///< Native value
        name_type _name;   ///< Name for value
        /// Flag for being the primary association.
        /// @internal @c mutable because it's not a key so we can modify it directly.
        mutable bool _primary;

        /// Default constructor.
        value_type() : _primary(false) {}
        /// Construct from @a name and @a key.
        value_type
            ( key_type key      ///< Key
            , name_type const& name    ///< Name
            )
            : _key(key), _name(name), _primary(false)
        {}
        /// Construct with explicit @a primary state
        value_type
            ( key_type key ///< Key
            , name_type const& name ///< Name
            , bool primary ///< Mark this as primary
            )
            : _key(key), _name(name), _primary(primary)
        {}

        friend class lexicon;
        friend struct init;
    };

protected:
    /** Storage for key / name associations.
        The structure is optimized for the case where few values have more than
        one name and rarely will values with multiples names have more
        than 3 names. That means we can store all the associations in one big list
        and linear search all values for a name to find the primary name.

        We keep a third random access index for clients that need to treat the
        names as an array (this is primarily useful for user interface elements
        such as menus / dropdowns).

        @internal This doesn't use @c hashed_unique because that would require
        writing a case insensitive hash (because the compare isn't used if
        the hashes are distinct).
     */
    typedef bmi::multi_index_container
        < value_type
        , bmi::indexed_by
            < bmi::hashed_non_unique < bmi::tag<BY_KEY> , bmi::member<value_type, key_type, &value_type::_key > >
            , bmi::ordered_unique < bmi::tag<BY_NAME>, bmi::member<value_type, name_type, &value_type::_name > , util::ascii_iless >
            , bmi::random_access < bmi::tag<BY_POS> >
            >
        > Container;

    Container _container; ///< Association container
    // Define types for the indices so we don't forget the reference.
    typedef typename Container::template index<BY_KEY>::type& key_index; ///< Index on key
    typedef typename Container::template index<BY_NAME>::type& name_index; ///< Index on name
    typedef typename Container::template index<BY_POS>::type& pos_index; ///< Index by position
    typedef typename Container::template index<BY_POS>::type const& const_pos_index; ///< Index by position.

    /// Accessor for key index
    key_index by_key() { return _container.template get<BY_KEY>(); }
    /// Reader for key index
    typename Container::template index<BY_KEY>::type const& by_key() const { return _container.template get<BY_KEY>(); }
    /// Accessor for name index
    name_index by_name() { return _container.template get<BY_NAME>(); }
    /// Iterate in key order
    typedef typename Container::template index<BY_KEY>::type::iterator key_iterator;
    /// Iterate in key order
    typedef typename Container::template index<BY_KEY>::type::const_iterator const_key_iterator;
    /// Iterate in name order
    typedef typename Container::template index<BY_NAME>::type::iterator name_iterator;

    /// Nil type for use in signaling no data.
    struct nil_type { nil_type() {} };

    /// Functor type for generating a key from a name
    typedef boost::function<key_type (name_type const&)> key_generator;
    /// Default key handler.
    /// It's either @c nil, a key, or a functor for generating a key from a name.
    /// Default constructs to @c nil because @c nil_type is first in the type list.
    boost::variant<nil_type, key_type, key_generator> _default_key;
    /// Functor type for generating a name from a key
    typedef boost::function<name_type (key_type)> name_generator;
    /// Default name handler.
    /// It's either @c nil, a name, or a functor for generating a name from a key.
    /// Default constructs to @c nil because @c nil_type is first in the type list.
    boost::variant<nil_type, name_type, name_generator> _default_name;

    /// List of name / values to force to be primary.
    typedef std::set<key_type> Forcing;

    bool _auto_sort; ///< Keep the random access sorted.
    bool _sorted; ///< Set when random access is sorted, reset when set of names is changed.

    /// Sort the list iff auto sort is enabled and the dirty bit (@a _sorted) is set.
    void auto_sort()
    {
        if (_auto_sort && ! _sorted) {
            _container.template get<BY_POS>().sort(typename value_type::key_compare());
            _sorted = true;
        }
    }

public:
    /** Initialization support.
        A structure to enable simpler yet efficient initialization of a @c Lexicon.
        This can be constructed and then passed or assigned to a @c Lexicon. Because
        it is passed as an argument, it can be manipulated without additional initialization
        logic.

        Manipulation is performed via the function operator, which is overloaded to accept
        values, names, or value / name pairs.
        - A value sets that value as the current value
        - A name adds that name to the current value. The first name set for a value becomes the primary value.
        - A value / name pair sets the current value to that value and then adds the name to the value.

        This example sets up a @c Lexicon that names @c int values. The numbers 1,2,3 get the names "one", "two", "three"
        respectively. The number 4 gets the primary name "four" and the alias "quad". Similarly, the number 5 has the
        primary name "five" and the alias "cinco".
        @code
        typedef ngeo::Lexicon<int> LI;
        LI lexicon(LI::init(1, "one")
                           (2, "two")
                           (3)("three")
                           (4)("four")("quad")
                           (5, "five")("cinco")
                  );
        @endcode

        A @c init structure can be directly assigned to a @c Lexicon, which clears any current
        values and names and replaces them with the contents of the @c init.

        A @c init can be added via @c operator+= to a @c Lexicon. The associations are added to the @c Lexicon.
        To force a change in the primary name for a value that already has one, the name must be set in the @c init
        using square brackets instead of parentheses. This can be done in any @c init initialization but is only
        useful in this case. Brackets are not needed if the value was not already present in the @c Lexicon, in which case the
        previous rules apply. Brackets are useful only to @b change the primary when the value is already present.
        Adding the same name multiple times to a value is silently ignored. A consequence is that a name in brackets
        does not have to be previously unused - if it is already present, it will not be added again but it will be set
        to be the primary name.

        @code
        // Add '6' with primary "six" and aliases "sextet" and "roku"
        // Add "first" as the primary for '1' and change the primary for '4' to "quad".
        lexicon += LI::init(6)("six")("sextet")("roku")
                           (1)["first"]
                           (4)["quad"]
                            ;
        @endcode

        Default values can be set with the @c default_key and @c default_name methods.
        @code
        LI lexicon(LI::init()
                           (1, "one")
                           (2, "two")
                           (3)("three")
                           .default_key(-1)
                           .default_name("INVALID")
                           (4)("four")
                   );
        @endcode

        @note This @c struct is designed to be used only inline for initialization. Other uses may fail in mysterious ways.
     */
    struct init
    {
        typedef init self; ///< Self reference type
        /// Default constructor.
        init() : _primary(true) {}
        /// Construct with current @a key.
        init
            ( key_type key ///< Initial value for current key
            ) : _primary(true)
        { (*this)(key); }
        /// Construct with current @a key and @a name
        init
            ( key_type key      ///< Key for primary name and to make current
            , name_type const& name    ///< Primary name
            ) : _primary(true)
        { (*this)(key, name); }

        /// Set the current @a key.
        /// @return A reference to @c this object.
        self& operator ()
            ( key_type key ///< Key to make current
            )
        {
            _key = key;
            _primary = true;
            return *this;
        }

        /** Add a @a name to the current key.
            If this is the first name for the key, it becomes the primary name.
            It is an error for no current key to have been previously set.
            @return A reference to @c this object.
         */
        self& operator ()
            ( name_type const& name ///< Name to associate with current key
            )
        {
            if (_primary)
                _container.set_primary(_key, name);
            else
                _container.define(_key, name);
            _primary = false;
            return *this;
        }

        /** Set the current @a key and add a @a name.
            Set the current @a key and then add @a name to it.
            If this is the first name for the key, it becomes the primary name.
            @return A reference to @c this object.
         */
        self& operator ()
            ( key_type key    ///< Key for name and to make current
            , name_type const& name  ///< Name
            )
        {
            return (*this)(key) , (*this)(name);
        }

        /** Set the primary @a name for the current key.
            It is an error for no current key to have been previously set.
            It it an error for more than one primary name to be set for the same key.

            @return A reference to @c this object.
            @throw std::domain_error If a different name has already be forced to be primary for the current key.

            @note This is useful when augmenting an existing @c Lexicon.
            It should be avoided for initialization or replacement.
         */
        self& operator []
            ( name_type const& name ///< Name to make primary
            )
        {
            typename Forcing::iterator spot;
            bool status;

            _container.set_primary(_key, name);
            _primary = false;
            // Make a note in the forcing table
            boost::tie(spot, status) = _force.insert(_key);
            if (!status && !util::iequal(_container[_key],name))
                throw std::domain_error((boost::format("Lexicon Initialization Error: More than one primary name set for value '%1%'") % _key).str());
            return *this;
        }

        // Tried adding the sequence operator ("comma operator") but due to confusion
        // with argument lists and low precedence, it's not really of much use.

        /// Set the default key as a value.
        /// @return A reference to @c this object.
        self& default_key
            ( key_type key ///< Default key
            )
        {
            _container.set_default_key(key);
            return *this;
        }

        /// Set the default key as a functor.
        /// @return A reference to @c this object.
        self& default_key
            ( key_generator const& f ///< Handler
            )
        {
            _container.set_default_key(f);
            return *this;
        }

        /// Set the default name as a value.
        /// @return A reference to @c this object.
        self& default_name
            ( name_type const& name ///< Default name
            )
        {
            _container.set_default_name(name);
            return *this;
        }

        /// Set the default name as a value.
        /// @note Convenience overload.
        /// @return A reference to @c this object.
        self& default_name
            ( char const* name ///< Default name
            )
        {
            _container.set_default_name(name);
            return *this;
        }

        /// Set the default name as a functor.
        /// @return A reference to @c this object.
        self& default_name
            ( name_generator const& f ///< Handler
            )
        {
            _container.set_default_name(f);
            return *this;
        }

    private:
        key_type _key; ///< Current key
        bool _primary; ///< Set next name to primary
        lexicon _container; ///< Storage for accumulated key / name pairs
        Forcing _force; ///< Names to force as primaries (out of order).

        friend class lexicon;
    };

    /// Default constructor.
    lexicon() : _auto_sort(false), _sorted(true) {}
    /// Construct from initialization structure.
    lexicon(init& d)
        : _auto_sort(false)
    {
        this->load_from_init(d);
    }

    self& set_auto_sort(bool flag = true) { _auto_sort = flag; return *this; }

    /** Assign from initialization structure.
        Current contents are discarded and replaced with the contents of @a d.
        @return A reference to @c this object.
     */
    self& operator = (init& d)
    {
        this->load_from_init(d);
        return *this;
    }

    /** Add contents of initialization structure.
        @return A reference to @c this object.
     */
    self& operator += (init const& d)
    {
        BOOST_FOREACH(value_type const& item, d._container) {
            if (item._primary
                && ( d._force.end() != d._force.find(item._key) // marked as forced
                   || ! this->has_primary(item._key) // value wasn't already there
               ))
            {
                this->set_primary(item._key, item._name);
            } else {
                this->define(item._key, item._name);
            }
        }
        return *this;
    }

    /// Test if a @a name is defined.
    /// @return @c true if @a name is defined for any key in this @c Lexicon.
    bool contains
        (name_type const& name ///< Name to check
        )
    {
        name_index nidx = this->by_name();
        return nidx.end() != nidx.find(name);
    }

    /// Get number of defined names.
    size_t size() const { return this->by_key().size(); }
    /// Get the number of defined names.
    size_t count() const { return this->size(); }
    /// Get the number of defined names for @a key.
    size_t count(key_type key) const { return _container.count(key); }

    /// Test if a name is defined for @a key.
    /// @return @c true if a name is defined for @a key, @c false otherwise.
    bool contains
        ( key_type key ///< Search key
        )
    {
        key_index kidx = this->by_key();
        return kidx.end() != this->by_key().find(key);
    }

    /** Define a @a name for a @a key.
        @return A reference to @c this object.
        @throw std::domain_error If @a name is already defined for another key.
        @note If @a name is already associated with @a value the request is silently ignored.
        Because a lexicon is case insensitive, this means that the case of a name cannot be
        changed by redefining it.
     */
    self& define
        ( key_type key ///< Target key
        , name_type const& name ///< Associated name
        )
    {
        iterator spot;
        bool status;
        boost::tie(spot, status) = _container.insert(value_type(key, name));
        if (!status) {
            key_index kidx = this->by_key();
            if (spot != kidx.end()) {
                if (spot->_key != key)
                    throw std::domain_error((boost::format("Lexicon Error:failed to define '%1%' for value '%2%' because it is already defined for value '%3%'") % name % key % spot->_key).str());
                // else we're just defining the same name/value pair, so it's OK.
            } else {
                throw std::domain_error((boost::format("Lexicon Internal Error: failed to define '%1%' for value '%2%' for unknown reason") % name % key).str());
            }
        }
        _sorted = false;
        return *this;
    }

    /** Set @a name as the primary for @a key.
        If @a name is not already associated with @a key it becomes associated.
        @a name is then promoted to be the primary for @a key.
        @return A reference to @c this object.
        @throw std::domain_error If @a name is defined for any other key.
     */
    self& set_primary
        ( key_type key ///< Key
        , name_type const& name ///< Primary name
        )
    {
        this->define(key, name); // force the name to be defined for the key
        key_index kidx = this->by_key(); // Can't use "this" in second argument of BOOST_FOREACH
        // Set the primary by clearing all the flags and then setting the target.
        // Searching for the current primary isn't going to be much faster and this is
        // more robust. Moreover, we do the find to avoid case sensitivity problems
        // (although we could get around that by using the strcmp.h header).
        BOOST_FOREACH(value_type const& item, kidx.equal_range(key))
           item._primary = false;
        // set the target primary flag
        this->by_name().find(name)->_primary = true;
        return *this;
    }

    /// Remove a name.
    /// If this is the only name for a key, the key is removed as well.
    /// If this is the primary name for a key, another name is selected
    /// arbitrarily to be the primary name for that key.
    /// @return @c true if the name was present, @c false if it was not defined.
    bool undefine
        ( name_type const& name ///< Name to remove
        )
    {
        bool zret = false;
        name_index nidx = this->by_name();
        name_iterator spot = nidx.find(name);

        // Need to do fix up if we are deleting the primary for the value
        // and it's not the only name for the value.
        if (spot != nidx.end()) {
            key_type key = spot->_key; // cache the value before it disappears
            key_iterator l,r; // working with set of names for a value

            zret = true; // let caller know we deleted something
            nidx.erase(spot);

            // Find the set of names for the value.
            boost::tie(l,r) = this->by_key().equal_range(key);
            // Set a primary if there are still names and none of them are primary
            if (l != r && r == std::find_if(l, r, boost::bind(&value_type::_primary, _1))) {
                l->_primary = true;
            }
            _sorted = false;
        }
        return zret;
    }

    /// Remove all names for @a key.
    /// This also removes the key from the lexicon.
    /// @return @c true if at least one name was defined for @a key, @c false otherwise.
    bool undefine
        ( key_type key
        )
    {
        _sorted = false;
        return this->by_key().erase(key) != 0;
    }

    /// @name Iteration
    /// Iteration in a @c Lexicon iterates over the defined names. The @c value_type
    /// contains the name, the key with which the name is associated, and a flag to
    /// indicate whether the association is the primary association. The iteration
    /// groups all names for the same key together, but no ordering is guaranteed
    /// between key or for names associated with the same key.
    ///
    /// All iteration is effectively constant, as @c value_type does not provide a
    /// public API that permits modification.
    //@{
    /// Iterator for name / value associations.
    typedef key_iterator iterator;
    /// Iterator for name / value associations.
    typedef const_key_iterator const_iterator;

    /// Iterator for first association.
    iterator begin() { return this->by_key().begin(); }
    /// Iterator past last association.
    iterator end() { return this->by_key().end(); }
    /// Iterator for first association.
    const_iterator begin() const { return this->by_key().begin(); }
    /// Iterator past last association.
    const_iterator end() const { return this->by_key().end(); }
    //@}

    /** Find the primary name for @a key.
        @return The primary name for @a key.
        @throw std::domain_error If @a key does not have an associated name and no default has been set.
     */
    name_type operator []
        ( key_type key ///< Search key
        )
    {
        iterator spot(this->find(key));
        if (spot == this->end()) {
            if (_default_name.which() == 1)
                return boost::get<name_type>(_default_name);
            else if (_default_name.which() == 2)
                return boost::get<name_generator>(_default_name)(key);
            else
                throw std::domain_error((boost::format("Lexicon Error: no names defined for value '%1%'") % key).str());
        }
        return spot->_name;
    }

    /** Convert @a name to the key with which it is associated.
        @return The key associated with @a name.
        @throw std::domain_error If @a name is not associated with any key and no default has been set.
     */
    key_type operator []
        ( name_type const& name ///< Search string
        )
    {
        iterator spot(this->find(name));
        if (spot == this->end()) {
            if (_default_key.which() == 1)
                return boost::get<key_type>(_default_key);
            else if (_default_key.which() == 2)
                return boost::get<key_generator>(_default_key)(name);
            else
                throw std::domain_error((boost::format("Lexicon Error: use of undefined name '%1%'") % name).str());
        }
        return spot->_key;
    }

    /// Find item for @a name.
    /// @return An @c iterator that is either @c end() or references the @c Item for @a name.
    iterator find
        ( name_type const& name ///< Search value
        )
    {
        name_index nidx = this->by_name();
        name_iterator spot(nidx.find(name));
        return _container.template project<BY_KEY>(spot);
    }

    /// Find item for a @a key.
    /// @return An @c iterator that is @c end() if no name is associated with the @a key
    /// or references an @c Item that is the primary for the @a key.
    const_iterator find
        ( key_type key ///< Search key
        ) const
    {
        const_key_iterator begin, end, spot;
        boost::tie(begin, end) = _container.equal_range(key);
        spot = std::find_if(begin, end, boost::bind(&value_type::_primary, _1));
        return spot == end ? this->end() : spot;
    }

    /// Get the @a n th name.
    name_type const& name ( size_t n ) const
    {
        const_cast<self*>(this)->auto_sort();
        return _container.template get<BY_POS>().at(n)._name;
    }

    /// Get the key for the @a n th name.
    key_type const& key ( size_t n ) const
    {
        const_cast<self*>(this)->auto_sort();
        return _container.template get<BY_POS>().at(n)._key;
    }

# if 0
    /// Get the index of a key.
    size_t index_of ( key_type key ) const
    {
        Container::index<BY_POS>::type const& pdx = _container.template get<BY_POS>();
        const_iterator spot(this->find(key));
        Container::index<BY_POS>::type::const_iterator p_spot;

        if (spot != this->end()) {
            const_cast<self*>(this)->auto_sort();
            p_spot = _container.template project<BY_POS>(spot);
            if (_auto_sort) {
                while ( p_spot != pdx.begin() && boost::prior(p_spot)->_key == key ) --p_spot;
            }
            return p_spot - pdx.begin();
        } else {
            throw std::domain_error((boost::format("Lexicon Error: no names defined for value '%1%'") % key).str());
        }
    }
# endif
    size_t lower_index_of ( key_type key) const
    {
        size_t zret = 0;
        if (_auto_sort) {
            const_cast<self*>(this)->auto_sort();
            const_pos_index pdx = _container.template get<BY_POS>();
            typename Container::template index<BY_POS>::type::const_iterator spot = std::lower_bound(pdx.begin(), pdx.end(), key, typename value_type::key_compare());
            zret = spot - pdx.begin();
        }
        return zret;
    }

    size_t upper_index_of ( key_type key) const
    {
        size_t zret = 0;
        if (_auto_sort) {
            const_cast<self*>(this)->auto_sort();
            const_pos_index pdx = _container.template get<BY_POS>();
            typename Container::template index<BY_POS>::type::const_iterator spot = std::upper_bound(pdx.begin(), pdx.end(), key, typename value_type::key_compare());
            zret = spot - pdx.begin();
        }
        return zret;
    }

    /// Set the default name.
    /// @return A reference to @c this object.
    self& set_default_name
        ( name_type const& name ///< Default name.
        )
    {
        _default_name = name;
        return *this;
    }

    /// Set the default name.
    /// @return A reference to @c this object.
    self& set_default_name
        ( char const* name ///< Default name.
        )
    {
        _default_name = name_type(name);
        return *this;
    }

    /// Set the default name formatter.
    /// @return A reference to @c this object.
    self& set_default_name
        ( name_generator const& f ///< Formatter
        )
    {
        _default_name = f;
        return *this;
    }

    /// Set default key.
    /// @return A reference to @c this object.
    self& set_default_key
        ( key_type key ///< Default value
        )
    {
        _default_key = key;
        return *this;
    }

    /// Set default key handler.
    /// @return A reference to @c this object.
    self& set_default_key
        ( key_generator const& f ///< Value handler.
        )
    {
        _default_key = f;
        return *this;
    }

protected:
    /// Check if a primary is defined for a @a key.
    /// Used only internally during updates from an @c init structure.
    bool has_primary(key_type key)
    {
        return this->end() != this->find(key);
    }

    /// Load from an @c init structure.
    void load_from_init
        (init& i ///< Source structure
        )
    {
        // Bring in the underlying container data
        swap(_container, i._container._container);
        // default handlers, if any
        if (0 != i._container._default_key.which())
            _default_key = i._container._default_key;
        if (0 != i._container._default_name.which())
            _default_name = i._container._default_name;

        _sorted = false;
   }
};

}
