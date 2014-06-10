/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <boost/program_options.hpp>

#include "testing/gunit.h"
#include "test/test_cmn_util.h"

namespace opt = boost::program_options;

void RouterIdDepInit(Agent *agent) {
}

class VmwareTest : public ::testing::Test {
public:
    virtual void SetUp() {
        uint16_t http_server_port = ContrailPorts::HttpPortAgent;

        desc.add_options()
        ("help", "help message")
        ("HYPERVISOR.type", opt::value<string>()->default_value("kvm"), 
         "Type of hypervisor <kvm|xen|vmware>")
        ("HYPERVISOR.vmware_physical_port", opt::value<string>(),
         "Physical port used to connect to VMs in VMWare environment")
        ("HYPERVISOR.vmware_mode",
         opt::value<string>()->default_value("esxi_neutron"), 
         "VMWare mode <esxi_neutron|vcenter>")
            ;

        agent = Agent::GetInstance();
        param = agent->params();

        intf_count_ = agent->GetInterfaceTable()->Size();
        nh_count_ = agent->GetNextHopTable()->Size();
        vrf_count_ = agent->GetVrfTable()->Size();
    }

    virtual void TearDown() {
        WAIT_FOR(100, 1000, (agent->GetInterfaceTable()->Size() == intf_count_));
        WAIT_FOR(100, 1000, (agent->GetVrfTable()->Size() == vrf_count_));
        WAIT_FOR(100, 1000, (agent->GetNextHopTable()->Size() == nh_count_));
        WAIT_FOR(100, 1000, (agent->GetVmTable()->Size() == 0U));
        WAIT_FOR(100, 1000, (agent->GetVnTable()->Size() == 0U));
        var_map.clear();
    }

    AgentParam *param;
    Agent *agent;

    int intf_count_;
    int nh_count_;
    int vrf_count_;

    opt::options_description desc;
    opt::variables_map var_map;
};

// Validate the vmware parameters set in AgentParam
TEST_F(VmwareTest, VmwareParam_1) {
    EXPECT_EQ(param->mode(), AgentParam::MODE_VMWARE);
    EXPECT_STREQ(param->vmware_physical_port().c_str(), "vmport");
}

// Validate creation of VM connecting port
TEST_F(VmwareTest, VmwarePhysicalPort_1) {

    // Validate the both IP Fabric and VM physical interfaces are present
    PhysicalInterfaceKey key(param->vmware_physical_port());
    Interface *intf = static_cast<Interface *>
        (agent->GetInterfaceTable()->Find(&key, false));
    EXPECT_TRUE(intf != NULL);

    PhysicalInterfaceKey key1(agent->GetIpFabricItfName());
    EXPECT_TRUE((agent->GetInterfaceTable()->Find(&key1, false)) != NULL);
}

// Create a VM VLAN interface
TEST_F(VmwareTest, VmwareVmPort_1) {
    struct PortInfo input1[] = {
        {"vnet1", 1, "1.1.1.1", "00:00:00:01:01:01", 1, 1}
    };

    client->Reset();
    IntfCfgAdd(1, "vnet1", "1.1.1.1", 1, 1, "00:00:00:01:01:01", 1);
    CreateVmportEnv(input1, 1);
    client->WaitForIdle();

    if (VmPortGet(1) == NULL) {
        EXPECT_STREQ("Port not created", "");
        return;
    }

    VmInterface *intf = dynamic_cast<VmInterface *>(VmPortGet(1));
    EXPECT_TRUE(intf != NULL);
    if (intf == NULL)
        return;

    EXPECT_TRUE(intf->vlan_id() == 1);
    EXPECT_TRUE(intf->parent() != NULL);

    DeleteVmportEnv(input1, 1, true);
    client->WaitForIdle();
    EXPECT_FALSE(VmPortFind(1));
    client->Reset();
}

// Validate the vmware mode parameters
TEST_F(VmwareTest, VmwareMode_1) {
    AgentParam param(Agent::GetInstance());
    param.Init("controller/src/vnsw/agent/cmn/test/cfg-vmware.ini",
               "test-param", var_map);
    EXPECT_EQ(param.mode(), AgentParam::MODE_VMWARE);
    EXPECT_EQ(param.vmware_mode(), AgentParam::ESXI_NEUTRON);
}

TEST_F(VmwareTest, VmwareMode_2) {
    AgentParam param(Agent::GetInstance());
    param.Init("controller/src/vnsw/agent/cmn/test/cfg-vmware-1.ini",
               "test-param", var_map);
    EXPECT_EQ(param.mode(), AgentParam::MODE_VMWARE);
    EXPECT_EQ(param.vmware_mode(), AgentParam::ESXI_NEUTRON);
}

TEST_F(VmwareTest, VmwareMode_3) {
    AgentParam param(Agent::GetInstance());
    param.Init("controller/src/vnsw/agent/cmn/test/cfg-vmware-2.ini",
               "test-param", var_map);
    EXPECT_EQ(param.mode(), AgentParam::MODE_VMWARE);
    EXPECT_EQ(param.vmware_mode(), AgentParam::VCENTER);
}

TEST_F(VmwareTest, VmwareMode_4) {
    int argc = 5;
    char *argv[] = {
        (char *) "",
        (char *) "--HYPERVISOR.type",    (char *)"vmware",
        (char *) "--HYPERVISOR.vmware_mode", (char *)"esxi_neutron"
    };

    try {
        opt::store(opt::parse_command_line(argc, argv, desc), var_map);
        opt::notify(var_map);
    } catch (...) {
        cout << "Invalid arguments. ";
        cout << desc << endl;
        exit(0);
    }

    AgentParam param(Agent::GetInstance());
    param.Init("controller/src/vnsw/agent/cmn/test/cfg-vmware-2.ini",
               "test-param", var_map);
    EXPECT_EQ(param.mode(), AgentParam::MODE_VMWARE);
    EXPECT_EQ(param.vmware_mode(), AgentParam::ESXI_NEUTRON);
}

TEST_F(VmwareTest, VmwareMode_5) {
    int argc = 5;
    char *argv[] = {
        (char *) "",
        (char *) "--HYPERVISOR.type",    (char *)"vmware",
        (char *) "--HYPERVISOR.vmware_mode", (char *)"vcenter"
    };

    try {
        opt::store(opt::parse_command_line(argc, argv, desc), var_map);
        opt::notify(var_map);
    } catch (...) {
        cout << "Invalid arguments. ";
        cout << desc << endl;
        exit(0);
    }

    AgentParam param(Agent::GetInstance());
    param.Init("controller/src/vnsw/agent/cmn/test/cfg-vmware-1.ini",
               "test-param", var_map);
    EXPECT_EQ(param.mode(), AgentParam::MODE_VMWARE);
    EXPECT_EQ(param.vmware_mode(), AgentParam::VCENTER);
}

int main(int argc, char **argv) {
    GETUSERARGS();
    strcpy(init_file, "controller/src/vnsw/agent/cmn/test/cfg-vmware.ini");

    client = TestInit(init_file, ksync_init);
    int ret = RUN_ALL_TESTS();
    usleep(1000);
    TestShutdown();
    usleep(1000);
    delete client;
    return ret;
}
