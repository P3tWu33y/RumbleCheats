#pragma once

// AntiDebug
// Lightweight debugger detection + code-section integrity check.
//
// This is a deterrent, not a guarantee -- a sufficiently motivated
// attacker with a kernel debugger or a patched checker can defeat any
// of this. It exists to raise the cost of casual tampering with the
// login/license flow, not to make it impossible.

#include <cstdint>
#include "ThemidaSDK.h"

namespace AntiDebug
{
    // Returns true if a user-mode debugger is detected via any of
    // several independent checks. Cheap, call freely.
    bool IsDebuggerDetected();

    // Computes a CRC32 over this module's .text (code) section as it
    // currently exists in memory, and compares it against the expected
    // value baked in at build time. Returns false if they don't match
    // (code was patched) or if the section couldn't be located.
    bool VerifyCodeIntegrity();

    // Combines both checks. Call this around/before the login and
    // license-check flow. Does not report which check failed.
    bool SecurityCheckPassed();
}