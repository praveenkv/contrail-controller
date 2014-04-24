/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef ctrlplane_evpn_route_h
#define ctrlplane_evpn_route_h

#include <boost/system/error_code.hpp>

#include "bgp/bgp_route.h"
#include "bgp/inet/inet_route.h"
#include "net/address.h"
#include "net/bgp_af.h"
#include "net/esi.h"
#include "net/mac_address.h"
#include "net/rd.h"

class EvpnPrefix {
public:
    static const EvpnPrefix null_prefix;
    static const uint32_t null_tag;
    static const uint32_t max_tag;

    static const size_t rd_size;
    static const size_t esi_size;
    static const size_t tag_size;
    static const size_t mac_size;
    static const size_t label_size;
    static const size_t min_auto_discovery_route_size;
    static const size_t min_mac_advertisment_route_size;
    static const size_t min_inclusive_multicast_route_size;
    static const size_t min_segment_route_size;

    enum RouteType {
        Unspecified = 0,
        AutoDiscoveryRoute = 1,
        MacAdvertisementRoute = 2,
        InclusiveMulticastRoute = 3,
        SegmentRoute = 4
    };

    EvpnPrefix();
    EvpnPrefix(const RouteDistinguisher &rd, const EthernetSegmentId &esi,
        uint32_t tag);
    EvpnPrefix(const RouteDistinguisher &rd,
        const MacAddress &mac_addr, const IpAddress &ip_address);
    EvpnPrefix(const RouteDistinguisher &rd, uint32_t tag,
        const MacAddress &mac_addr, const IpAddress &ip_address);
    EvpnPrefix(const RouteDistinguisher &rd, uint32_t tag,
        const IpAddress &ip_address);
    EvpnPrefix(const RouteDistinguisher &rd, const EthernetSegmentId &esi,
        const IpAddress &ip_address);

    void BuildProtoPrefix(uint32_t label, BgpProtoPrefix *proto_prefix) const;

    static int FromProtoPrefix(const BgpProtoPrefix &proto_prefix,
        EvpnPrefix *evpn_prefix, uint32_t *label);
    static EvpnPrefix FromString(const std::string &str,
        boost::system::error_code *errorp = NULL);
    std::string ToString() const;

    int CompareTo(const EvpnPrefix &rhs) const;
    bool operator==(const EvpnPrefix &rhs) const { return CompareTo(rhs) == 0; }

    uint8_t type() const { return type_; }
    RouteDistinguisher route_distinguisher() const { return rd_; }
    EthernetSegmentId esi() const { return esi_; }
    uint32_t tag() const { return tag_; }
    MacAddress mac_addr() const { return mac_addr_; }
    Address::Family family() const { return family_; }
    IpAddress ip_address() const { return ip_address_; }
    static size_t label_offset(const BgpProtoPrefix &proto_prefix);

private:
    uint8_t type_;
    RouteDistinguisher rd_;
    EthernetSegmentId esi_;
    uint32_t tag_;
    MacAddress mac_addr_;
    Address::Family family_;
    IpAddress ip_address_;

    static uint32_t ReadLabel(const BgpProtoPrefix &proto_prefix,
        size_t label_offset);
    static void WriteLabel(BgpProtoPrefix *proto_prefix,
        size_t label_offset, uint32_t label);

    size_t GetIpAddressSize() const;
    void ReadIpAddress(const BgpProtoPrefix &proto_prefix,
        size_t ip_size, size_t ip_offset);
    void WriteIpAddress(BgpProtoPrefix *proto_prefix, size_t ip_offset) const;
};

class EvpnRoute : public BgpRoute {
public:
    EvpnRoute(const EvpnPrefix &prefix);
    virtual int CompareTo(const Route &rhs) const;
    virtual std::string ToString() const;

    const EvpnPrefix &GetPrefix() const { return prefix_; }

    virtual KeyPtr GetDBRequestKey() const;
    virtual void SetKey(const DBRequestKey *reqkey);

    virtual void BuildProtoPrefix(BgpProtoPrefix *proto_prefix,
        uint32_t label) const;
    virtual void BuildBgpProtoNextHop(std::vector<uint8_t> &nh,
        IpAddress nexthop) const;

    virtual bool IsLess(const DBEntry &genrhs) const {
        const EvpnRoute &rhs = static_cast<const EvpnRoute &>(genrhs);
        int cmp = CompareTo(rhs);
        return (cmp < 0);
    }

    virtual u_int16_t Afi() const { return BgpAf::L2Vpn; }
    virtual u_int8_t Safi() const { return BgpAf::EVpn; }
    virtual u_int8_t XmppSafi() const { return BgpAf::Enet; }

private:
    EvpnPrefix prefix_;

    DISALLOW_COPY_AND_ASSIGN(EvpnRoute);
};

#endif
