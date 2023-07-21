#pragma once

#include "BufferQueue.hpp"
#include "TypeAliases.hpp"
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <functional>
#include <iostream>
#include <string_view>

namespace ShmFactory {
/// Struct, that contain all info about managed_shared_memory and can return the pointer
/// to it.
struct Shmem_factory {
    static constexpr const char *shm_name = "SharedMem";
    size_t                       getSize() const { return size_; }
    std::string_view             getMemName() const { return shm_name; }

    // 4K is a minimum size shared object
    static constexpr size_t size_ =
        std::max(sizeof(BufferQueue) +
                     BufferQueue::OVERALL_BUFFS_AMNT * (sizeof(void *) + sizeof(int)),
                 4096ul);

    /// Opens or creates managed_shared_memory. !!! Can throw an exception !!!
    static TypeAliases::ManagedMemPtr getManagedMemPtr()
    {
        using namespace boost::interprocess;

        try {
            try {
                return TypeAliases::ManagedMemPtr{
                    new managed_shared_memory(open_or_create, shm_name, size_)};
            }
            catch (interprocess_exception &ex) {
                std::cout << ex.what();
                throw;
            }
        }
        catch (std::bad_alloc &ex) {
            std::cout << ex.what();
            throw;
        }
    }
};

/// Structure for removing shared_memory from file system, create  it only in main proc
/// and only once
struct MemCleaner {
    MemCleaner()
    {
        boost::interprocess::shared_memory_object::remove(Shmem_factory::shm_name);
    }
    ~MemCleaner()
    {
        boost::interprocess::shared_memory_object::remove(Shmem_factory::shm_name);
    }
};

} // namespace ShmFactory
