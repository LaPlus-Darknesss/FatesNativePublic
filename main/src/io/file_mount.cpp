#include "fates/io/file_mount.hpp"

#include "fates/engine/ident_hash.hpp"

#include <algorithm>
#include <cstring>

namespace {

void CopyPath(std::array<char, 0x50>& destination, const char* source) {
    destination.fill('\0');
    if (source == nullptr) {
        return;
    }
    std::strncpy(destination.data(), source, destination.size());
    destination.back() = '\0';
}

} // namespace

FileMount::FileMount()
    : index_(new IdentHash(0x7F)) {}

FileMount::Node* FileMount::Find(const char* path) const {
    if (path == nullptr || index_ == nullptr) {
        return nullptr;
    }
    return static_cast<Node*>(index_->GetSurely(path));
}

bool FileMount::IsExist(const char* path) const {
    return Find(path) != nullptr;
}

void FileMount::Entry(const char* path, const char* hookPath, int id) {
    if (path == nullptr || index_ == nullptr || Find(path) != nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (Find(path) != nullptr) {
        return;
    }

    auto* node = new Node{};
    node->id = id;
    CopyPath(node->mountPath, path);
    CopyPath(node->hookPath, hookPath);
    node->previous = tail_;

    if (tail_ != nullptr) {
        tail_->next = node;
    }
    if (head_ == nullptr) {
        head_ = node;
    }
    tail_ = node;
    ++count_;

    index_->Set(node->mountPath.data(), node);
}

void FileMount::Remove(int id) {
    std::lock_guard<std::mutex> lock(mutex_);

    Node* node = head_;
    while (node != nullptr) {
        Node* const next = node->next;
        if (node->id == id) {
            index_->Delete(node->mountPath.data());

            if (node->previous != nullptr) {
                node->previous->next = node->next;
            } else {
                head_ = node->next;
            }
            if (node->next != nullptr) {
                node->next->previous = node->previous;
            } else {
                tail_ = node->previous;
            }

            node->previous = nullptr;
            node->next = nullptr;
            --count_;
            delete node;
        }
        node = next;
    }
}
