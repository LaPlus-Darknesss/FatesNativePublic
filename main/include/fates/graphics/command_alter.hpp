#pragma once
#include <cstddef>
#include <cstdint>

class CommandAlter {
public:
    class Callback {
    public:
        virtual void OnReplace() = 0;
    protected:
        ~Callback() = default;
    };

    static void Initialize();
    static void Clear();
    static void Execute(unsigned int destinationBase, unsigned int sourceBase, unsigned int commandOffset);
    static void AddMtx44(const float* matrix);
    static void AddMtx34(const float* matrix);
    static void AddCommand(const unsigned int* words, int wordCount);
    static void AddCallback(Callback* callback);
    static unsigned int GetCommandOffset();
    static void* Alloc(unsigned int size);
};
