#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>

namespace TypeAliases {
using ManagedMemPtr =
    std::unique_ptr<boost::interprocess::managed_shared_memory,
                    std::function<void(boost::interprocess::managed_shared_memory *)>>;
using SegmentManager = boost::interprocess::managed_shared_memory::segment_manager;
using VoidAllocator  = boost::interprocess::allocator<void, SegmentManager>;
} // namespace TypeAliases
