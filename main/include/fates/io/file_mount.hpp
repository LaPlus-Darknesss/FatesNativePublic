#pragma once

#include <array>
#include <mutex>

class IdentHash;

class FileMount {
public:
    class Node {
    public:
        int id{};
        std::array<char, 0x50> mountPath{};
        std::array<char, 0x50> hookPath{};

        const char* GetMountPath() const { return mountPath.data(); }
        const char* GetHookPath() const { return hookPath.data(); }

    private:
        friend class FileMount;
        Node* previous{};
        Node* next{};
    };

    FileMount();

    Node* Find(const char* path) const;
    bool IsExist(const char* path) const;
    void Entry(const char* path, const char* hookPath, int id);
    void Remove(int id);

private:
    IdentHash* index_{};
    Node* head_{};
    Node* tail_{};
    int count_{};
    mutable std::mutex mutex_{};
};
