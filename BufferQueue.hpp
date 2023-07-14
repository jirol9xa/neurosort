#pragma once

#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/containers/deque.hpp>
#include <queue>
#include <utility>

#define PRINT_LINE fprintf(stderr, "[%s:%d: pid = %d]\n", __func__, __LINE__, getpid())

// TODO: If app will have a lot of threads we should replace bool isLoggerFinished with smth idk

struct Buff {
    void  *mem_begin  = nullptr;
    void  *mem_end = nullptr;
    int    buff_number;

    bool can_fit(size_t msg_size) {
        if (!mem_begin || !mem_end)
            return false;

        return static_cast<char *>(mem_end) - static_cast<char *>(mem_begin) > msg_size;
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

    BufferQueue()
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

        if (old_buff.mem_begin) {
            ready_for_write_.push_back(old_buff.buff_number);
            cond_.notify_all();
        }
        if (ready_for_read_.empty()) {
            PRINT_LINE;
            cond_.wait(lock);
        }

        PRINT_LINE;

        auto [buff_number, buff_end] = ready_for_read_.front();
        ready_for_read_.pop_front();

        Buff mem_buff_by_number = getBuffByNumber(buff_number);
        mem_buff_by_number.mem_end = buff_end;

        return mem_buff_by_number;
    }

    /// Returns the buffer, that now is available for writing (Logger will fit it)
    Buff getBuffForWrite(Buff old_buff = {})
    {
        PRINT_LINE;

        using namespace boost::interprocess;
        scoped_lock<interprocess_mutex> lock(mutex_);

        PRINT_LINE;

        if (old_buff.mem_begin) {
            ready_for_read_.push_back({old_buff.buff_number, old_buff.mem_begin});
            cond_.notify_all();
        }
        if (ready_for_write_.empty())
            cond_.wait(lock);

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

        ready_for_read_.push_back({old_buff.buff_number, old_buff.mem_begin});
        cond_.notify_all();
    }

    bool isLoggerFinished() const {
        PRINT_LINE;

        using namespace boost::interprocess;
        scoped_lock<interprocess_mutex> lock(mutex_);

        PRINT_LINE;

        return is_logger_finished_;
    }

  private:
    // Memory, where all buffers take place
    char buffer_mem[Arch::OVERALL_SIZE];

    // Queues contain numbers of memory buffers, that are ready for write or read
    boost::interprocess::deque<int> ready_for_write_;
    boost::interprocess::deque<std::pair<int, void *>> ready_for_read_;

    mutable boost::interprocess::interprocess_mutex     mutex_;
    mutable boost::interprocess::interprocess_condition cond_;
    bool is_logger_finished_ = false;

    Buff getBuffByNumber(int number)
    {
        PRINT_LINE;

        constexpr size_t chunk_size = OVERALL_SIZE / OVERALL_BUFFS_AMNT;
        return {buffer_mem +  chunk_size * number,
                buffer_mem + chunk_size * (number + 1), number};
    }
};
