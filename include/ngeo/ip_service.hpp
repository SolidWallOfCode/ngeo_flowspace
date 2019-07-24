/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */

/* ------------------------------------------------------------------------ */
# pragma once

/** @file
    IPv4 Services.
    Classes to model services in IPv4.
 */

# include <ngeo/ip_base.hpp>
# include <boost/variant.hpp>
# if !defined(_MSC_VER)
#   include <endian.h>
# endif
/* ------------------------------------------------------------------------ */
# if defined(_MSC_VER)
#   pragma warning(disable:4251)
#   if NG_STATIC
#       define API
#   else
#       if defined(NETWORK_GEOGRAPHICS_IP_API)
#           define API _declspec(dllexport)
#       else
#           define API _declspec(dllimport)
#       endif
#   endif
# else
#   define API
# endif
/* ------------------------------------------------------------------------ */
namespace ngeo { 
/* ------------------------------------------------------------------------ */
class ip4_flow; // forward declare

/** IPv4 Service.
    This models an IPv4 service. It always contains a protocol value (@c ip4_protocol)
    and can contain additional data specific to that service. Such data is referred
    to as "ancillary" data. The services with currently supported ancillary data are
    - TCP (port : @c ip_port)
    - UDP (port : @c ip_port)
    - ICMP (message type : @c icmp)

    This class is @ref totally_ordered.
 */
class API ip4_service
    : public boost::totally_ordered<ip4_service>
{
    /// Ancillary data nil type.
    struct nil_type {
        /// Default constructor.
        /// @internal VC9 whines if we don't do this.
        nil_type() {} ///< Default constructor.
        // Define these to make service operators easier to implement.
        bool operator == (nil_type const&) const { return true; } ///< Always equal.
        bool operator <  (nil_type const&) const { return false; } ///< Never less.
    };
public:
    typedef ip4_service self; //!< Self reference type.
    typedef ip_protocol::data_type data_type; ///< Import for client convenience.

    /** Ancillary data.

        @internal @c nil_type is first so that it is used for default construction.
     */
    typedef boost::variant<
        nil_type, ///< No data.
        icmp,     ///< ICMP data
        ip4_port  ///< TCP, UDP data
    > data;

    /** Default constructor.
        Construct an invalid / empty service.
     */
    ip4_service();

    /** Construct from protocol.
        The ancillary data is left uninitialized.
     */
    ip4_service(
        ip4_protocol const& p //!< Protocol for the service.
    );

    /** Construct from ICMP.
        The protocol is set to @c ip4_protocol::ICMP and the ancillary data
        copied from @a msg.
     */
    ip4_service(
        icmp const& msg //!< ICMP data.
    );

    /** Construct from ICMP message type.
        The protocol is set to @c ip4_protocol::ICMP and the ancillary data
        copied from @a msg.
     */
    ip4_service(
        icmp_type const& msg ///< ICMP message type.
    );
    
    /// Copy constructor.
    ip4_service(
        self const& that  //!< source to clone.
    );

    /** Constructor for TCP/UDP.
        It is an error to call this constructor if @a proto is not
        @c ip4_protocol::TCP nor @c ip4_protocol::UDP.
        @a port is copied to the ancillary data.
     */
    ip4_service(
        ip4_protocol const& proto, ///< UDP or TCP.
        ip4_port const& port ///< Service port.
    );

    /** Construct from string.
        @see operator>>(std::istream&, ip4_service&)
     */
    explicit ip4_service(
        std::string const& str ///< Text description of service
    );

    ~ip4_service();

    /** Assignment operator.
        @return A reference to @c this object.
     */
    ip4_service & operator = (
        ip4_service const& that ///< Source object.
    );

    static self const MIN; //!< Minimum service value.
    static self const MAX; //!< Maximum service value.
    struct static_MIN_MAX_tag; //!< Mark as having built in MIN/MAX members.

    static nil_type NIL; ///< Ancillary data value for protocols with no ancillary data.

    //! User conversion to protocol of service stored in this object.
    operator ip4_protocol const& () const { return _protocol; }
    //! Accessor for protocol of service stored in this object.
    ip4_protocol get_protocol () const { return _protocol; }
    /** Test for internal validity.
        This verifies that the protocol is valid, and that the ancillary data
        corresponds in type to the protocol.
        @return @c true if the validity checks succeed, @c false otherwise.
     */
    bool is_valid() const;

    /** Accessor for raw ancillary data.
        @note This is used primarily internally, although it is made available
        to clients.
     */
    data const& get_data() const { return _data; }

    /** Get the type of ancillary data for the protocol in this service.
     */
    data_type get_data_type() const { return _protocol.get_data_type(); }

    /** Is the ancillary data ICMP data?
        @return @c true if the ancillary data is ICMP data, @c false otherwise.
     */
    bool has_icmp() const {
        return _protocol.get_data_type() == ip_protocol::DATA_ICMP;
    }
    
    /// Does this instance contain ancillary data?
    /// @return @c true if ancillary data, @c false otherwise.
    /// @note This does not check the protocol, but checks the actual ancillary data storage object.
    bool has_ancillary() const {
    	return _protocol.get_data_type() != ip_protocol::DATA_NONE;
    }

    /// Is the ancillary data a port?
    /// @return @c true if the ancillary data is a port, @c false otherwise.
    bool has_port() const {
        return _protocol.get_data_type() == ip4_protocol::DATA_PORT;
    }

    /** Get the ICMP data from the service.
        @throw @c boost::bad_get if the protocol is not @c ip4_protocol::ICMP.
     */
    icmp const& get_icmp () const;

    /** Get the TCP/UDP port data from service.
        @throw @c boost::bad_get if the protocol does not have an @c ip4_port for ancillary data.
     */
    ip4_port const& get_port () const;
    
    /** Set the port value directly.
        @throw @c boost::bad_get if the protocol does not have an @c ip4_port for ancillary data.
     */
    bool set_port(
        ip4_port const &port ///< Port value.
    ) {
        if (_protocol.get_data_type() != ip4_protocol::DATA_PORT) throw boost::bad_get();
        _data = port;
        return true;
    }

    /** Set the ICMP value directory.
        @throw @c boost::bad_get if the protocol does not have an @c icmp for ancillary data.
     */
    bool set_icmp(
        icmp const &i ///< ICMP data.
    ) {
        if (_protocol.get_data_type() != ip4_protocol::DATA_ICMP) throw boost::bad_get();
        _data = i;
        return true;
    }

    /** Test if the service is a specific protocol.
     */
    bool is(
        ip4_protocol const& p //!< Protocol value to check.
    ) const {
        return _protocol == p;
    }
    
    //! Pre-increment.
    self& operator ++  ();
    //! Pre-decrement.
    self& operator --  ();

    /// Equality check.
    friend API bool operator == (
        self const& lhs, ///< Left hand side
        self const& rhs ///< Right hand side
    );
    /// Ordering operator.
    /// This is primarily used for storing services in an STL container. This
    /// imposes a complete ordering on all services.
    /// @return @c true if @a lhs < @a rhs.
    friend API bool operator < (
        self const& lhs, ///< Left hand side
        self const& rhs ///< Right hand side
    );

    //! Generate an instance that has the minimum ancillary data value for the protocol.
    static self minimum_for(
        ip4_protocol //!< Protocol.
    );
    //! Generate an instance that has the maximum ancillary data value for the protocol.
    static self maximum_for(
        ip4_protocol //!< Protocol.
    );

private:
    ip4_protocol _protocol; //!< The service protocol.
    data _data; //!< Ancillary data for protocol.

    /** Privileged constructor.
        This does no checking for protocol / ancillary data consistency and so
        isn't safe for client use.
     */
    ip4_service(
        ip4_protocol const& p, //!< Protocol.
        data const& ancillary //!< Ancillary data for the protocol.
    );

    friend class ip4_flow;
};

/** Read the service from a stream.
    The format is "P:A" where
    - P is the protocol, as recognized by @c ip4_protocol
    - A is the ancillary data, if any

    E.g. "UDP:517", "6:80".

    @return @a s
    @relates ip4_service
    @see ip_protocol, ip_port, icmp
 */
API std::istream& operator >> (
    std::istream& s,         //!< [in,out] Input stream
    ip4_service& svc         //!< [in,out] Service to store in to
);

/** Write the service to a stream.
    @see operator>>(std::istream&, ip4_service&)
    @return @a s
    @relates ip4_service
 */
API std::ostream& operator << (
    std::ostream& s,       //!< [in,out] Output stream
    ip4_service const& svc //!< [in] Service to write from
);

/* ------------------------------------------------------------------------ */
// Not in use.
# if 0
class API ip4_flow
{
public:
    typedef ip4_flow self; //!< Self reference type.
    typedef ip4_service::data data;

    //! Default constructor.
    ip4_flow();

    /** Construct from protocol.
        Any ancillary data is default constructed.
     */
    ip4_flow(
        ip4_protocol const& p //!< Protocol for flow.
        );

    /** Construct from a service.
        The ancillary data for source and destination are the same.
     */
    ip4_flow(
        ip4_service const& srv //!< Service for source and destination.
        ) : m_protocol(srv), m_src(srv.get_data()), m_dst(srv.get_data())
    {
    }

    /** Construct from ICMP.
        Source and destination type / code are set from @a i.
     */
    ip4_flow(
        icmp const& i
        ) : m_protocol(ip4_protocol::ICMP), m_src(i), m_dst(i)
    {
    }

    /** Constructor for TCP/UDP.
     */
    ip4_flow(
        ip4_protocol const& pr, //!< Must be TCP or UDP
        ip4_port const& p //!< Source and destination port.
        );

    /** Constructor to TCP/UDP.
     */
    ip4_flow(
        ip4_protocol const& pr, //!< Must be TCP or UDP
        ip4_port const& src, //!< Source port.
        ip4_port const& dst  //!< Destination port.
        );

    //! Get a service describing the source of the flow.
    ip4_service get_src() const { return ip4_service(m_protocol, m_src); }
    //! Get a service decribing the destination of the flow.
    ip4_service get_dst() const { return ip4_service(m_protocol, m_dst); }

private:
    ip4_protocol m_protocol;
    data m_src;
    data m_dst;
};
# endif
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
} // namespace ngeo::flowspace
# undef API
/* ------------------------------------------------------------------------ */
