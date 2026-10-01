#pragma once

class Compress {
public:
    static unsigned int Uncompress(const void* source, void* destination);
    static unsigned int GetOverlapSize(const void* source);
    static unsigned int GetUncompressSize(const void* source);
};
