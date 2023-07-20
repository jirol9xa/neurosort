#pragma once

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include "BufferQueue.hpp"

class Shmem_factory {
  public:
    static Shmem_factory &getInstance()
    {
        static Shmem_factory shmem_fact;
        return shmem_fact;
    }

    void changeSize(size_t size)
    {
        auto &fact = getInstance();
        fact.shm_.truncate(size);

        size_ = size;
    }
    size_t getSize() { return size_; }
    void  *getAddress() { return region_.get_address(); }

    Shmem_factory &operator=(Shmem_factory &) = delete;
    Shmem_factory(Shmem_factory &)            = delete;

    static constexpr const char *shm_name = "SharedMem";

  private:
    Shmem_factory()
    {
        using namespace boost::interprocess;

        shared_memory_object::remove(shm_name);

        shm_ = shared_memory_object(create_only, shm_name, read_write);
        shm_.truncate(size_);
        region_ = mapped_region(shm_, read_write);
    }
    ~Shmem_factory() {}// boost::interprocess::shared_memory_object::remove(shm_name); }

    boost::interprocess::shared_memory_object shm_;
    boost::interprocess::mapped_region        region_;

    size_t size_ = sizeof(BufferQueue) + BufferQueue::OVERALL_BUFFS_AMNT * (sizeof(void*) + sizeof(int));
};
