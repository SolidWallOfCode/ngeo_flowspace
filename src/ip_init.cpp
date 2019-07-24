/* Copyright 2005-2014 Network Geographics
 * SPDX-License-Identifier: Apache-2.0
 */
# include "ip_local.hpp"
# include "ip_static.hpp"

# if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable:4075)
#   pragma init_seg(".CRT$XCL1")
#   pragma warning(pop)
# endif

namespace ngeo {

ip_port const ip_port::MIN(std::numeric_limits<ip_port::host_type>::min());
ip_port const ip_port::MAX(std::numeric_limits<ip_port::host_type>::max());
/* ------------------------------------------------------------------------ */
ip4_addr const ip4_addr::MIN(std::numeric_limits<host_type>::min());
ip4_addr const ip4_addr::MAX(std::numeric_limits<host_type>::max());
/* ------------------------------------------------------------------------ */
icmp_type const icmp_type::INVALID(-1);
icmp_type const icmp_type::ECHO_REPLY(0);
icmp_type const icmp_type::UNREACHABLE(3);
icmp_type const icmp_type::SOURCE_QUENCH(4);
icmp_type const icmp_type::REDIRECT(5);
icmp_type const icmp_type::ALTERNATE_ADDRESS(6);
icmp_type const icmp_type::ECHO(8);
icmp_type const icmp_type::ROUTER_ADVERTISEMENT(9);
icmp_type const icmp_type::ROUTER_SOLICITATION(10);
icmp_type const icmp_type::TIME_EXCEEDED(11);
icmp_type const icmp_type::PARAMETER_PROBLEM(12);
icmp_type const icmp_type::TIME_STAMP_REQUEST(13);
icmp_type const icmp_type::TIME_STAMP_REPLY(14);
icmp_type const icmp_type::INFO_REQUEST(15);
icmp_type const icmp_type::INFO_REPLY(16);
icmp_type const icmp_type::ADDR_MASK_REQUEST(17);
icmp_type const icmp_type::ADDR_MASK_REPLY(18);
icmp_type const icmp_type::TRACEROUTE(30);
icmp_type const icmp_type::CONVERSION_ERROR(31);
icmp_type const icmp_type::MOBILE_REDIRECT(32);

icmp_type const icmp_type::MIN(0);
icmp_type const icmp_type::MAX(255);

std::string icmp_type_default_name(icmp_type v)
{
    static std::string const INVALID("INVALID");
    return v.is_valid() ? boost::lexical_cast<std::string>(v.host_order()) : INVALID;
}

icmp_type::lexicon_type ICMP_LEXICON
    (icmp_type::lexicon_type::init
        (icmp_type::ECHO_REPLY, "ECHO_REPLY")("MIN")
        (icmp_type::UNREACHABLE, "UNREACHABLE")
        (icmp_type::SOURCE_QUENCH, "SOURCE_QUENCH")
        (icmp_type::REDIRECT, "REDIRECT")
        (icmp_type::ALTERNATE_ADDRESS, "ALTERNATE_ADDRESS")
        (icmp_type::ECHO, "ECHO")
        (icmp_type::ROUTER_ADVERTISEMENT, "ROUTER_ADVERTISEMENT")
        (icmp_type::ROUTER_SOLICITATION, "ROUTER_SOLICITATION")
        (icmp_type::TIME_EXCEEDED, "TIME_EXCEEDED")
        (icmp_type::PARAMETER_PROBLEM, "PARAMETER_PROBLEM")
        (icmp_type::TIME_STAMP_REQUEST, "TIME_STAMP_REQUEST")
        (icmp_type::TIME_STAMP_REPLY, "TIME_STAMP_REPLY")
        (icmp_type::INFO_REQUEST, "INFO_REQUEST")
        (icmp_type::INFO_REPLY, "INFO_REPLY")
        (icmp_type::ADDR_MASK_REQUEST, "ADDR_MASK_REQUEST")
        (icmp_type::ADDR_MASK_REPLY, "ADDR_MASK_REPLY")
        (icmp_type::TRACEROUTE, "TRACEROUTE")
        (icmp_type::CONVERSION_ERROR, "CONVERSION_ERROR")
        (icmp_type::MOBILE_REDIRECT, "MOBILE_REDIRECT")
//        (icmp_type::MAX, "MAX")
        .default_key(icmp_type::INVALID)
        .default_name(&icmp_type_default_name)
    );

icmp const icmp::MIN(std::numeric_limits<icmp_type>::min(), std::numeric_limits<icmp_code>::min());
icmp const icmp::MAX(std::numeric_limits<icmp_type>::max(), std::numeric_limits<icmp_code>::max());
/* ------------------------------------------------------------------------ */
ip4_protocol const ip4_protocol::ICMP(ip4_protocol::HOST_ICMP);
ip4_protocol const ip4_protocol::TCP(ip4_protocol::HOST_TCP);
ip4_protocol const ip4_protocol::UDP(ip4_protocol::HOST_UDP);
ip4_protocol const ip4_protocol::MIN(ip4_protocol::HOST_MIN);
ip4_protocol const ip4_protocol::MAX(ip4_protocol::HOST_MAX);
ip4_protocol const ip4_protocol::IP(ip4_protocol::HOST_MAX+1);
ip4_protocol const ip4_protocol::INVALID(ip4_protocol::HOST_INVALID);
// Neither of these has ancillary data
ip4_service const ip4_service::MIN(ip_protocol::MIN);
ip4_service const ip4_service::MAX(ip_protocol::MAX);

std::string ip_protocol_default_name(ip_protocol const& p)
{
    static std::string const INVALID("INVALID");
    return p.is_valid() ? boost::lexical_cast<std::string>(p.host_order()) : INVALID;
}

ip_protocol::lexicon_type IP_PROTOCOL_LEXICON
    (ip_protocol::lexicon_type::init
        (ip_protocol::IP, "IP")
        (ip_protocol::TCP, "TCP")
        (ip_protocol::UDP, "UDP")
        (ip_protocol::ICMP, "ICMP")
        (51, "AHP")("AH")
        (88, "EIGRP")
        (50, "ESP")("IPSEC")
        (47, "GRE")("PPTP")
        (56, "ICMP6")
        (2, "IGMP")
        (4, "IPINIP")
        (94, "NOS")
        (89, "OSPF")
        (108, "PCP")
        (103, "PIM")
        (109, "SNP")
        .default_name(&ip_protocol_default_name)
        .default_key(ip_protocol::INVALID)
    );
/* ------------------------------------------------------------------------ */
ip_port ip_port_default_value(std::string const& s)
{
    ip_port zret;
    try {
        ip_port::host_type p = boost::lexical_cast<ip_port::host_type>(s);
        zret.set(p);
    } catch (...) {
    }
    return zret;
}

ip_port::lexicon_type PORT_LEXICON
    (ip_port::lexicon_type::init
        (7, "ECHO")
		(9, "DISCARD")
		(13, "DAYTIME")
		(19, "CHARGEN")
		(20, "FTP-DATA")
        (21, "FTP")
        (22, "SSH")
        (23, "TELNET")
        (25, "SMTP")
        (37, "TIME")
        (42, "NAMESERVER")
        (43, "WHOIS")
        (49, "TACACS")
        (53, "DOMAIN")("DNS")
        (67, "BOOTPS")
        (68, "BOOTPC")
        (69, "TFTP")
        (70, "GOPHER")
        (79, "FINGER")
        (80, "HTTP")("WWW")("WEB")
        (90, "DNSIX")
        (101, "HOSTNAME")
        (109, "POP2")
        (110, "POP3")
        (111, "RPC")("SUNRPC")
        (113, "IDENT")
        (119, "NNTP")
        (123, "NTP")
        (137, "NETBIOS-NS")
        (138, "NETBIOS-DGM")
        (139, "NETBIOS-SSN")
        (143, "IMAP")
        (161, "SNMP")
        (162, "SNMPTRAP")
        (177, "XDMP")
        (179, "BGP")
        (194, "IRC")
        (389, "LDAP")
        (434, "MOBILE-IP")
        (443, "HTTPS")
        (445, "SMB")("Microsoft-DS")
        (496, "PIM-AUTO-RP")
        (500, "ISAKMP")
        (512, "BIFF")("EXEC")
        (513, "LOGIN")("WHO")
        (514, "SYSLOG")
        (515, "LDP")
        (517, "TALK")
        (520, "RIP")
        (540, "UUCP")
        (543, "KLOGIN")
        (544, "KSHELL")
        (750, "KERBEROS")
        (1352, "LOTUSNOTES")
        (1494, "CITRIX-ICA")
        (1521, "SQLNET")
        (1645, "RADIUS")
        (1646, "RADIUS-ACCT")
        (1720, "H323")
        (1723, "PPTP")
        (2748, "CTIQBE")
        (5190, "AOL")
        (5510, "SECUREID-UDP")
        (5631, "PCANYWHERE-DATA")
        (5632, "PCANYWHERE-STATUS")
        .default_name(bind(&boost::lexical_cast<std::string, ip_port::host_type>, bind(&ip_port::host_order, _1)))
        .default_key(&ip_port_default_value)
    );
/* ------------------------------------------------------------------------ */
} // namespace

