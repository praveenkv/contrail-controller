/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
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

#include <vnc_cfg_types.h> 
#include <bgp_schema_types.h>
#include <pugixml/pugixml.hpp>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_trace.h>

#include <ksync/ksync_index.h>
#include <ksync/ksync_entry.h>
#include <ksync/ksync_object.h>

#include <cmn/agent_cmn.h>
#include <cmn/agent_factory.h>
#include <cfg/cfg_init.h>
#include <cfg/cfg_mirror.h>

#include <oper/operdb_init.h>
#include <oper/vrf.h>
#include <oper/multicast.h>
#include <oper/mirror_table.h>
#include <controller/controller_init.h>
#include <controller/controller_vrf_export.h>

#include <cmn/agent_param.h>
#include "linux_vxlan_agent_init.h"

#include <vxlan_agent/ksync_vxlan.h>
#include <vxlan_agent/ksync_vxlan_bridge.h>
#include <vxlan_agent/ksync_vxlan_route.h>
#include <vxlan_agent/ksync_vxlan_port.h>

#include "linux_vxlan.h"
#include "linux_bridge.h"
#include "linux_port.h"
#include "linux_fdb.h"

/****************************************************************************
 * Cleanup routines on shutdown
****************************************************************************/
void LinuxVxlanAgentInit::Shutdown() {
}

/****************************************************************************
 * Initialization routines
****************************************************************************/
void LinuxVxlanAgentInit::InitLogging() {
    Sandesh::SetLoggingParams(params_->log_local(),
                              params_->log_category(),
                              params_->log_level());
}

// Connect to collector specified in config, if discovery server is not set
void LinuxVxlanAgentInit::InitCollector() {
    agent_->InitCollector();
}

// Create the basic modules for agent operation.
// Optional modules or modules that have different implementation are created
// by init module
void LinuxVxlanAgentInit::CreateModules() {
    agent_->set_cfg(new AgentConfig(agent_));
    agent_->set_oper_db(new OperDB(agent_));
    agent_->set_controller(new VNController(agent_));
    ksync_vxlan_.reset(new KSyncLinuxVxlan(agent_));
}

void LinuxVxlanAgentInit::CreateDBTables() {
    agent_->CreateDBTables();
}

void LinuxVxlanAgentInit::CreateDBClients() {
    agent_->CreateDBClients();
    ksync_vxlan_->RegisterDBClients(agent_->GetDB());
}

void LinuxVxlanAgentInit::InitPeers() {
    agent_->InitPeers();
}

void LinuxVxlanAgentInit::InitModules() {
    agent_->InitModules();
    ksync_vxlan_->Init();
}

void LinuxVxlanAgentInit::CreateVrf() {
    // Create the default VRF
    VrfTable *vrf_table = agent_->GetVrfTable();

    vrf_table->CreateStaticVrf(agent_->GetDefaultVrf());
    VrfEntry *vrf = vrf_table->FindVrfFromName(agent_->GetDefaultVrf());
    assert(vrf);

    // Default VRF created; create nexthops
    agent_->SetDefaultInet4UnicastRouteTable(vrf->GetInet4UnicastRouteTable());
    agent_->SetDefaultInet4MulticastRouteTable
        (vrf->GetInet4MulticastRouteTable());
    agent_->SetDefaultLayer2RouteTable(vrf->GetLayer2RouteTable());
}

void LinuxVxlanAgentInit::CreateNextHops() {
    DiscardNH::Create();
    ResolveNH::Create();

    DiscardNHKey key;
    NextHop *nh = static_cast<NextHop *>
                (agent_->nexthop_table()->FindActiveEntry(&key));
    agent_->nexthop_table()->set_discard_nh(nh);
}

void LinuxVxlanAgentInit::CreateInterfaces() {
}

void LinuxVxlanAgentInit::InitDiscovery() {
    agent_->cfg()->InitDiscovery();
}

void LinuxVxlanAgentInit::InitDone() {
    RouterIdDepInit(agent_);
    agent_->cfg()->InitDone();
}

void LinuxVxlanAgentInit::InitXenLinkLocalIntf() {
    assert(0);
}

// Start init sequence
bool LinuxVxlanAgentInit::Run() {
    InitLogging();
    InitCollector();
    InitPeers();
    CreateModules();
    CreateDBTables();
    CreateDBClients();
    InitModules();
    CreateVrf();
    CreateNextHops();
    InitDiscovery();
    CreateInterfaces();
    InitDone();

    init_done_ = true;
    return true;
}

void LinuxVxlanAgentInit::Init(AgentParam *param, Agent *agent,
                     const boost::program_options::variables_map &var_map) {
    params_ = param;
    agent_ = agent;
}

// Trigger inititlization in context of DBTable
void LinuxVxlanAgentInit::Start() {
    if (params_->log_file() == "") {
        LoggingInit();
    } else {
        LoggingInit(params_->log_file());
    }

    params_->LogConfig();
    params_->Validate();

    int task_id = TaskScheduler::GetInstance()->GetTaskId("db::DBTable");
    trigger_.reset(new TaskTrigger(boost::bind(&LinuxVxlanAgentInit::Run, this), 
                                   task_id, 0));
    trigger_->Set();
    return;
}
