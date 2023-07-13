#include "BufferQueue.hpp"
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

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

  private:
    Shmem_factory()
    {
        using namespace boost::interprocess;

        shared_memory_object::remove("SharedMem");

        shm_ = shared_memory_object(create_only, "SharedMem", read_write);
        shm_.truncate(size_);
        region_ = mapped_region(shm_, read_write);
    }
    ~Shmem_factory() { boost::interprocess::shared_memory_object::remove("SharedMem"); }

    boost::interprocess::shared_memory_object shm_;
    boost::interprocess::mapped_region        region_;

    size_t size_ = sizeof(BufferQueue);
};

int main()
{
    using namespace boost::interprocess;
    Shmem_factory &shmem_fact = Shmem_factory::getInstance();

    void *addr = shmem_fact.getAddress();

    BufferQueue *queue = new (addr) BufferQueue;

    return 0;
}
