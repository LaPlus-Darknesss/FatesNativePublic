#pragma once

class PrivateSkill {
public:
    // PROVEN: exact identifier lookup through the private-skill IdentHash;
    // retail uses GetSurely semantics (identifier is expected to exist).
    static const PrivateSkill* Get(const char* identifier);
};
