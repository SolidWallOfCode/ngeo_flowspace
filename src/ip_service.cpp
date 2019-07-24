/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */
/* ------------------------------------------------------------------------ */
# include "ip_local.hpp"
# include "ip_static.hpp"
/* ------------------------------------------------------------------------ */
namespace ngeo { 
using boost::lexical_cast;
/* ------------------------------------------------------------------------ */
ip4_service::nil_type ip4_service::NIL;

ip4_service::ip4_service() {
}

ip4_service::ip4_service(self const& that)
    : _protocol(that._protocol), _data(that._data) {
}

ip4_service::ip4_service(icmp const& msg)
    : _protocol(ip_protocol::ICMP), _data(msg) {
}

ip4_service::ip4_service(icmp_type const& msg)
    : _protocol(ip_protocol::ICMP), _data(icmp(msg)) {
}

ip4_service::ip4_service(ip_protocol const& p)
    : _protocol(p)
{
    switch (p.get_data_type()) {
        case ip4_protocol::DATA_PORT:
            _data = ip_port();
            break;
        case ip4_protocol::DATA_ICMP:
            _data = icmp();
            break;
        case ip_protocol::DATA_NONE: 
            _data = NIL;
            break;            
    }
}

ip4_service::ip4_service(
        ip4_protocol const& proto,
        ip4_port const& port)
    : _protocol(proto), _data(port) {
}

ip4_service & ip4_service::operator = (ip4_service const& that) {
        this->_protocol = that._protocol;
        this->_data = that._data;
        return *this;
}

ip4_service::ip4_service(ip4_protocol const& p, data const& ancillary)
  : _protocol(p), _data(ancillary)  {
}

ip4_service::~ip4_service() {
}
/* ------------------------------------------------------------------------ */
ip4_service::ip4_service(std::string const &str) {
    std::istringstream istr(str);
    istr >> *this;
}
/* ------------------------------------------------------------------------ */
bool
ip4_service::is_valid() const {
    return _protocol.is_valid();
}
/* ------------------------------------------------------------------------ */
icmp const&
ip4_service::get_icmp() const
{
    icmp const* ptr = boost::get<icmp const>(&_data);
    if (!ptr) throw boost::bad_get();
    return *ptr;
}
/* ------------------------------------------------------------------------ */
ip4_port const&
ip4_service::get_port() const
{
    ip4_port const* ptr = boost::get<ip4_port const>(&_data);
    if (!ptr) throw boost::bad_get();
    return *ptr;
}
/* ------------------------------------------------------------------------ */
ip4_service&
ip4_service::operator ++ ()
{
    bool pinc_p = true; // Do we need to increment the protocol value?

    if (this->has_port()) {
        ip4_port port = this->get_port();
        if (port != ip4_port::MAX) {
            this->set_port(++port);
            pinc_p = false;
        }
    } else if (this->has_icmp()) {
        icmp icmpData = this->get_icmp();
        if (icmpData.type() != icmp_type::MAX) {
            this->set_icmp(++icmpData.type()); 
            pinc_p = false;
        }
    }

    if (pinc_p) {
        ++_protocol;
        if (this->has_port())
            this->set_port(ip4_port::MIN);
        else if (this->has_icmp())
            this->set_icmp(icmp_type::MIN);
        else
            _data = NIL;
    }
            
    return *this;
}
/* ------------------------------------------------------------------------ */
ip4_service&
ip4_service::operator -- ()
{
    bool pdec_p = true; // decrement the protocol?

    if (this->has_port()) {
        ip4_port port = this->get_port();
        if (port != ip4_port::MIN) {
            this->set_port(--port);
            pdec_p = false;
        }
    } else if (this->has_icmp()) {
        icmp icmpData = this->get_icmp();
        if (icmpData.type() != icmp_type::MIN) {
            this->set_icmp(icmp(--icmpData.type())); 
            pdec_p = false;
        }
    }

    if (pdec_p) {
        --_protocol;
        if (this->has_port())
            this->set_port(ip4_port::MAX);
        else if (this->has_icmp())
            this->set_icmp(icmp_type::MAX);
        else
            _data = NIL;
    }
    return *this;
}
/* ------------------------------------------------------------------------ */
bool
operator < ( ip4_service const& lhs, ip4_service const& rhs )
{
    return lhs._protocol == rhs._protocol
        ? lhs._data < rhs._data
        : lhs._protocol < rhs._protocol;
        ;
}
/* ------------------------------------------------------------------------ */
bool
operator == ( ip4_service const& lhs, ip4_service const& rhs )
{
    return lhs._protocol == rhs._protocol && lhs._data == rhs._data;
}
/* ------------------------------------------------------------------------ */
ip4_service
ip4_service::minimum_for(ip4_protocol p)
{
    self zret(p);
    switch (p.get_data_type()) {
        case ip4_protocol::DATA_PORT:
            zret._data = ip_port::MIN;
            break;
        case ip4_protocol::DATA_ICMP:
            zret._data = icmp::MIN;
            break;
        case ip4_protocol::DATA_NONE:
        	break;
    }
    return zret;
}
/* ------------------------------------------------------------------------ */
ip4_service
ip4_service::maximum_for(ip4_protocol p)
{
    self zret(p);
    switch (p.get_data_type()) {
        case ip4_protocol::DATA_PORT:
            zret._data = ip_port::MAX;
            break;
        case ip4_protocol::DATA_ICMP:
            zret._data = icmp::MAX;
            break;
        case ip4_protocol::DATA_NONE:
        	break;
    }
    return zret;
}
/* ------------------------------------------------------------------------ */
std::ostream& operator << (
    std::ostream& s,
    ip4_service const& svc
) {
    s << svc.get_protocol();
    ip_protocol::data_type type = svc.get_data_type();
    switch (type) {
        case ip_protocol::DATA_ICMP: s << ':' << svc.get_icmp().type(); break;
        case ip_protocol::DATA_PORT: s << ':' << svc.get_port(); break;
        default: break;
    }
    return s;
}

std::istream& operator >> (
    std::istream& s,
    ip4_service& svc
) {
    ip4_protocol p;
    if (s >> p) {
        if (p.get_data_type() != ip_protocol::DATA_NONE) {
            char c;
            s >> std::ws >> c >> std::ws; // skip trailing colon
            switch (p.get_data_type()) {
                case ip_protocol::DATA_ICMP: {
                    icmp_type it;
                    if (s >> it) svc = ip4_service(icmp(it));
                    break;
                }
                case ip_protocol::DATA_PORT: {
                    ip_port port;
                    if (s >> port) svc = ip4_service(p, port);
                    break;
                }
                default: // Should not get here.
                    assert(false);
                    break;
            }
        } else { // no ancillary data
            if (s && !s.eof() && (':' == s.peek())) s.get(); // drop trailing colon if present
            svc = ip4_service(p);
        }
    }
    return s;
}

/* ------------------------------------------------------------------------ */
} // end namespace ngeo::flowspace
