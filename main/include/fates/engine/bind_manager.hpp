#pragma once

class BindManager {
public:
    BindManager();

    // Returns true when this transitions from zero to one binding.
    bool Bind();

    // Returns true when this releases the final binding.
    bool Unbind();

    int GetCount() const { return count_; }

private:
    int count_{};
};
