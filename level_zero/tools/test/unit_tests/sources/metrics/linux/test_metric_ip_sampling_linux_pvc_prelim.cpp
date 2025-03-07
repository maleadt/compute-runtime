/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/os_interface/linux/drm_neo.h"
#include "shared/source/os_interface/linux/ioctl_helper.h"
#include "shared/source/os_interface/linux/sys_calls.h"
#include "shared/test/common/libult/linux/drm_mock.h"
#include "shared/test/common/mocks/ult_device_factory.h"
#include "shared/test/common/os_interface/linux/sys_calls_linux_ult.h"
#include "shared/test/common/test_macros/test.h"

#include "level_zero/core/test/unit_tests/fixtures/device_fixture.h"
#include "level_zero/tools/source/metrics/os_metric_ip_sampling.h"

#include "device_ids_configs.h"
#include "hw_cmds.h"

namespace NEO {
namespace SysCalls {
extern int closeFuncRetVal;

} // namespace SysCalls
} // namespace NEO

namespace L0 {
namespace ult {

class IoctlHelperPrelim20Mock : public NEO::IoctlHelperPrelim20 {

    bool getEuStallProperties(std::array<uint64_t, 10u> &properties, uint64_t dssBufferSize, uint64_t samplingRate, uint64_t pollPeriod, uint64_t engineInstance) override {
        return false;
    }
};

class DrmPrelimMock : public DrmMock {
  public:
    DrmPrelimMock(RootDeviceEnvironment &rootDeviceEnvironment) : DrmPrelimMock(rootDeviceEnvironment, defaultHwInfo.get()) {}
    DrmPrelimMock(RootDeviceEnvironment &rootDeviceEnvironment, HardwareInfo *inputHwInfo, bool invokeQueryEngineInfo = true) : DrmMock(rootDeviceEnvironment) {
        customHwInfo = std::make_unique<NEO::HardwareInfo>(&inputHwInfo->platform, &inputHwInfo->featureTable,
                                                           &inputHwInfo->workaroundTable, &inputHwInfo->gtSystemInfo, inputHwInfo->capabilityTable);
        customHwInfo->gtSystemInfo.MaxDualSubSlicesSupported = 64;
        rootDeviceEnvironment.setHwInfo(customHwInfo.get());
        setupIoctlHelper(rootDeviceEnvironment.getHardwareInfo()->platform.eProductFamily);
        recentProperties.fill(255);
        if (invokeQueryEngineInfo) {
            queryEngineInfo();
        }
    }

    bool queryEngineInfo() override {
        uint16_t computeEngineClass = getIoctlHelper()->getComputeEngineClass();
        std::vector<EngineCapabilities> engines(4);
        engines[0].engine = {computeEngineClass, 0};
        engines[0].capabilities = 0;
        engines[1].engine = {computeEngineClass, 1};
        engines[1].capabilities = 0;
        engines[2].engine = {computeEngineClass, 2};
        engines[2].capabilities = 0;
        engines[3].engine = {computeEngineClass, 3};
        engines[3].capabilities = 0;

        std::vector<DistanceInfo> distances(4);
        distances[0].engine = engines[0].engine;
        distances[0].region = {I915_MEMORY_CLASS_DEVICE, 0};
        distances[1].engine = engines[1].engine;
        distances[1].region = {I915_MEMORY_CLASS_DEVICE, 1};
        distances[2].engine = engines[2].engine;
        distances[2].region = {I915_MEMORY_CLASS_DEVICE, 2};
        distances[3].engine = engines[3].engine;
        distances[3].region = {I915_MEMORY_CLASS_DEVICE, 3};

        std::vector<drm_i915_query_item> queryItems{distances.size()};
        for (auto i = 0u; i < distances.size(); i++) {
            queryItems[i].length = sizeof(drm_i915_query_engine_info);
        }

        engineInfo = std::make_unique<EngineInfo>(this, customHwInfo.get(), 4, distances, queryItems, engines);
        return true;
    }

    bool queryEngineInfo1SubDevice() {
        uint16_t computeEngineClass = getIoctlHelper()->getComputeEngineClass();
        std::vector<EngineCapabilities> engines(1);
        engines[0].engine = {computeEngineClass, 0};
        engines[0].capabilities = 0;

        std::vector<DistanceInfo> distances(1);
        distances[0].engine = engines[0].engine;
        distances[0].region = {I915_MEMORY_CLASS_DEVICE, 0};

        std::vector<drm_i915_query_item> queryItems{distances.size()};
        for (auto i = 0u; i < distances.size(); i++) {
            queryItems[i].length = sizeof(drm_i915_query_engine_info);
        }

        engineInfo = std::make_unique<EngineInfo>(this, customHwInfo.get(), 1, distances, queryItems, engines);
        return true;
    }

    void getPrelimVersion(std::string &prelimVersion) override {
        prelimVersion = "2.0";
    }

    int handleRemainingRequests(unsigned long request, void *arg) override {

        if (request == DRM_IOCTL_I915_PERF_OPEN) {
            drm_i915_perf_open_param *param = reinterpret_cast<drm_i915_perf_open_param *>(arg);
            std::memcpy(recentProperties.data(), reinterpret_cast<void *>(param->properties_ptr), param->num_properties * 8 * 2);
            return ioctli915PerfOpenReturn;
        }
        return -1;
    }

    void setIoctlHelperPrelim20Mock() {
        backUpIoctlHelper = std::move(ioctlHelper);
        ioctlHelper = static_cast<std::unique_ptr<NEO::IoctlHelper>>(std::make_unique<IoctlHelperPrelim20Mock>());
    }

    void restoreIoctlHelperPrelim20() {
        ioctlHelper = std::move(backUpIoctlHelper);
    }

    std::unique_ptr<NEO::HardwareInfo> customHwInfo;
    std::unique_ptr<NEO::IoctlHelper> backUpIoctlHelper;
    int ioctli915PerfOpenReturn = 1;
    std::array<uint64_t, 10u> recentProperties{};
};

class MetricIpSamplingLinuxTestPrelim : public DeviceFixture,
                                        public ::testing::Test {
  public:
    void SetUp() override {
        DeviceFixture::SetUp();
        neoDevice->getExecutionEnvironment()->rootDeviceEnvironments[device->getRootDeviceIndex()]->osInterface = std::make_unique<NEO::OSInterface>();
        auto &osInterface = device->getOsInterface();
        osInterface.setDriverModel(std::make_unique<DrmPrelimMock>(const_cast<NEO::RootDeviceEnvironment &>(neoDevice->getRootDeviceEnvironment())));

        metricIpSamplingOsInterface = MetricIpSamplingOsInterface::create(static_cast<L0::Device &>(*device));
    }

    void TearDown() override {
        DeviceFixture::TearDown();
    }
    std::unique_ptr<MetricIpSamplingOsInterface> metricIpSamplingOsInterface = nullptr;
};

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, givenCorrectArgumentsWhenStartMeasurementIsCalledThenReturnSuccess, IsPVC) {

    constexpr uint32_t samplingGranularity = 251;
    constexpr uint32_t gpuClockPeriodNs = 1000000;
    constexpr uint32_t samplingUnit = 3;
    uint32_t notifyEveryNReports = 0, samplingPeriodNs = samplingGranularity * samplingUnit * gpuClockPeriodNs;
    EXPECT_EQ(metricIpSamplingOsInterface->startMeasurement(notifyEveryNReports, samplingPeriodNs), ZE_RESULT_SUCCESS);
    EXPECT_EQ(samplingPeriodNs, samplingGranularity * samplingUnit * gpuClockPeriodNs);
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, givenGetTimestampFrequencyFailsWhenStartMeasurementIsCalledThenReturnFailure, IsPVC) {

    auto drm = static_cast<DrmPrelimMock *>(device->getOsInterface().getDriverModel()->as<NEO::Drm>());
    VariableBackup<int> backupCsTimeStampFrequency(&drm->storedCsTimestampFrequency, 0);
    VariableBackup<int> backupStoredRetVal(&drm->storedRetVal, -1);

    uint32_t notifyEveryNReports = 0, samplingPeriodNs = 10000;
    EXPECT_EQ(metricIpSamplingOsInterface->startMeasurement(notifyEveryNReports, samplingPeriodNs), ZE_RESULT_ERROR_UNKNOWN);
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, givenIoctlI915PerfOpenFailsWhenStartMeasurementIsCalledThenReturnFailure, IsPVC) {

    auto drm = static_cast<DrmPrelimMock *>(device->getOsInterface().getDriverModel()->as<NEO::Drm>());
    uint32_t notifyEveryNReports = 0, samplingPeriodNs = 10000;
    VariableBackup<int> backupI915PerfOpenReturn(&drm->ioctli915PerfOpenReturn, -1);
    EXPECT_EQ(metricIpSamplingOsInterface->startMeasurement(notifyEveryNReports, samplingPeriodNs), ZE_RESULT_ERROR_UNKNOWN);
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, givenI915PerfIoctlEnableFailsWhenStartMeasurementIsCalledThenReturnFailure, IsPVC) {

    uint32_t notifyEveryNReports = 0, samplingPeriodNs = 10000;
    VariableBackup<decltype(SysCalls::sysCallsIoctl)> mockIoctl(&SysCalls::sysCallsIoctl, [](int fileDescriptor, unsigned long int request, void *arg) -> int {
        if (request == I915_PERF_IOCTL_ENABLE) {
            return -1;
        }
        return 0;
    });

    EXPECT_EQ(metricIpSamplingOsInterface->startMeasurement(notifyEveryNReports, samplingPeriodNs), ZE_RESULT_ERROR_UNKNOWN);
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, givenCloseSucceedsWhenStopMeasurementIsCalledThenReturnSuccess, IsPVC) {

    VariableBackup<int> backupCloseFuncRetval(&NEO::SysCalls::closeFuncRetVal, 0);
    EXPECT_EQ(metricIpSamplingOsInterface->stopMeasurement(), ZE_RESULT_SUCCESS);
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, givenCloseFailsWhenStopMeasurementIsCalledThenReturnFailure, IsPVC) {

    VariableBackup<int> backupCloseFuncRetval(&NEO::SysCalls::closeFuncRetVal, -1);
    EXPECT_EQ(metricIpSamplingOsInterface->stopMeasurement(), ZE_RESULT_ERROR_UNKNOWN);
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, givenI915PerfIoctlDisableFailsWhenStartMeasurementIsCalledThenReturnFailure, IsPVC) {

    VariableBackup<decltype(SysCalls::sysCallsIoctl)> mockIoctl(&SysCalls::sysCallsIoctl, [](int fileDescriptor, unsigned long int request, void *arg) -> int {
        if (request == I915_PERF_IOCTL_DISABLE) {
            return -1;
        }
        return 0;
    });

    EXPECT_EQ(metricIpSamplingOsInterface->stopMeasurement(), ZE_RESULT_ERROR_UNKNOWN);
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, givenReadSucceedsWhenReadDataIsCalledThenReturnSuccess, IsPVC) {

    VariableBackup<decltype(SysCalls::sysCallsRead)> mockRead(&SysCalls::sysCallsRead, [](int fd, void *buf, size_t count) -> ssize_t {
        return 1;
    });
    uint8_t pRawData = 0u;
    size_t pRawDataSize = 0;
    EXPECT_EQ(metricIpSamplingOsInterface->readData(&pRawData, &pRawDataSize), ZE_RESULT_SUCCESS);
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, givenReadFailsWhenReadDataIsCalledThenReturnFailure, IsPVC) {

    VariableBackup<decltype(SysCalls::sysCallsRead)> mockRead(&SysCalls::sysCallsRead, [](int fd, void *buf, size_t count) -> ssize_t {
        return -1;
        errno = EBADF;
    });
    uint8_t pRawData = 0u;
    size_t pRawDataSize = 0;
    EXPECT_EQ(metricIpSamplingOsInterface->readData(&pRawData, &pRawDataSize), ZE_RESULT_ERROR_UNKNOWN);
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, givenReadFailsWithRetryErrorNumberWhenReadDataIsCalledThenReturnSuccess, IsPVC) {

    VariableBackup<decltype(SysCalls::sysCallsRead)> mockRead(&SysCalls::sysCallsRead, [](int fd, void *buf, size_t count) -> ssize_t {
        return -1;
    });
    uint8_t pRawData = 0u;
    size_t pRawDataSize = 0;
    errno = EINTR;
    EXPECT_EQ(metricIpSamplingOsInterface->readData(&pRawData, &pRawDataSize), ZE_RESULT_SUCCESS);
    errno = EBUSY;
    EXPECT_EQ(metricIpSamplingOsInterface->readData(&pRawData, &pRawDataSize), ZE_RESULT_SUCCESS);
    errno = EAGAIN;
    EXPECT_EQ(metricIpSamplingOsInterface->readData(&pRawData, &pRawDataSize), ZE_RESULT_SUCCESS);
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, WhenGetRequiredBufferSizeIsCalledThenCorrectSizeIsReturned, IsPVC) {

    constexpr uint32_t unitReportSize = 64;
    EXPECT_EQ(metricIpSamplingOsInterface->getRequiredBufferSize(10), unitReportSize * 10);
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, GivenPollIsSuccessfulWhenisNReportsAvailableIsCalledThenReturnSuccess, IsPVC) {

    VariableBackup<decltype(SysCalls::sysCallsPoll)> mockPoll(&SysCalls::sysCallsPoll, [](struct pollfd *pollFd, unsigned long int numberOfFds, int timeout) -> int {
        return 1;
    });
    EXPECT_TRUE(metricIpSamplingOsInterface->isNReportsAvailable());
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, GivenPollIsFailureWhenisNReportsAvailableIsCalledThenReturnFailure, IsPVC) {

    VariableBackup<decltype(SysCalls::sysCallsPoll)> mockPoll(&SysCalls::sysCallsPoll, [](struct pollfd *pollFd, unsigned long int numberOfFds, int timeout) -> int {
        return -1;
    });
    EXPECT_FALSE(metricIpSamplingOsInterface->isNReportsAvailable());
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, GivenSupportedProductFamilyAndUnsupportedDeviceIdIsUsedWhenIsDependencyAvailableIsCalledThenReturnFailure, IsPVC) {

    auto hwInfo = neoDevice->getRootDeviceEnvironment().getMutableHardwareInfo();
    hwInfo->platform.eProductFamily = productFamily;
    hwInfo->platform.usDeviceID = NEO::PVC_XL_IDS.front();
    EXPECT_FALSE(metricIpSamplingOsInterface->isDependencyAvailable());
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, GivenSupportedProductFamilyAndSupportedDeviceIdIsUsedWhenIsDependencyAvailableIsCalledThenReturnFailure, IsPVC) {

    auto hwInfo = neoDevice->getRootDeviceEnvironment().getMutableHardwareInfo();
    hwInfo->platform.eProductFamily = productFamily;

    for (auto deviceId : NEO::PVC_XT_IDS) {
        hwInfo->platform.usDeviceID = deviceId;
        EXPECT_TRUE(metricIpSamplingOsInterface->isDependencyAvailable());
    }
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, GivenDriverOpenFailsWhenIsDependencyAvailableIsCalledThenReturnFailure, IsPVC) {

    auto hwInfo = neoDevice->getRootDeviceEnvironment().getMutableHardwareInfo();
    hwInfo->platform.eProductFamily = productFamily;
    hwInfo->platform.usDeviceID = NEO::PVC_XT_IDS.front();

    auto drm = static_cast<DrmPrelimMock *>(device->getOsInterface().getDriverModel()->as<NEO::Drm>());
    VariableBackup<int> backupCsTimeStampFrequency(&drm->storedCsTimestampFrequency, 0);
    VariableBackup<int> backupStoredRetVal(&drm->storedRetVal, -1);

    EXPECT_FALSE(metricIpSamplingOsInterface->isDependencyAvailable());
}

HWTEST2_F(MetricIpSamplingLinuxTestPrelim, GivenIoctlHelperFailsWhenIsDependencyAvailableIsCalledThenReturnFailure, IsPVC) {

    auto hwInfo = neoDevice->getRootDeviceEnvironment().getMutableHardwareInfo();
    hwInfo->platform.eProductFamily = productFamily;
    hwInfo->platform.usDeviceID = NEO::PVC_XT_IDS.front();

    auto drm = static_cast<DrmPrelimMock *>(device->getOsInterface().getDriverModel()->as<NEO::Drm>());

    drm->setIoctlHelperPrelim20Mock();
    EXPECT_FALSE(metricIpSamplingOsInterface->isDependencyAvailable());
    drm->restoreIoctlHelperPrelim20();
}

struct MetricIpSamplingLinuxMultiDeviceTest : public ::testing::Test {

    std::unique_ptr<UltDeviceFactory> createDevices(uint32_t numSubDevices) {
        DebugManager.flags.CreateMultipleSubDevices.set(numSubDevices);
        DebugManager.flags.UseDrmVirtualEnginesForCcs.set(0);
        NEO::ExecutionEnvironment *executionEnvironment = new MockExecutionEnvironment(defaultHwInfo.get(), false, 1);
        executionEnvironment->parseAffinityMask();
        executionEnvironment->rootDeviceEnvironments[0]->osInterface.reset(new OSInterface);
        executionEnvironment->rootDeviceEnvironments[0]->osInterface->setDriverModel(
            std::make_unique<DrmPrelimMock>(const_cast<NEO::RootDeviceEnvironment &>(*executionEnvironment->rootDeviceEnvironments[0]),
                                            defaultHwInfo.get(),
                                            false));
        return std::make_unique<UltDeviceFactory>(1, numSubDevices, *executionEnvironment);
    }

    DebugManagerStateRestore restorer;
};

HWTEST2_F(MetricIpSamplingLinuxMultiDeviceTest, GivenCombinationOfAffinityMaskWhenStartMeasurementIsCalledForRootDeviceThenInstanceIdIsCorrect, IsPVC) {
    DebugManager.flags.ZE_AFFINITY_MASK.set("0.1,0.2,0.3");

    auto deviceFactory = createDevices(4);
    auto driverHandle = std::make_unique<DriverHandleImp>();
    ze_result_t returnValue = ZE_RESULT_SUCCESS;
    auto rootDevice = std::unique_ptr<L0::Device>(L0::Device::create(driverHandle.get(), deviceFactory->rootDevices[0], false, &returnValue));
    auto metricIpSamplingOsInterface = MetricIpSamplingOsInterface::create(static_cast<L0::Device &>(*rootDevice));
    uint32_t notifyEveryNReports = 0, samplingPeriodNs = 10000;
    auto drm = static_cast<DrmPrelimMock *>(rootDevice->getOsInterface().getDriverModel()->as<NEO::Drm>());
    drm->queryEngineInfo();
    EXPECT_EQ(metricIpSamplingOsInterface->startMeasurement(notifyEveryNReports, samplingPeriodNs), ZE_RESULT_SUCCESS);
    // verifying engine instance is set correctly
    EXPECT_EQ(drm->recentProperties[9], 1ull);
}

HWTEST2_F(MetricIpSamplingLinuxMultiDeviceTest, GivenCombinationOfAffinityMaskWhenStartMeasurementIsCalledForSubDeviceThenInstanceIdIsCorrect, IsPVC) {
    DebugManager.flags.ZE_AFFINITY_MASK.set("0.2,0.3");

    auto deviceFactory = createDevices(4);
    auto driverHandle = std::make_unique<DriverHandleImp>();
    ze_result_t returnValue = ZE_RESULT_SUCCESS;
    auto rootDevice = std::unique_ptr<L0::Device>(L0::Device::create(driverHandle.get(), deviceFactory->rootDevices[0], false, &returnValue));

    uint32_t subDeviceCount = 2;
    ze_device_handle_t subDevices[2] = {};

    rootDevice->getSubDevices(&subDeviceCount, subDevices);
    auto metricIpSamplingOsInterface = MetricIpSamplingOsInterface::create(static_cast<L0::Device &>(*subDevices[0]));
    uint32_t notifyEveryNReports = 0, samplingPeriodNs = 10000;
    auto drm = static_cast<DrmPrelimMock *>(rootDevice->getOsInterface().getDriverModel()->as<NEO::Drm>());
    drm->queryEngineInfo();

    EXPECT_EQ(metricIpSamplingOsInterface->startMeasurement(notifyEveryNReports, samplingPeriodNs), ZE_RESULT_SUCCESS);
    // verifying engine instance is set correctly
    EXPECT_EQ(drm->recentProperties[9], 2ull);

    metricIpSamplingOsInterface = MetricIpSamplingOsInterface::create(static_cast<L0::Device &>(*subDevices[1]));
    EXPECT_EQ(metricIpSamplingOsInterface->startMeasurement(notifyEveryNReports, samplingPeriodNs), ZE_RESULT_SUCCESS);
    EXPECT_EQ(drm->recentProperties[9], 3ull);
}

HWTEST2_F(MetricIpSamplingLinuxMultiDeviceTest, GivenEngineInfoIsNullWhenStartMeasurementIsCalledForRootDeviceThenErrorIsReturned, IsPVC) {
    DebugManager.flags.ZE_AFFINITY_MASK.set("0.1");

    auto deviceFactory = createDevices(4);
    auto driverHandle = std::make_unique<DriverHandleImp>();
    ze_result_t returnValue = ZE_RESULT_SUCCESS;
    auto rootDevice = std::unique_ptr<L0::Device>(L0::Device::create(driverHandle.get(), deviceFactory->rootDevices[0], false, &returnValue));
    auto metricIpSamplingOsInterface = MetricIpSamplingOsInterface::create(static_cast<L0::Device &>(*rootDevice));
    uint32_t notifyEveryNReports = 0, samplingPeriodNs = 10000;
    auto drm = static_cast<DrmPrelimMock *>(rootDevice->getOsInterface().getDriverModel()->as<NEO::Drm>());
    drm->queryEngineInfo1SubDevice();
    EXPECT_EQ(metricIpSamplingOsInterface->startMeasurement(notifyEveryNReports, samplingPeriodNs), ZE_RESULT_ERROR_UNKNOWN);
}

HWTEST2_F(MetricIpSamplingLinuxMultiDeviceTest, GivenEngineInstanceIsNullWhenStartMeasurementIsCalledForRootDeviceThenErrorIsReturned, IsPVC) {
    DebugManager.flags.ZE_AFFINITY_MASK.set("0.1");

    auto deviceFactory = createDevices(4);
    auto driverHandle = std::make_unique<DriverHandleImp>();
    ze_result_t returnValue = ZE_RESULT_SUCCESS;
    auto rootDevice = std::unique_ptr<L0::Device>(L0::Device::create(driverHandle.get(), deviceFactory->rootDevices[0], false, &returnValue));
    auto metricIpSamplingOsInterface = MetricIpSamplingOsInterface::create(static_cast<L0::Device &>(*rootDevice));
    uint32_t notifyEveryNReports = 0, samplingPeriodNs = 10000;
    EXPECT_EQ(metricIpSamplingOsInterface->startMeasurement(notifyEveryNReports, samplingPeriodNs), ZE_RESULT_ERROR_UNKNOWN);
}

} // namespace ult
} // namespace L0
