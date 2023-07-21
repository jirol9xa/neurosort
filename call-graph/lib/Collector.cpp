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

    std::thread exec_patch([argv = argv]() {
        if (execvp(*(argv + 1), argv + 1) == -1)
            perror("Execvp failed");
    });

    while (!queue->isLoggerFinished()) {
        buff = queue->getBuffForRead(buff);
        long msg_size =
            static_cast<char *>(buff.mem_end) - static_cast<char *>(buff.mem_begin);
        file.write(static_cast<char *>(buff.mem_begin), msg_size);

        printf("Print to file done\n\n");
    }

    file.close();

    exec_patch.join();

    // Here we need call parser+elf-parser for formatting "OutFile.txt" into the proper
    // way

    return 0;
}
