/*
 * Copyright (C) 2020-2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/command_container/command_encoder.h"
#include "shared/source/gen11/hw_cmds_base.h"
#include "shared/source/gen11/reg_configs.h"

using Family = NEO::ICLFamily;

#include "shared/source/command_container/command_encoder.inl"
#include "shared/source/command_container/command_encoder_bdw_and_later.inl"
#include "shared/source/command_container/encode_compute_mode_bdw_and_later.inl"
#include "shared/source/command_container/image_surface_state/compression_params_bdw_and_later.inl"

namespace NEO {

template <>
size_t EncodeComputeMode<Family>::getCmdSizeForComputeMode(const HardwareInfo &hwInfo, bool hasSharedHandles, bool isRcs) {
    return sizeof(typename Family::MI_LOAD_REGISTER_IMM);
}

template <>
void EncodeComputeMode<Family>::programComputeModeCommandWithSynchronization(
    LinearStream &csr, StateComputeModeProperties &properties, const PipelineSelectArgs &args,
    bool hasSharedHandles, const HardwareInfo &hwInfo, bool isRcs) {
    LriHelper<Family>::program(&csr,
                               gen11HdcModeRegister::address,
                               DwordBuilder::build(gen11HdcModeRegister::forceNonCoherentEnableBit, true, !properties.isCoherencyRequired.value),
                               false);
}

template <>
bool EncodeSurfaceState<Family>::doBindingTablePrefetch() {
    return false;
}

template <>
void EncodeSurfaceState<Family>::setAuxParamsForMCSCCS(R_SURFACE_STATE *surfaceState) {
}

template <>
void EncodeSurfaceState<Family>::setClearColorParams(R_SURFACE_STATE *surfaceState, Gmm *gmm) {
}

template <>
void EncodeSurfaceState<Family>::setFlagsForMediaCompression(R_SURFACE_STATE *surfaceState, Gmm *gmm) {
    if (gmm->gmmResourceInfo->getResourceFlags()->Info.MediaCompressed) {
        surfaceState->setAuxiliarySurfaceMode(Family::RENDER_SURFACE_STATE::AUXILIARY_SURFACE_MODE::AUXILIARY_SURFACE_MODE_AUX_NONE);
    }
}

template struct EncodeDispatchKernel<Family>;
template struct EncodeStates<Family>;
template struct EncodeMath<Family>;
template struct EncodeMathMMIO<Family>;
template struct EncodeIndirectParams<Family>;
template struct EncodeSetMMIO<Family>;
template struct EncodeL3State<Family>;
template struct EncodeMediaInterfaceDescriptorLoad<Family>;
template struct EncodeStateBaseAddress<Family>;
template struct EncodeStoreMMIO<Family>;
template struct EncodeSurfaceState<Family>;
template struct EncodeAtomic<Family>;
template struct EncodeSempahore<Family>;
template struct EncodeBatchBufferStartOrEnd<Family>;
template struct EncodeMiFlushDW<Family>;
template struct EncodeMemoryPrefetch<Family>;
template struct EncodeWA<Family>;
template struct EncodeMiArbCheck<Family>;
template struct EncodeComputeMode<Family>;
template struct EncodeEnableRayTracing<Family>;
template struct EncodeNoop<Family>;
template struct EncodeStoreMemory<Family>;
} // namespace NEO
