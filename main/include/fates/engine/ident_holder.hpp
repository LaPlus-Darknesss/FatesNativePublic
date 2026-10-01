#pragma once

#include "fates/engine/bind_manager.hpp"

class IdentHash;

class IdentHolder {
public:
    IdentHolder();

    bool Bind(const char* identifier);
    bool Unbind(const char* identifier);
    void SetPtr(const char* identifier, void* value);
    bool IsExist(const char* identifier) const;

private:
    struct BindingEntry {
        BindingEntry* previous{};
        BindingEntry* next{};
        void* owner{};
        BindManager bindings{};
        char* identifier{};
        void* pointer{};
    };

    struct List {
        BindingEntry* head{};
        BindingEntry* tail{};
        int count{};
    };

    IdentHash* index_{};
    List* entries_{};
};
