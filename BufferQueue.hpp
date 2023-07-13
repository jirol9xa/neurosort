#pragma once

#include <optional>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

struct Buff {
    void *mem;
    size_t size;
};

/// Implementation of class, that will process interprocess communication and 
/// buffer accessing
class BufferQueue {
    public:
        enum Arch : ssize_t {
            OVERALL_SIZE = 1024,
            OVERALL_BUFFS_AMNT = 2,
            INVALID_VALUE = -1,
        };

        /// Returns the buffer, that now is available for reading (Logger filled it, 
        /// need to write in file)
        std::optional<Buff> getBuffForRead() {
            using namespace boost::interprocess;

            scoped_lock<interprocess_mutex> lock(mutex_);
            
            if (full == INVALID_VALUE)
                return {};

            Buff buffer = getBuffByNumber(full);
            getBufferImpl(full, empty);  

            return buffer;
        }

        /// Returns the buffer, that now is available for writing (Logger will fit it)
        std::optional<Buff> getBuffForWrite() {
            using namespace boost::interprocess;

            scoped_lock<interprocess_mutex> lock(mutex_);

            if (empty == INVALID_VALUE)
                return {};

            Buff buffer = getBuffByNumber(empty);
            getBufferImpl(empty, full);

            return buffer;
        }

    private:
        int empty = 0;
        int full  = -1;

        char buff[Arch::OVERALL_SIZE];

        // TODO: I guess, that only one mutex is not enouhg, so need to make cond_var
        boost::interprocess::interprocess_mutex mutex_;

        void getBufferImpl(int &touched_number, int &other_number) {
            if (other_number == INVALID_VALUE) {
                // Only for 2 buffs system implementation
                touched_number = OVERALL_BUFFS_AMNT - 1 - touched_number;
            }
            else {
                touched_number = INVALID_VALUE;
            }
        }

        // Only for two equals buffer implementation
        Buff getBuffByNumber(int number) { 
            return  {buff + OVERALL_SIZE / 2 * number, OVERALL_SIZE / 2}; }
};

