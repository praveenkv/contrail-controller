/*
 * Copyright (c) 2014 Juniper Networks, Inc. All rights reserved.
 */

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/stat.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include <db/db.h>
#include <db/db_graph.h>
#include <base/logging.h>

#include <ksync/ksync_index.h>
#include <ksync/ksync_entry.h>
#include <ksync/ksync_object.h>

#include <vnc_cfg_types.h> 
#include <bgp_schema_types.h>
#include <pugixml/pugixml.hpp>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_trace.h>

#include <cmn/agent_cmn.h>
#include <cmn/agent_factory.h>
#include <cfg/cfg_init.h>
#include <cfg/cfg_mirror.h>
#include <cfg/discovery_agent.h>

#include <oper/operdb_init.h>
#include <oper/vrf.h>
#include <oper/multicast.h>
#include <oper/mirror_table.h>
#include <controller/controller_init.h>
#include <controller/controller_vrf_export.h>
#include <cmn/agent_param.h>

#include <vxlan_agent/ksync_vxlan.h>
#include <vxlan_agent/ksync_vxlan_bridge.h>
#include <vxlan_agent/ksync_vxlan_route.h>
#include <vxlan_agent/ksync_vxlan_port.h>

#include "linux_vxlan.h"
#include "linux_bridge.h"
#include "linux_port.h"
#include "linux_fdb.h"

KSyncLinuxVxlan::KSyncLinuxVxlan(Agent *agent) : KSyncVxlan(agent) {
    set_bridge_obj(new KSyncLinuxBridgeObject(this));
    set_port_obj(new KSyncLinuxPortObject(this));
    set_vrf_obj(new KSyncLinuxVrfObject(this));
}

void KSyncLinuxVxlan::Init() { 
}

static void Execute(const string &str) {
    //system(str.c_str());
    cout << str << endl;
}

/****************************************************************************
 ****************************************************************************/
KSyncLinuxBridgeEntry::KSyncLinuxBridgeEntry(KSyncLinuxBridgeObject *obj,
                                             const VxLanId *vxlan) :
    KSyncVxlanBridgeEntry(obj, vxlan) {
    std::stringstream s;
    s << "br-" << vxlan_id();
    name_ = s.str();
}

KSyncLinuxBridgeEntry::KSyncLinuxBridgeEntry
(KSyncLinuxBridgeObject *obj, const KSyncLinuxBridgeEntry *entry) :
    KSyncVxlanBridgeEntry(obj, entry) { 
    std::stringstream s;
    s << "br-" << vxlan_id();
    name_ = s.str();
}

bool KSyncLinuxBridgeEntry::Add() {
    std::stringstream s;
    s << "brctl addbr " << name_;
    Execute(s.str());

    s.str("");
    s << "ip link add " << name_ << " type vxlan id " << vxlan_id();
    Execute(s.str());
    return true;
}

bool KSyncLinuxBridgeEntry::Change() {
    return true;
}

bool KSyncLinuxBridgeEntry::Delete() {
    std::stringstream s;
    s << "ip link del " << name_ << " type vxlan id " << vxlan_id();
    Execute(s.str());

    s.str("");
    s << " brctl delbr " << name_;
    Execute(s.str());
    return true;
}

/****************************************************************************
 ****************************************************************************/
KSyncLinuxBridgeObject::KSyncLinuxBridgeObject(KSyncLinuxVxlan *ksync) : 
    KSyncVxlanBridgeObject(ksync) {
}

KSyncEntry *KSyncLinuxBridgeObject::Alloc(const KSyncEntry *entry,
                                          uint32_t index) {
    const KSyncLinuxBridgeEntry *bridge =
        static_cast<const KSyncLinuxBridgeEntry *>(entry);
    return new KSyncLinuxBridgeEntry(this, bridge);
}

KSyncEntry *KSyncLinuxBridgeObject::DBToKSyncEntry(const DBEntry *e) {
    const VxLanId *vxlan = static_cast<const VxLanId *>(e);
    return static_cast<KSyncEntry *>(new KSyncLinuxBridgeEntry(this, vxlan));
}

/****************************************************************************
 ****************************************************************************/
KSyncLinuxPortEntry::KSyncLinuxPortEntry(KSyncLinuxPortObject *obj,
                                         const Interface *interface) :
    KSyncVxlanPortEntry(obj, interface), old_bridge_(NULL) {
}

KSyncLinuxPortEntry::KSyncLinuxPortEntry(KSyncVxlanPortObject *obj,
                                         const KSyncLinuxPortEntry *entry) :
    KSyncVxlanPortEntry(obj, entry), old_bridge_(NULL) {
}

bool KSyncLinuxPortEntry::Add() { 
    const KSyncLinuxBridgeEntry *br =
        dynamic_cast<const KSyncLinuxBridgeEntry *>(bridge());

    if (br == old_bridge_) {
        return true;
    }

    std::stringstream s;
    if (old_bridge_) {
        s << "ip link del " << old_bridge_->name() << " dev " << port_name();
        Execute(s.str());
    }

    if (br) {
        s.str("");
        old_bridge_ = br;
        s << "ip link add " << br->name() << " dev " << port_name();
        Execute(s.str());
    }
    return true;
}

bool KSyncLinuxPortEntry::Change() { 
    return Add();
}

bool KSyncLinuxPortEntry::Delete() {
    if (old_bridge_ == NULL)
        return true;

    std::stringstream s;
    s << "ip link del " << old_bridge_->name() << " dev " << port_name();
    Execute(s.str());
    return true;
}

/****************************************************************************
 ****************************************************************************/
KSyncLinuxPortObject::KSyncLinuxPortObject(KSyncLinuxVxlan *ksync) : 
    KSyncVxlanPortObject(ksync) {
}

KSyncEntry *KSyncLinuxPortObject::Alloc(const KSyncEntry *entry,
                                        uint32_t index) {
    const KSyncLinuxPortEntry *interface = 
        static_cast<const KSyncLinuxPortEntry *>(entry);
    return new KSyncLinuxPortEntry(this, interface);
}

KSyncEntry *KSyncLinuxPortObject::DBToKSyncEntry(const DBEntry *e) {
    const Interface *interface = static_cast<const Interface *>(e);

    switch (interface->type()) {
    case Interface::PHYSICAL:
    case Interface::VM_INTERFACE:
        return new KSyncLinuxPortEntry(this, interface);
        break;

    default:
        assert(0);
        break;
    }
    return NULL;
}

/****************************************************************************
 ****************************************************************************/
KSyncLinuxFdbEntry::KSyncLinuxFdbEntry(KSyncLinuxFdbObject *obj,
                                       const KSyncLinuxFdbEntry *entry) :
    KSyncVxlanFdbEntry(obj, entry) {
}

KSyncLinuxFdbEntry::KSyncLinuxFdbEntry(KSyncLinuxFdbObject *obj,
                                       const Layer2RouteEntry *route) :
    KSyncVxlanFdbEntry(obj, route) {
}

KSyncLinuxFdbEntry::~KSyncLinuxFdbEntry() {
}

static void MacToStr(char *buff, const struct ether_addr &mac) {
    sprintf(buff, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac.ether_addr_octet[0],
            mac.ether_addr_octet[1],
            mac.ether_addr_octet[2],
            mac.ether_addr_octet[3],
            mac.ether_addr_octet[4],
            mac.ether_addr_octet[5]);
}

bool KSyncLinuxFdbEntry::Add() {
    const KSyncLinuxBridgeEntry *br =
        dynamic_cast<const KSyncLinuxBridgeEntry *>(bridge());

    char buff[64];
    MacToStr(buff, mac());

    std::stringstream s;
    s << "bridge fdb add " << buff << " dst " << tunnel_dest() << " dev "
        << (br ? br->name() : "<bridge-null>");
    Execute(s.str());
    return true;
}

bool KSyncLinuxFdbEntry::Change() {
    return Add();
}

bool KSyncLinuxFdbEntry::Delete() {
    const KSyncLinuxBridgeEntry *br =
        dynamic_cast<const KSyncLinuxBridgeEntry *>(bridge());

    char buff[64];
    MacToStr(buff, mac());

    std::stringstream s;
    s << "bridge fdb del " << buff << " dev "
        << (br ? br->name() : "<bridge-null>");
    Execute(s.str());
    return true;
}

/****************************************************************************
 ****************************************************************************/
KSyncLinuxFdbObject::KSyncLinuxFdbObject(KSyncLinuxVrfObject *vrf,
                                         AgentRouteTable *rt_table) :
    KSyncVxlanRouteObject(vrf, rt_table) {
}

KSyncLinuxFdbObject::~KSyncLinuxFdbObject() {
}

KSyncEntry *KSyncLinuxFdbObject::Alloc(const KSyncEntry *entry,
                                       uint32_t index) {
    const KSyncLinuxFdbEntry *fdb =
        static_cast<const KSyncLinuxFdbEntry *>(entry);
    return new KSyncLinuxFdbEntry(this, fdb);
}

KSyncEntry *KSyncLinuxFdbObject::DBToKSyncEntry(const DBEntry *e) {
    const Layer2RouteEntry *fdb = static_cast<const Layer2RouteEntry *>(e);
    return new KSyncLinuxFdbEntry(this, fdb);
}

/****************************************************************************
 ****************************************************************************/
KSyncLinuxVrfObject::KSyncLinuxVrfObject(KSyncLinuxVxlan *ksync) : 
    KSyncVxlanVrfObject(ksync) {
}

KSyncLinuxVrfObject::~KSyncLinuxVrfObject() {
}

KSyncVxlanRouteObject *KSyncLinuxVrfObject::AllocLayer2RouteTable
(const VrfEntry *vrf) {
    return new KSyncLinuxFdbObject(this, vrf->GetLayer2RouteTable());
}
