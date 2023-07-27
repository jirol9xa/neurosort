#pragma once

#include "TypeAliases.hpp"
#include <boost/interprocess/containers/deque.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <queue>
#include <utility>

#define PRINT_LINE fprintf(stderr, "[%s:%d: pid = %d]\n", __func__, __LINE__, getpid())

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

        return end - begin > msg_size;
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
        PRINT_LINE;

        using namespace boost::interprocess;
        scoped_lock<interprocess_mutex> lock(mutex_);

        PRINT_LINE;
        fprintf(stderr, "ready_for_write.empty() = %d\n", ready_for_write_.empty());

        if (old_buff.begin) {
            ready_for_write_.push_back(old_buff.buff_number);
            cond_full_.notify_one();
        }
        if (ready_for_read_.empty()) {
            PRINT_LINE;
            cond_empty_.wait(lock);
        }

        PRINT_LINE;

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
        PRINT_LINE;

        using namespace boost::interprocess;
        scoped_lock<interprocess_mutex> lock(mutex_);

        PRINT_LINE;

        if (old_buff.begin) {
            PRINT_LINE;

            printf("beginByNumber = %p, actualBegin = %p\n",
                   getBuffByNumber(old_buff.buff_number).begin, old_buff.begin);

            ready_for_read_.push_back({old_buff.buff_number, old_buff.size});
            cond_empty_.notify_one();
        }
        if (ready_for_write_.empty()) {
            PRINT_LINE;
            cond_full_.wait(lock);
        }

        PRINT_LINE;

        int number_of_buff = ready_for_write_.front();

        PRINT_LINE;

        ready_for_write_.pop_front();

        PRINT_LINE;

        return getBuffByNumber(number_of_buff);
    }

    void endWriting(Buff old_buff)
    {
        PRINT_LINE;

        using namespace boost::interprocess;
        scoped_lock<interprocess_mutex> lock(mutex_);

        PRINT_LINE;

        is_logger_finished_ = true;

        ready_for_read_.push_back({old_buff.buff_number, old_buff.size});
        cond_empty_.notify_one();

        lock.unlock();
    }

    bool isLoggerFinished() const
    {
        PRINT_LINE;

        using namespace boost::interprocess;
        scoped_lock<interprocess_mutex> lock(mutex_);

        PRINT_LINE;

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
        PRINT_LINE;
        fprintf(stderr, "Number = %d\n", number);

        constexpr size_t chunk_size = OVERALL_SIZE / OVERALL_BUFFS_AMNT;
        return {buffer_mem + chunk_size * number, buffer_mem + chunk_size * (number + 1),
                number};
    }
};
