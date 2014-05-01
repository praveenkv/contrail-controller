/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#include "bgp/evpn/evpn_route.h"

#include "base/logging.h"
#include "base/task.h"
#include "bgp/bgp_log.h"
#include "control-node/control_node.h"
#include "testing/gunit.h"

using namespace std;

class EvpnPrefixTest : public ::testing::Test {
};

// No dash.
TEST_F(EvpnPrefixTest, ParseType_Error1) {
    boost::system::error_code ec;
    string prefix_str("1+10.1.1.1:65535+00:01:02:03:04:05:06:07:08:09+0");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad type.
TEST_F(EvpnPrefixTest, ParseType_Error2) {
    boost::system::error_code ec;
    string prefix_str("x-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-0");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Out of range type.
TEST_F(EvpnPrefixTest, ParseType_Error3) {
    boost::system::error_code ec;
    string prefix_str("0-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-0");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Out of range type.
TEST_F(EvpnPrefixTest, ParseType_Error4) {
    boost::system::error_code ec;
    string prefix_str("5-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-0");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

class EvpnAutoDiscoveryPrefixTest : public EvpnPrefixTest {
};

TEST_F(EvpnAutoDiscoveryPrefixTest, BuildPrefix) {
    RouteDistinguisher rd(RouteDistinguisher::FromString("10.1.1.1:65535"));
    EthernetSegmentId esi(
        EthernetSegmentId::FromString("00:01:02:03:04:05:06:07:08:09"));

    string temp("1-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536, 4294967295 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        EvpnPrefix prefix(rd, esi, tag);
        string prefix_str = temp + integerToString(tag);
        EXPECT_EQ(prefix_str, prefix.ToString());
        EXPECT_EQ(EvpnPrefix::AutoDiscoveryRoute, prefix.type());
        EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
        EXPECT_EQ("00:01:02:03:04:05:06:07:08:09", prefix.esi().ToString());
        EXPECT_EQ(tag, prefix.tag());
        EXPECT_EQ(Address::UNSPEC, prefix.family());
    }
}

TEST_F(EvpnAutoDiscoveryPrefixTest, ParsePrefix) {
    string temp("1-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536, 4294967295 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        string prefix_str = temp + integerToString(tag);
        boost::system::error_code ec;
        EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
        EXPECT_EQ(0, ec.value());
        EXPECT_EQ(prefix_str, prefix.ToString());
        EXPECT_EQ(EvpnPrefix::AutoDiscoveryRoute, prefix.type());
        EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
        EXPECT_EQ("00:01:02:03:04:05:06:07:08:09", prefix.esi().ToString());
        EXPECT_EQ(tag, prefix.tag());
        EXPECT_EQ(Address::UNSPEC, prefix.family());
    }
}

TEST_F(EvpnAutoDiscoveryPrefixTest, FromProtoPrefix) {
    string temp("1-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536, 4294967295 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        string prefix_str = temp + integerToString(tag);
        boost::system::error_code ec;
        EvpnPrefix prefix1(EvpnPrefix::FromString(prefix_str, &ec));
        EXPECT_EQ(0, ec.value());

        BgpAttr attr;
        uint32_t label1 = 10000;
        BgpProtoPrefix proto_prefix;
        prefix1.BuildProtoPrefix(&attr, label1, &proto_prefix);
        EXPECT_EQ(EvpnPrefix::AutoDiscoveryRoute, proto_prefix.type);
        EXPECT_EQ(EvpnPrefix::min_auto_discovery_route_size * 8,
            proto_prefix.prefixlen);
        EXPECT_EQ(EvpnPrefix::min_auto_discovery_route_size,
            proto_prefix.prefix.size());

        EvpnPrefix prefix2;
        EthernetSegmentId esi2;
        uint32_t label2;
        EXPECT_EQ(0,
            EvpnPrefix::FromProtoPrefix(proto_prefix, &prefix2, &esi2, &label2));
        EXPECT_EQ(prefix1, prefix2);
        EXPECT_EQ(EthernetSegmentId::null_esi, esi2);
        EXPECT_EQ(label1, label2);
    }
}

// No dashes.
TEST_F(EvpnAutoDiscoveryPrefixTest, ParsePrefix_Error1) {
    boost::system::error_code ec;
    string prefix_str("1+10.1.1.1:65535+00:01:02:03:04:05:06:07:08:09+0");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// No dashes after type delimiter.
TEST_F(EvpnAutoDiscoveryPrefixTest, ParsePrefix_Error2) {
    boost::system::error_code ec;
    string prefix_str("1-10.1.1.1:65535+00:01:02:03:04:05:06:07:08:09+0");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad RD.
TEST_F(EvpnAutoDiscoveryPrefixTest, ParsePrefix_Error3) {
    boost::system::error_code ec;
    string prefix_str("1-10.1.1.1:65536-00:01:02:03:04:05:06:07:08:09-0");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// No dashes after RD delimiter.
TEST_F(EvpnAutoDiscoveryPrefixTest, ParsePrefix_Error4) {
    boost::system::error_code ec;
    string prefix_str("1-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09+0");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad ESI.
TEST_F(EvpnAutoDiscoveryPrefixTest, ParsePrefix_Error5) {
    boost::system::error_code ec;
    string prefix_str("1-10.1.1.1:65535-00:01:02:03:04:05:06:07:08-0");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad tag.
TEST_F(EvpnAutoDiscoveryPrefixTest, ParsePrefix_Error6) {
    boost::system::error_code ec;
    string prefix_str("1-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-0x");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

class EvpnMacAdvertisementPrefixTest : public EvpnPrefixTest {
};

TEST_F(EvpnMacAdvertisementPrefixTest, BuildPrefix1) {
    boost::system::error_code ec;
    RouteDistinguisher rd(RouteDistinguisher::FromString("10.1.1.1:65535"));
    MacAddress mac_addr(MacAddress::FromString("11:12:13:14:15:16", &ec));
    Ip4Address ip4_addr;

    string temp1("2-10.1.1.1:65535-");
    string temp2("-11:12:13:14:15:16,0.0.0.0");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        EvpnPrefix prefix;
        if (tag == EvpnPrefix::null_tag) {
            prefix = EvpnPrefix(rd, mac_addr, ip4_addr);
        } else {
            prefix = EvpnPrefix(rd, tag, mac_addr, ip4_addr);
        }
        string prefix_str = temp1 + integerToString(tag) + temp2;
        EXPECT_EQ(prefix_str, prefix.ToString());
        EXPECT_EQ(EvpnPrefix::MacAdvertisementRoute, prefix.type());
        EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
        EXPECT_EQ(tag, prefix.tag());
        EXPECT_EQ("11:12:13:14:15:16", prefix.mac_addr().ToString());
        EXPECT_EQ(Address::UNSPEC, prefix.family());
        EXPECT_EQ("0.0.0.0", prefix.ip_address().to_string());
    }
}

TEST_F(EvpnMacAdvertisementPrefixTest, BuildPrefix2) {
    boost::system::error_code ec;
    RouteDistinguisher rd(RouteDistinguisher::FromString("10.1.1.1:65535"));
    MacAddress mac_addr(MacAddress::FromString("11:12:13:14:15:16", &ec));
    Ip4Address ip4_addr = Ip4Address::from_string("192.1.1.1", ec);

    string temp1("2-10.1.1.1:65535-");
    string temp2("-11:12:13:14:15:16,192.1.1.1");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        EvpnPrefix prefix;
        if (tag == EvpnPrefix::null_tag) {
            prefix = EvpnPrefix(rd, mac_addr, ip4_addr);
        } else {
            prefix = EvpnPrefix(rd, tag, mac_addr, ip4_addr);
        }
        string prefix_str = temp1 + integerToString(tag) + temp2;
        EXPECT_EQ(prefix_str, prefix.ToString());
        EXPECT_EQ(EvpnPrefix::MacAdvertisementRoute, prefix.type());
        EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
        EXPECT_EQ(tag, prefix.tag());
        EXPECT_EQ("11:12:13:14:15:16", prefix.mac_addr().ToString());
        EXPECT_EQ(Address::INET, prefix.family());
        EXPECT_EQ("192.1.1.1", prefix.ip_address().to_string());
    }
}

TEST_F(EvpnMacAdvertisementPrefixTest, BuildPrefix3) {
    boost::system::error_code ec;
    RouteDistinguisher rd(RouteDistinguisher::FromString("10.1.1.1:65535"));
    MacAddress mac_addr(MacAddress::FromString("11:12:13:14:15:16", &ec));
    Ip6Address ip6_addr = Ip6Address::from_string("2001:db8:0:9::1", ec);

    string temp1("2-10.1.1.1:65535-");
    string temp2("-11:12:13:14:15:16,2001:db8:0:9::1");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        EvpnPrefix prefix;
        if (tag == EvpnPrefix::null_tag) {
            prefix = EvpnPrefix(rd, mac_addr, ip6_addr);
        } else {
            prefix = EvpnPrefix(rd, tag, mac_addr, ip6_addr);
        }
        string prefix_str = temp1 + integerToString(tag) + temp2;
        EXPECT_EQ(prefix_str, prefix.ToString());
        EXPECT_EQ(EvpnPrefix::MacAdvertisementRoute, prefix.type());
        EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
        EXPECT_EQ(tag, prefix.tag());
        EXPECT_EQ("11:12:13:14:15:16", prefix.mac_addr().ToString());
        EXPECT_EQ(Address::INET6, prefix.family());
        EXPECT_EQ("2001:db8:0:9::1", prefix.ip_address().to_string());
    }
}

TEST_F(EvpnMacAdvertisementPrefixTest, ParsePrefix1) {
    string temp1("2-10.1.1.1:65535-");
    string temp2("-11:12:13:14:15:16,0.0.0.0");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        boost::system::error_code ec;
        string prefix_str = temp1 + integerToString(tag) + temp2;
        EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
        EXPECT_EQ(0, ec.value());
        EXPECT_EQ(prefix_str, prefix.ToString());
        EXPECT_EQ(EvpnPrefix::MacAdvertisementRoute, prefix.type());
        EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
        EXPECT_EQ(tag, prefix.tag());
        EXPECT_EQ("11:12:13:14:15:16", prefix.mac_addr().ToString());
        EXPECT_EQ(Address::UNSPEC, prefix.family());
        EXPECT_EQ("0.0.0.0", prefix.ip_address().to_string());
    }
}

TEST_F(EvpnMacAdvertisementPrefixTest, ParsePrefix2) {
    string temp1("2-10.1.1.1:65535-");
    string temp2("-11:12:13:14:15:16,192.1.1.1");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        boost::system::error_code ec;
        string prefix_str = temp1 + integerToString(tag) + temp2;
        EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
        EXPECT_EQ(0, ec.value());
        EXPECT_EQ(prefix_str, prefix.ToString());
        EXPECT_EQ(EvpnPrefix::MacAdvertisementRoute, prefix.type());
        EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
        EXPECT_EQ(tag, prefix.tag());
        EXPECT_EQ("11:12:13:14:15:16", prefix.mac_addr().ToString());
        EXPECT_EQ(Address::INET, prefix.family());
        EXPECT_EQ("192.1.1.1", prefix.ip_address().to_string());
    }
}

TEST_F(EvpnMacAdvertisementPrefixTest, ParsePrefix3) {
    string temp1("2-10.1.1.1:65535-");
    string temp2("-11:12:13:14:15:16,2001:db8:0:9::1");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        boost::system::error_code ec;
        string prefix_str = temp1 + integerToString(tag) + temp2;
        EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
        EXPECT_EQ(0, ec.value());
        EXPECT_EQ(prefix_str, prefix.ToString());
        EXPECT_EQ(EvpnPrefix::MacAdvertisementRoute, prefix.type());
        EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
        EXPECT_EQ(tag, prefix.tag());
        EXPECT_EQ("11:12:13:14:15:16", prefix.mac_addr().ToString());
        EXPECT_EQ(Address::INET6, prefix.family());
        EXPECT_EQ("2001:db8:0:9::1", prefix.ip_address().to_string());
    }
}

TEST_F(EvpnMacAdvertisementPrefixTest, FromProtoPrefix1) {
    string temp1("2-10.1.1.1:65535-");
    string temp2("-11:12:13:14:15:16,0.0.0.0");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        string prefix_str = temp1 + integerToString(tag) + temp2;
        boost::system::error_code ec;
        EvpnPrefix prefix1(EvpnPrefix::FromString(prefix_str, &ec));
        EXPECT_EQ(0, ec.value());

        BgpAttr attr;
        uint32_t label1 = 10000;
        BgpProtoPrefix proto_prefix;
        EthernetSegmentId esi1 =
            EthernetSegmentId::FromString("00:01:02:03:04:05:06:07:08:09");
        attr.set_esi(esi1);
        prefix1.BuildProtoPrefix(&attr, label1, &proto_prefix);
        EXPECT_EQ(EvpnPrefix::MacAdvertisementRoute, proto_prefix.type);
        EXPECT_EQ(EvpnPrefix::min_mac_advertisment_route_size * 8,
            proto_prefix.prefixlen);
        EXPECT_EQ(EvpnPrefix::min_mac_advertisment_route_size,
            proto_prefix.prefix.size());

        EvpnPrefix prefix2;
        EthernetSegmentId esi2;
        uint32_t label2;
        EXPECT_EQ(0,
            EvpnPrefix::FromProtoPrefix(proto_prefix, &prefix2, &esi2, &label2));
        EXPECT_EQ(prefix1, prefix2);
        EXPECT_TRUE(prefix2.esi().IsNull());
        EXPECT_EQ(esi1, esi2);
        EXPECT_EQ(label1, label2);
    }
}

TEST_F(EvpnMacAdvertisementPrefixTest, FromProtoPrefix2) {
    string temp1("2-10.1.1.1:65535-");
    string temp2("-11:12:13:14:15:16,192.1.1.1");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        string prefix_str = temp1 + integerToString(tag) + temp2;
        boost::system::error_code ec;
        EvpnPrefix prefix1(EvpnPrefix::FromString(prefix_str, &ec));
        EXPECT_EQ(0, ec.value());

        BgpAttr attr;
        uint32_t label1 = 10000;
        BgpProtoPrefix proto_prefix;
        EthernetSegmentId esi1 =
            EthernetSegmentId::FromString("00:01:02:03:04:05:06:07:08:09");
        attr.set_esi(esi1);
        prefix1.BuildProtoPrefix(&attr, label1, &proto_prefix);
        EXPECT_EQ(EvpnPrefix::MacAdvertisementRoute, proto_prefix.type);
        EXPECT_EQ((EvpnPrefix::min_mac_advertisment_route_size + 4) * 8,
            proto_prefix.prefixlen);
        EXPECT_EQ(EvpnPrefix::min_mac_advertisment_route_size + 4,
            proto_prefix.prefix.size());

        EvpnPrefix prefix2;
        EthernetSegmentId esi2;
        uint32_t label2;
        EXPECT_EQ(0,
            EvpnPrefix::FromProtoPrefix(proto_prefix, &prefix2, &esi2, &label2));
        EXPECT_EQ(prefix1, prefix2);
        EXPECT_TRUE(prefix2.esi().IsNull());
        EXPECT_EQ(esi1, esi2);
        EXPECT_EQ(label1, label2);
    }
}

TEST_F(EvpnMacAdvertisementPrefixTest, FromProtoPrefix3) {
    string temp1("2-10.1.1.1:65535-");
    string temp2("-11:12:13:14:15:16,2001:db8:0:9::1");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        string prefix_str = temp1 + integerToString(tag) + temp2;
        boost::system::error_code ec;
        EvpnPrefix prefix1(EvpnPrefix::FromString(prefix_str, &ec));
        EXPECT_EQ(0, ec.value());

        BgpAttr attr;
        uint32_t label1 = 10000;
        BgpProtoPrefix proto_prefix;
        EthernetSegmentId esi1 =
            EthernetSegmentId::FromString("00:01:02:03:04:05:06:07:08:09");
        attr.set_esi(esi1);
        prefix1.BuildProtoPrefix(&attr, label1, &proto_prefix);
        EXPECT_EQ(EvpnPrefix::MacAdvertisementRoute, proto_prefix.type);
        EXPECT_EQ((EvpnPrefix::min_mac_advertisment_route_size + 16) * 8,
            proto_prefix.prefixlen);
        EXPECT_EQ(EvpnPrefix::min_mac_advertisment_route_size + 16,
            proto_prefix.prefix.size());

        EvpnPrefix prefix2;
        EthernetSegmentId esi2;
        uint32_t label2;
        EXPECT_EQ(0,
            EvpnPrefix::FromProtoPrefix(proto_prefix, &prefix2, &esi2, &label2));
        EXPECT_EQ(prefix1, prefix2);
        EXPECT_TRUE(prefix2.esi().IsNull());
        EXPECT_EQ(esi1, esi2);
        EXPECT_EQ(label1, label2);
    }
}

// No dashes.
TEST_F(EvpnMacAdvertisementPrefixTest, ParsePrefix_Error1) {
    boost::system::error_code ec;
    string prefix_str("2+10.1.1.1:65535+65536+11:12:13:14:15:16,192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// No dashes after type delimiter.
TEST_F(EvpnMacAdvertisementPrefixTest, ParsePrefix_Error2) {
    boost::system::error_code ec;
    string prefix_str("2-10.1.1.1:65535+65536+11:12:13:14:15:16,192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad RD.
TEST_F(EvpnMacAdvertisementPrefixTest, ParsePrefix_Error3) {
    boost::system::error_code ec;
    string prefix_str("2-10.1.1.1:65536-65536-11:12:13:14:15:16,192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// No dashes after RD delimiter.
TEST_F(EvpnMacAdvertisementPrefixTest, ParsePrefix_Error4) {
    boost::system::error_code ec;
    string prefix_str("2-10.1.1.1:65535-65536+11:12:13:14:15:16,192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad tag.
TEST_F(EvpnMacAdvertisementPrefixTest, ParsePrefix_Error5) {
    boost::system::error_code ec;
    string prefix_str("2-10.1.1.1:65535-65536x-11:12:13:14:15:16,192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// No comma.
TEST_F(EvpnMacAdvertisementPrefixTest, ParsePrefix_Error6) {
    boost::system::error_code ec;
    string prefix_str("2-10.1.1.1:65535-65536-11:12:13:14:15:16+192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad MAC.
TEST_F(EvpnMacAdvertisementPrefixTest, ParsePrefix_Error7) {
    boost::system::error_code ec;
    string prefix_str("2-10.1.1.1:65535-0-11:12:13:14:15,192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad IP address.
TEST_F(EvpnMacAdvertisementPrefixTest, ParsePrefix_Error8) {
    boost::system::error_code ec;
    string prefix_str("2-10.1.1.1:65535-0-11:12:13:14:15:16,192.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

class EvpnInclusiveMulticastPrefixTest : public EvpnPrefixTest {
};

TEST_F(EvpnInclusiveMulticastPrefixTest, BuildPrefix) {
    boost::system::error_code ec;
    RouteDistinguisher rd(RouteDistinguisher::FromString("10.1.1.1:65535"));
    Ip4Address ip4_addr = Ip4Address::from_string("192.1.1.1", ec);

    string temp1("3-10.1.1.1:65535-");
    string temp2("-192.1.1.1");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        EvpnPrefix prefix(rd, tag, ip4_addr);
        string prefix_str = temp1 + integerToString(tag) + temp2;
        EXPECT_EQ(prefix_str, prefix.ToString());
        EXPECT_EQ(EvpnPrefix::InclusiveMulticastRoute, prefix.type());
        EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
        EXPECT_EQ(tag, prefix.tag());
        EXPECT_EQ(Address::INET, prefix.family());
        EXPECT_EQ("192.1.1.1", prefix.ip_address().to_string());
    }
}

TEST_F(EvpnInclusiveMulticastPrefixTest, ParsePrefix) {
    string temp1("3-10.1.1.1:65535-");
    string temp2("-192.1.1.1");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        boost::system::error_code ec;
        string prefix_str = temp1 + integerToString(tag) + temp2;
        EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
        EXPECT_EQ(0, ec.value());
        EXPECT_EQ(prefix_str, prefix.ToString());
        EXPECT_EQ(EvpnPrefix::InclusiveMulticastRoute, prefix.type());
        EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
        EXPECT_EQ(tag, prefix.tag());
        EXPECT_EQ(Address::INET, prefix.family());
        EXPECT_EQ("192.1.1.1", prefix.ip_address().to_string());
    }
}

TEST_F(EvpnInclusiveMulticastPrefixTest, FromProtoPrefix) {
    string temp1("3-10.1.1.1:65535-");
    string temp2("-192.1.1.1");
    uint32_t tag_list[] = { 0, 100, 128, 4094, 65536, 4294967295 };
    BOOST_FOREACH(uint32_t tag, tag_list) {
        boost::system::error_code ec;
        string prefix_str = temp1 + integerToString(tag) + temp2;
        EvpnPrefix prefix1(EvpnPrefix::FromString(prefix_str, &ec));
        EXPECT_EQ(0, ec.value());

        BgpAttr attr;
        BgpProtoPrefix proto_prefix;
        prefix1.BuildProtoPrefix(&attr, 0, &proto_prefix);
        EXPECT_EQ(EvpnPrefix::InclusiveMulticastRoute, proto_prefix.type);
        EXPECT_EQ((EvpnPrefix::min_inclusive_multicast_route_size + 4) * 8,
            proto_prefix.prefixlen);
        EXPECT_EQ(EvpnPrefix::min_inclusive_multicast_route_size + 4,
            proto_prefix.prefix.size());

        EvpnPrefix prefix2;
        EthernetSegmentId esi2;
        uint32_t label2;
        EXPECT_EQ(0,
            EvpnPrefix::FromProtoPrefix(proto_prefix, &prefix2, &esi2, &label2));
        EXPECT_EQ(prefix1, prefix2);
        EXPECT_EQ(EthernetSegmentId::null_esi, esi2);
        EXPECT_EQ(0, label2);
    }
}

// No dashes.
TEST_F(EvpnInclusiveMulticastPrefixTest, ParsePrefix_Error1) {
    boost::system::error_code ec;
    string prefix_str("3+10.1.1.1:65535+65536+192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// No dashes after type delimiter.
TEST_F(EvpnInclusiveMulticastPrefixTest, ParsePrefix_Error2) {
    boost::system::error_code ec;
    string prefix_str("3-10.1.1.1:65535+65536+192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad RD.
TEST_F(EvpnInclusiveMulticastPrefixTest, ParsePrefix_Error3) {
    boost::system::error_code ec;
    string prefix_str("3-10.1.1.1:65536-65536-192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// No dashes after RD delimiter.
TEST_F(EvpnInclusiveMulticastPrefixTest, ParsePrefix_Error4) {
    boost::system::error_code ec;
    string prefix_str("3-10.1.1.1:65535-65536+192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad tag.
TEST_F(EvpnInclusiveMulticastPrefixTest, ParsePrefix_Error5) {
    boost::system::error_code ec;
    string prefix_str("3-10.1.1.1:65535-65536x-192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad IP address.
TEST_F(EvpnInclusiveMulticastPrefixTest, ParsePrefix_Error6) {
    boost::system::error_code ec;
    string prefix_str("3-10.1.1.1:65535-65536-192.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

class EvpnSegmentPrefixTest : public EvpnPrefixTest {
};

TEST_F(EvpnSegmentPrefixTest, BuildPrefix) {
    boost::system::error_code ec;
    RouteDistinguisher rd(RouteDistinguisher::FromString("10.1.1.1:65535"));
    EthernetSegmentId esi(
        EthernetSegmentId::FromString("00:01:02:03:04:05:06:07:08:09"));
    Ip4Address ip4_addr = Ip4Address::from_string("192.1.1.1", ec);
    EvpnPrefix prefix(rd, esi, ip4_addr);
    EXPECT_EQ("4-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-192.1.1.1",
        prefix.ToString());
    EXPECT_EQ(EvpnPrefix::SegmentRoute, prefix.type());
    EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
    EXPECT_EQ("00:01:02:03:04:05:06:07:08:09", prefix.esi().ToString());
    EXPECT_EQ(EvpnPrefix::null_tag, prefix.tag());
    EXPECT_EQ(Address::INET, prefix.family());
    EXPECT_EQ("192.1.1.1", prefix.ip_address().to_string());
}

TEST_F(EvpnSegmentPrefixTest, ParsePrefix) {
    boost::system::error_code ec;
    string prefix_str(
        "4-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_EQ(0, ec.value());
    EXPECT_EQ("4-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-192.1.1.1",
        prefix.ToString());
    EXPECT_EQ(EvpnPrefix::SegmentRoute, prefix.type());
    EXPECT_EQ("10.1.1.1:65535", prefix.route_distinguisher().ToString());
    EXPECT_EQ("00:01:02:03:04:05:06:07:08:09", prefix.esi().ToString());
    EXPECT_EQ(EvpnPrefix::null_tag, prefix.tag());
    EXPECT_EQ(Address::INET, prefix.family());
    EXPECT_EQ("192.1.1.1", prefix.ip_address().to_string());
}

TEST_F(EvpnSegmentPrefixTest, FromProtoPrefix) {
    boost::system::error_code ec;
    string prefix_str(
        "4-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-192.1.1.1");
    EvpnPrefix prefix1(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_EQ(0, ec.value());

    BgpAttr attr;
    BgpProtoPrefix proto_prefix;
    prefix1.BuildProtoPrefix(&attr, 0, &proto_prefix);
    EXPECT_EQ(EvpnPrefix::SegmentRoute, proto_prefix.type);
    EXPECT_EQ((EvpnPrefix::min_segment_route_size + 4) * 8,
        proto_prefix.prefixlen);
    EXPECT_EQ(EvpnPrefix::min_segment_route_size + 4,
        proto_prefix.prefix.size());

    EvpnPrefix prefix2;
    EthernetSegmentId esi2;
    uint32_t label2;
    EXPECT_EQ(0,
        EvpnPrefix::FromProtoPrefix(proto_prefix, &prefix2, &esi2, &label2));
    EXPECT_EQ(prefix1, prefix2);
    EXPECT_EQ(EthernetSegmentId::null_esi, esi2);
    EXPECT_EQ(0, label2);
}

// No dashes.
TEST_F(EvpnSegmentPrefixTest, ParsePrefix_Error1) {
    boost::system::error_code ec;
    string prefix_str(
        "4+10.1.1.1:65535+00:01:02:03:04:05:06:07:08:09+192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// No dashes after type delimiter.
TEST_F(EvpnSegmentPrefixTest, ParsePrefix_Error2) {
    boost::system::error_code ec;
    string prefix_str(
        "4-10.1.1.1:65535+00:01:02:03:04:05:06:07:08:09+192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad RD.
TEST_F(EvpnSegmentPrefixTest, ParsePrefix_Error3) {
    boost::system::error_code ec;
    string prefix_str(
        "4-10.1.1.1:65536-00:01:02:03:04:05:06:07:08:09-192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// No dashes after RD delimiter.
TEST_F(EvpnSegmentPrefixTest, ParsePrefix_Error4) {
    boost::system::error_code ec;
    string prefix_str(
        "4-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09+192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad ESI.
TEST_F(EvpnSegmentPrefixTest, ParsePrefix_Error5) {
    boost::system::error_code ec;
    string prefix_str(
        "4-10.1.1.1:65535-00:01:02:03:04:05:06:07:08-192.1.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

// Bad IP address.
TEST_F(EvpnSegmentPrefixTest, ParsePrefix_Error6) {
    boost::system::error_code ec;
    string prefix_str(
        "4-10.1.1.1:65535-00:01:02:03:04:05:06:07:08:09-192.1.1");
    EvpnPrefix prefix(EvpnPrefix::FromString(prefix_str, &ec));
    EXPECT_NE(0, ec.value());
}

int main(int argc, char **argv) {
    bgp_log_test::init();
    ::testing::InitGoogleTest(&argc, argv);
    ControlNode::SetDefaultSchedulingPolicy();
    int result = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return result;
}
