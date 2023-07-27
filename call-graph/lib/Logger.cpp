#include "ShmFactory.hpp"
#include "TypeAliases.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>

/// Class for implementing Parsing and Changing dot file with graph
/// Singleton implementation
class GraphEditor {
  private:
    // Ctor will dump /proc/self/maps to the maps.txt files
    // It is necessary, because we support -fPIC flag
    GraphEditor()
    {
        using namespace boost::interprocess;
        using ShmFactory::Shmem_factory;

        // Here we make file with maps for elf-parser file
        auto  pid         = getpid();
        char *system_text = (char *)calloc(64, sizeof(char));
        if (!system_text)
            return;

        sprintf(system_text, "cat /proc/%d/maps > maps.txt", pid);

        system(system_text);
        free(system_text);

        Shm   = Shmem_factory::getManagedMemPtr();
        Queue = Shm->find<BufferQueue>(unique_instance).first;
        assert(Queue != nullptr);

        Buffer = Queue->getBuffForWrite();
    };

    ~GraphEditor() { Queue->endWriting(Buffer); }

    TypeAliases::ManagedMemPtr Shm;

    Buff         Buffer;
    BufferQueue *Queue;
    // Lenght if digits of two (long unsigned int) + space sym + new_line sym
    static constexpr size_t MsgLenght = 2 * 16 + 1 + 1;

  public:
    static GraphEditor &getInstance()
    {
        static GraphEditor Object;
        return Object;
    }

    void addCall(uint64_t Caller, uint64_t Callee);
};

void GraphEditor::addCall(uint64_t Caller, uint64_t Callee)
{
    if (!Buffer.can_fit(MsgLenght)) // FIXME
        Buffer = Queue->getBuffForWrite(Buffer);

    Buffer.size += sprintf(Buffer.begin + Buffer.size, "%lx %lx\n", Caller, Callee);
}

// We will use linkonceodr, so we must say compiler not to inline the call
// Or remove it, if compiler inline in any way
void Logger() __attribute__((noinline));

void Logger()
{
    GraphEditor &graph = GraphEditor::getInstance();
    graph.addCall(reinterpret_cast<uint64_t>(__builtin_return_address(1)),
                  reinterpret_cast<uint64_t>(__builtin_return_address(0)));
}
