#pragma once

#include "IPC.h"

bool ConnectIPC();
void DisconnectIPC();

bool SetFeature(IPCCommand command, bool enabled);