#pragma once

#include <stdint.h>

class DFPlayerMgr;

// Simple global audio hook so any module (gateway/main) can play tracks
// without owning the DFPlayer instance.
void audioBegin(DFPlayerMgr& mgr);
bool audioReady();
void audioPlay(uint16_t track);
