/*
 * Copyright (C) 2020-2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "shared/source/command_container/command_encoder.h"
#include "shared/source/command_stream/csr_definitions.h"
#include "shared/source/command_stream/linear_stream.h"
#include "shared/source/device/device.h"
#include "shared/source/helpers/api_specific_config.h"
#include "shared/source/helpers/hw_helper.h"
#include "shared/source/helpers/hw_info.h"
#include "shared/source/helpers/interlocked_max.h"
#include "shared/source/helpers/preamble.h"
#include "shared/source/helpers/state_base_address.h"
#include "shared/source/os_interface/os_context.h"

#include "level_zero/core/source/cmdlist/cmdlist.h"
#include "level_zero/core/source/cmdqueue/cmdqueue_hw.h"
#include "level_zero/core/source/device/device.h"
#include "level_zero/core/source/fence/fence.h"
#include "level_zero/tools/source/metrics/metric.h"

#include <limits>

namespace L0 {

template <GFXCORE_FAMILY gfxCoreFamily>
void CommandQueueHw<gfxCoreFamily>::programStateBaseAddress(uint64_t gsba, bool useLocalMemoryForIndirectHeap, NEO::LinearStream &commandStream, bool cachedMOCSAllowed) {
    using GfxFamily = typename NEO::GfxFamilyMapper<gfxCoreFamily>::GfxFamily;
    using STATE_BASE_ADDRESS = typename GfxFamily::STATE_BASE_ADDRESS;

    const auto &hwInfo = this->device->getHwInfo();
    NEO::PipeControlArgs pcArgs;
    pcArgs.dcFlushEnable = NEO::MemorySynchronizationCommands<GfxFamily>::getDcFlushEnable(true, hwInfo);
    pcArgs.textureCacheInvalidationEnable = true;

    NEO::MemorySynchronizationCommands<GfxFamily>::addPipeControl(commandStream, pcArgs);

    NEO::Device *neoDevice = device->getNEODevice();
    bool isRcs = this->getCsr()->isRcs();
    NEO::EncodeWA<GfxFamily>::encodeAdditionalPipelineSelect(commandStream, {}, true, hwInfo, isRcs);

    auto pSbaCmd = static_cast<STATE_BASE_ADDRESS *>(commandStream.getSpace(sizeof(STATE_BASE_ADDRESS)));
    STATE_BASE_ADDRESS sbaCmd;

    bool useGlobalSshAndDsh = NEO::ApiSpecificConfig::getBindlessConfiguration();
    uint64_t globalHeapsBase = 0;
    if (useGlobalSshAndDsh) {
        globalHeapsBase = neoDevice->getBindlessHeapsHelper()->getGlobalHeapsBase();
    }

    auto indirectObjectHeapBaseAddress = neoDevice->getMemoryManager()->getInternalHeapBaseAddress(device->getRootDeviceIndex(), useLocalMemoryForIndirectHeap);
    auto instructionHeapBaseAddress = neoDevice->getMemoryManager()->getInternalHeapBaseAddress(device->getRootDeviceIndex(), neoDevice->getMemoryManager()->isLocalMemoryUsedForIsa(neoDevice->getRootDeviceIndex()));

    NEO::StateBaseAddressHelper<GfxFamily>::programStateBaseAddress(&sbaCmd,
                                                                    nullptr,
                                                                    nullptr,
                                                                    nullptr,
                                                                    gsba,
                                                                    true,
                                                                    (device->getMOCS(cachedMOCSAllowed, false) >> 1),
                                                                    indirectObjectHeapBaseAddress,
                                                                    instructionHeapBaseAddress,
                                                                    globalHeapsBase,
                                                                    true,
                                                                    useGlobalSshAndDsh,
                                                                    neoDevice->getGmmHelper(),
                                                                    false,
                                                                    NEO::MemoryCompressionState::NotApplicable,
                                                                    false,
                                                                    1u);
    *pSbaCmd = sbaCmd;
    csr->setGSBAStateDirty(false);

    if (NEO::Debugger::isDebugEnabled(internalUsage) && device->getL0Debugger()) {

        NEO::Debugger::SbaAddresses sbaAddresses = {};
        sbaAddresses.BindlessSurfaceStateBaseAddress = sbaCmd.getBindlessSurfaceStateBaseAddress();
        sbaAddresses.DynamicStateBaseAddress = sbaCmd.getDynamicStateBaseAddress();
        sbaAddresses.GeneralStateBaseAddress = sbaCmd.getGeneralStateBaseAddress();
        sbaAddresses.IndirectObjectBaseAddress = sbaCmd.getIndirectObjectBaseAddress();
        sbaAddresses.InstructionBaseAddress = sbaCmd.getInstructionBaseAddress();
        sbaAddresses.SurfaceStateBaseAddress = sbaCmd.getSurfaceStateBaseAddress();

        device->getL0Debugger()->programSbaTrackingCommands(commandStream, sbaAddresses);
    }

    NEO::EncodeWA<GfxFamily>::encodeAdditionalPipelineSelect(commandStream, {}, false, hwInfo, isRcs);
}

template <GFXCORE_FAMILY gfxCoreFamily>
size_t CommandQueueHw<gfxCoreFamily>::estimateStateBaseAddressCmdSize() {
    using GfxFamily = typename NEO::GfxFamilyMapper<gfxCoreFamily>::GfxFamily;
    using STATE_BASE_ADDRESS = typename GfxFamily::STATE_BASE_ADDRESS;
    using PIPE_CONTROL = typename GfxFamily::PIPE_CONTROL;

    size_t size = sizeof(STATE_BASE_ADDRESS) + sizeof(PIPE_CONTROL) + NEO::EncodeWA<GfxFamily>::getAdditionalPipelineSelectSize(*device->getNEODevice());

    if (NEO::Debugger::isDebugEnabled(internalUsage) && device->getL0Debugger() != nullptr) {
        const size_t trackedAddressesCount = 6;
        size += device->getL0Debugger()->getSbaTrackingCommandsSize(trackedAddressesCount);
    }
    return size;
}

template <GFXCORE_FAMILY gfxCoreFamily>
void CommandQueueHw<gfxCoreFamily>::handleScratchSpace(NEO::HeapContainer &heapContainer,
                                                       NEO::ScratchSpaceController *scratchController,
                                                       bool &gsbaState, bool &frontEndState,
                                                       uint32_t perThreadScratchSpaceSize, uint32_t perThreadPrivateScratchSize) {

    if (perThreadScratchSpaceSize > 0) {
        scratchController->setRequiredScratchSpace(nullptr, 0u, perThreadScratchSpaceSize, 0u, csr->peekTaskCount(),
                                                   csr->getOsContext(), gsbaState, frontEndState);
        auto scratchAllocation = scratchController->getScratchSpaceAllocation();
        csr->makeResident(*scratchAllocation);
    }
}

template <GFXCORE_FAMILY gfxCoreFamily>
void CommandQueueHw<gfxCoreFamily>::patchCommands(CommandList &commandList, uint64_t scratchAddress) {
    auto &commandsToPatch = commandList.getCommandsToPatch();
    UNRECOVERABLE_IF(!commandsToPatch.empty());
}

} // namespace L0
