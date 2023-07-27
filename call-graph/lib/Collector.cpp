#include "BufferQueue.hpp"
#include "ShmFactory.hpp"
#include "TypeAliases.hpp"
#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <thread>

int main(int argc, char *const argv[])
{
    std::ofstream file("OutFile.txt", std::ios_base::out);
    if (!file) {
        std::cerr << "Can't open OutFile.txt\n";
        return 0;
    }

    using namespace boost::interprocess;
    using namespace ShmFactory;

    MemCleaner mem_cl;

    auto                       shm = Shmem_factory::getManagedMemPtr();
    TypeAliases::VoidAllocator alloc(shm->get_segment_manager());

    BufferQueue *queue = shm->construct<BufferQueue>(unique_instance)(alloc);
    Buff         buff;

#if 0
    std::thread exec_patch([argv = argv]() {
        if (execvp(*(argv + 1), argv + 1) == -1)
            perror("Execvp failed");
    });
#endif
    int pid = 0;
    if (pid = fork(); pid == 0) {
        if (execvp(*(argv + 1), argv + 1) == -1)
            perror("Execvp failed");
    }
    else {
        while (!queue->isLoggerFinished()) {
            PRINT_LINE;
            buff          = queue->getBuffForRead(buff);
            long msg_size = buff.size;
            file.write(buff.begin, msg_size);

            PRINT_LINE;
        }

        PRINT_LINE;

        file.close();

        // exec_patch.join();

        // Here we need call parser+elf-parser for formatting "OutFile.txt" into the
        // proper way

        int status;
        while (wait(&status) != -1)
            continue;

        PRINT_LINE;
    }

    return 0;
}
