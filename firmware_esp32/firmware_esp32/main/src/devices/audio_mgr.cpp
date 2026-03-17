#include "audio_mgr.h"

#include "dfplayer_mgr.h"

static DFPlayerMgr* g_mgr = nullptr;

void audioBegin(DFPlayerMgr& mgr) {
  g_mgr = &mgr;
}

bool audioReady() {
  return g_mgr && g_mgr->ready();
}

void audioPlay(uint16_t track) {
  if (!audioReady()) return;
  g_mgr->play(track);
}
