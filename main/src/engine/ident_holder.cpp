#include "fates/engine/ident_holder.hpp"

#include "fates/engine/ident_hash.hpp"

#include <cstring>

IdentHolder::IdentHolder()
    : index_(new IdentHash(0x7F)),
      entries_(new List{}) {}

bool IdentHolder::Bind(const char* identifier) {
    if (identifier == nullptr || index_ == nullptr || entries_ == nullptr) {
        return false;
    }

    auto* entry = static_cast<BindingEntry*>(index_->GetSurely(identifier));
    if (entry == nullptr) {
        entry = new BindingEntry{};
        const std::size_t length = std::strlen(identifier);
        entry->identifier = new char[length + 1];
        std::memcpy(entry->identifier, identifier, length + 1);

        entry->previous = entries_->tail;
        if (entries_->tail != nullptr) {
            entries_->tail->next = entry;
        } else {
            entries_->head = entry;
        }
        entries_->tail = entry;
        ++entries_->count;
        index_->Set(entry->identifier, entry);
    }

    return entry->bindings.Bind();
}

bool IdentHolder::Unbind(const char* identifier) {
    if (identifier == nullptr || index_ == nullptr || entries_ == nullptr) {
        return false;
    }

    auto* entry = static_cast<BindingEntry*>(index_->GetSurely(identifier));
    if (entry == nullptr) {
        return false;
    }

    const bool releasedFinalBinding = entry->bindings.Unbind();
    if (!releasedFinalBinding) {
        return false;
    }

    index_->Delete(entry->identifier);
    if (entry->previous != nullptr) {
        entry->previous->next = entry->next;
    } else {
        entries_->head = entry->next;
    }
    if (entry->next != nullptr) {
        entry->next->previous = entry->previous;
    } else {
        entries_->tail = entry->previous;
    }
    --entries_->count;

    delete[] entry->identifier;
    delete entry;
    return true;
}

void IdentHolder::SetPtr(const char* identifier, void* value) {
    if (identifier == nullptr || index_ == nullptr) {
        return;
    }
    if (auto* entry = static_cast<BindingEntry*>(index_->GetSurely(identifier))) {
        entry->pointer = value;
    }
}

bool IdentHolder::IsExist(const char* identifier) const {
    return identifier != nullptr && index_ != nullptr &&
           index_->GetSurely(identifier) != nullptr;
}
