#include "app_mp3_player.h"

AppMp3PlayerClass AppMp3Player;

void audio_eof_mp3(const char* filename) { AppMp3Player.audio_eof_cb(filename); }
