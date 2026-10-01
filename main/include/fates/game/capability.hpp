#pragma once

class Capability {
public:
    enum class Type : int {};

    // PROVEN: retail indexes a capability-name message table and resolves the
    // entry through Mess::Get. The enum labels themselves are not named here
    // until the static table is source-owned.
    static const wchar_t* GetName(int capabilityIndex);
};
