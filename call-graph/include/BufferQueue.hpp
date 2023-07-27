#pragma once

#include "TypeAliases.hpp"
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <queue>
#include <utility>

// TODO: If app will have a lot of threads we should replace bool isLoggerFinished with
// smth idk

struct Buff {
    char *begin       = nullptr;
    char *end         = nullptr;
    int   buff_number = 0;
    int   size        = 0;

    bool can_fit(size_t msg_size)
    {
        if (!begin || !end)
            return false;

        return end - begin - size > msg_size;
    }
};

/// Implementation of class, that will process interprocess communication and
/// buffer accessing
class BufferQueue {
  public:
    enum Arch : size_t {
        OVERALL_SIZE       = 1024,
        OVERALL_BUFFS_AMNT = 2,
    };

    BufferQueue(const TypeAliases::VoidAllocator &alloc)
        : ready_for_write_(alloc), ready_for_read_(alloc)
    {
        for (size_t i = 0; i < OVERALL_BUFFS_AMNT; ++i)
            ready_for_write_.push_back(i);
    }

    /// Returns the buffer, that now is available for reading (Logger filled it,
    /// need to write in file)
    Buff getBuffForRead(Buff old_buff = {})
    {
        using namespace boost::interprocess;
        scoped_lock<interprocess_mutex> lock(mutex_);

        if (old_buff.begin) {
            ready_for_write_.push_back(old_buff.buff_number);
            cond_full_.notify_one();
        }
        if (ready_for_read_.empty())
            cond_empty_.wait(lock);

        auto [buff_number, buff_size] = ready_for_read_.front();
        ready_for_read_.pop_front();

        Buff mem_buff_by_number = getBuffByNumber(buff_number);
        mem_buff_by_number.size = buff_size;

        assert(buff_size > 0);

        return mem_buff_by_number;
    }

    /// Returns the buffer, that now is available for writing (Logger will fit it)
    Buff getBuffForWrite(Buff old_buff = {})
    {
        using namespace boost::interprocess;
        scoped_lock<interprocess_mutex> lock(mutex_);

        if (old_buff.begin) {
            ready_for_read_.push_back({old_buff.buff_number, old_buff.size});
            cond_empty_.notify_one();
        }
        if (ready_for_write_.empty())
            cond_full_.wait(lock);

        int number_of_buff = ready_for_write_.front();
        ready_for_write_.pop_front();

        return getBuffByNumber(number_of_buff);
    }

    void endWriting(Buff old_buff)
    {
        using namespace boost::interprocess;
        scoped_lock<interprocess_mutex> lock(mutex_);

        is_logger_finished_ = true;

        ready_for_read_.push_back({old_buff.buff_number, old_buff.size});
        cond_empty_.notify_one();

        lock.unlock();
    }

    bool isLoggerFinished() const
    {
        using namespace boost::interprocess;
        scoped_lock<interprocess_mutex> lock(mutex_);

        return is_logger_finished_;
    }

  private:
    template <typename T>
    using MetaAllocator = boost::interprocess::allocator<T, TypeAliases::SegmentManager>;
    using ReadBuffType  = std::pair<int, long>;

    // Memory, where all buffers take place
    char buffer_mem[Arch::OVERALL_SIZE];

    // Queues contain numbers of memory buffers, that are ready for write or read
    boost::interprocess::deque<int, MetaAllocator<int>> ready_for_write_;
    boost::interprocess::deque<ReadBuffType, MetaAllocator<ReadBuffType>> ready_for_read_;

    mutable boost::interprocess::interprocess_mutex mutex_;
    // cond_full_ notifies if there is at least one buffer for logger (empty)
    mutable boost::interprocess::interprocess_condition cond_full_;
    // cond_empty_ notifies if there is at least one buffer for collector filled by logger
    mutable boost::interprocess::interprocess_condition cond_empty_;
    bool                                                is_logger_finished_ = false;

    Buff getBuffByNumber(int number)
    {
        constexpr size_t chunk_size = OVERALL_SIZE / OVERALL_BUFFS_AMNT;
        return {buffer_mem + chunk_size * number, buffer_mem + chunk_size * (number + 1),
                number};
    }
};
