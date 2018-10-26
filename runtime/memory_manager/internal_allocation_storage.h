/*
 * Copyright (C) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include <cstdint>
#include <memory>
#include <mutex>

namespace OCLRT {
class AllocationsList;
class CommandStreamReceiver;
class GraphicsAllocation;

class InternalAllocationStorage {
  public:
    MOCKABLE_VIRTUAL ~InternalAllocationStorage() = default;
    InternalAllocationStorage(CommandStreamReceiver &commandStreamReceiver);
    MOCKABLE_VIRTUAL void cleanAllocationList(uint32_t waitTaskCount, uint32_t allocationUsage);
    void freeAllocationsList(uint32_t waitTaskCount, AllocationsList &allocationsList);
    void storeAllocation(std::unique_ptr<GraphicsAllocation> gfxAllocation, uint32_t allocationUsage);
    void storeAllocationWithTaskCount(std::unique_ptr<GraphicsAllocation> gfxAllocation, uint32_t allocationUsage, uint32_t taskCount);
    std::unique_ptr<GraphicsAllocation> obtainReusableAllocation(size_t requiredSize, bool isInternalAllocationRequired);

  protected:
    std::recursive_mutex mutex;
    CommandStreamReceiver &commandStreamReceiver;
};
} // namespace OCLRT
