#pragma once

// DFPlayer Mini track mapping.
// IMPORTANT: With DFRobotDFPlayerMini::playMp3Folder(n), files must be stored on the SD card in folder:
//   /mp3
// and named using 4-digit numbers:
//   0001.mp3, 0002.mp3, ...
//
// Map your provided audio files (Vietnamese):
//  0001 chaomung.mp3
//  0002 quattudong.mp3
//  0003 remxuong.mp3
//  0004 remlen.mp3
//  0005 lua.mp3
//  0006 tatkhancap.mp3
//  0007 khigas.mp3
//  0008 quangtrotat.mp3
//  0009 quangtrobat.mp3
//  0010 tatquat.mp3
//  0011 batquat.mp3
//  0012 dongcua.mp3
//  0013 khoidongkhongthanhcong.mp3
//  0014 khoidongthanhcong.mp3
//  0015 doimauden.mp3
//  0016 tatden.mp3
//  0017 batden.mp3
//  0018 mocua.mp3
//  0019 saimatkhau.mp3
//  0020 saithe.mp3
//  0021 rfidmatkhau.mp3
//  0022 (reserved) hostage/emergency alarm (user will add)
//  0023 co_nguoi_truoc_cua.mp3  ("Phat hien co nguoi dung truoc cua, vui long mo cua hoac quet the...")

namespace audio_tracks {

static const uint16_t CHAOMUNG = 1;
static const uint16_t QUATTUDONG = 2;
static const uint16_t REMXUONG = 3;
static const uint16_t REMLEN = 4;
static const uint16_t LUA = 5;
static const uint16_t TATKHANCAP = 6;
static const uint16_t KHIGAS = 7;
static const uint16_t QUANGTROTAT = 8;
static const uint16_t QUANGTROBAT = 9;
static const uint16_t TATQUAT = 10;
static const uint16_t BATQUAT = 11;
static const uint16_t DONGCUA = 12;
static const uint16_t KHOIDONGKHONGTHANHCONG = 13;
static const uint16_t KHOIDONGTHANHCONG = 14;
static const uint16_t DOIMAUDEN = 15;
static const uint16_t TATDEN = 16;
static const uint16_t BATDEN = 17;
static const uint16_t MOCUA = 18;
static const uint16_t SAIMATKHAU = 19;
static const uint16_t SAITHE = 20;
static const uint16_t RFIDMATKHAU = 21;
static const uint16_t BAODONGDEDOA = 22;
static const uint16_t CNGUOI_TRUOC_CUA = 23;

} // namespace audio_tracks
