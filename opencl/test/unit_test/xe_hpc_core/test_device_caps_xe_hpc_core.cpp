/*
 * Copyright (C) 2021-2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/helpers/hw_helper.h"
#include "shared/test/common/test_macros/test.h"

#include "opencl/source/helpers/hardware_commands_helper.h"
#include "opencl/test/unit_test/fixtures/cl_device_fixture.h"
#include "opencl/test/unit_test/fixtures/device_info_fixture.h"
#include "opencl/test/unit_test/mocks/mock_kernel.h"

using namespace NEO;

typedef Test<ClDeviceFixture> XeHpcCoreDeviceCaps;

XE_HPC_CORETEST_F(XeHpcCoreDeviceCaps, givenXeHpcCoreWhenCheckFtrSupportsInteger64BitAtomicsThenReturnTrue) {
    EXPECT_TRUE(pDevice->getHardwareInfo().capabilityTable.ftrSupportsInteger64BitAtomics);
}

XE_HPC_CORETEST_F(XeHpcCoreDeviceCaps, givenXeHpcCoreWhenCheckingImageSupportThenReturnFalse) {
    EXPECT_FALSE(pDevice->getHardwareInfo().capabilityTable.supportsImages);
}

XE_HPC_CORETEST_F(XeHpcCoreDeviceCaps, givenXeHpcCoreWhenCheckingMediaBlockSupportThenReturnFalse) {
    EXPECT_FALSE(pDevice->getHardwareInfo().capabilityTable.supportsMediaBlock);
}

XE_HPC_CORETEST_F(XeHpcCoreDeviceCaps, givenXeHpcCoreWhenCheckingCoherencySupportThenReturnFalse) {
    EXPECT_FALSE(pDevice->getHardwareInfo().capabilityTable.ftrSupportsCoherency);
}

XE_HPC_CORETEST_F(XeHpcCoreDeviceCaps, givenHwInfoWhenSlmSizeIsRequiredThenReturnCorrectValue) {
    EXPECT_EQ(128u, pDevice->getHardwareInfo().capabilityTable.slmSize);
}

XE_HPC_CORETEST_F(XeHpcCoreDeviceCaps, givenXeHpcCoreWhenCheckExtensionsThenDeviceDoesNotReportClKhrSubgroupsExtension) {
    const auto &caps = pClDevice->getDeviceInfo();

    EXPECT_THAT(caps.deviceExtensions, testing::HasSubstr(std::string("cl_khr_subgroups")));
    EXPECT_THAT(caps.deviceExtensions, testing::HasSubstr(std::string("cl_intel_subgroup_matrix_multiply_accumulate")));
    EXPECT_THAT(caps.deviceExtensions, testing::HasSubstr(std::string("cl_intel_subgroup_split_matrix_multiply_accumulate")));
}

XE_HPC_CORETEST_F(XeHpcCoreDeviceCaps, givenXeHpcCoreWhenCheckingCapsThenDeviceDoesNotSupportIndependentForwardProgress) {
    const auto &caps = pClDevice->getDeviceInfo();

    EXPECT_TRUE(caps.independentForwardProgress);
}

XE_HPC_CORETEST_F(XeHpcCoreDeviceCaps, givenDeviceWhenAskingForSubGroupSizesThenReturnCorrectValues) {
    auto &hwHelper = HwHelper::get(pDevice->getHardwareInfo().platform.eRenderCoreFamily);

    auto deviceSubgroups = hwHelper.getDeviceSubGroupSizes();

    EXPECT_EQ(2u, deviceSubgroups.size());
    EXPECT_EQ(16u, deviceSubgroups[0]);
    EXPECT_EQ(32u, deviceSubgroups[1]);
}

using QueueFamilyNameTestXeHpcCore = QueueFamilyNameTest;

XE_HPC_CORETEST_F(QueueFamilyNameTestXeHpcCore, givenCccsWhenGettingQueueFamilyNameThenReturnProperValue) {
    verify(EngineGroupType::RenderCompute, "cccs");
}

XE_HPC_CORETEST_F(QueueFamilyNameTestXeHpcCore, givenLinkedBcsWhenGettingQueueFamilyNameThenReturnProperValue) {
    verify(EngineGroupType::LinkedCopy, "linked bcs");
}
