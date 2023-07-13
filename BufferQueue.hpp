#pragma once

#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <queue>

struct Buff {
    void  *mem  = nullptr;
    size_t size = 0;
    int    buff_number;
};

/// Implementation of class, that will process interprocess communication and
/// buffer accessing
class BufferQueue {
  public:
    enum Arch : ssize_t {
        OVERALL_SIZE       = 1024,
        OVERALL_BUFFS_AMNT = 2,
    };

    /// Returns the buffer, that now is available for reading (Logger filled it,
    /// need to write in file)
    Buff getBuffForRead(Buff old_buff = {})
    {
        if (old_buff.mem) {
            ready_for_write_.push(old_buff.buff_number);
            cond_.notify_one();
        }

        return getBufferImpl(ready_for_read_);
    }

    /// Returns the buffer, that now is available for writing (Logger will fit it)
    Buff getBuffForWrite(Buff old_buff = {})
    {
        if (old_buff.mem) {
            ready_for_read_.push(old_buff.buff_number);
            cond_.notify_one();
        }

        return getBufferImpl(ready_for_write_);
    }

  private:
    // Memory, where all buffers take place
    char buffer_mem[Arch::OVERALL_SIZE];

    // Queues contain numbers of memory buffers, that are ready for write or read
    std::queue<int> ready_for_write_;
    std::queue<int> ready_for_read_;

    boost::interprocess::interprocess_mutex     mutex_;
    boost::interprocess::interprocess_condition cond_;

    enum STATUSES : size_t {
        NOT_USED,
        READY_FOR_WRITE,
        WRITING_IN_PROGRESS,
        READY_FOR_READ,
        READING_IN_PROGRESS,
    };

    Buff getBufferImpl(std::queue<int> &queue)
    {
        using namespace boost::interprocess;
        scoped_lock<interprocess_mutex> lock(mutex_);

        if (queue.empty())
            cond_.wait(lock);

        int number_of_buff = queue.front();
        queue.pop();

        return getBuffByNumber(number_of_buff);
    }

    // Only for two equals buffer implementation
    Buff getBuffByNumber(int number)
    {
        return {buffer_mem + OVERALL_SIZE / 2 * number, OVERALL_SIZE / 2};
    }
};
