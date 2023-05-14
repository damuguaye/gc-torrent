#pragma once
#ifdef BUILD_DLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

#include "sha1.h"
#include "bencode.h"
#include "marshal.h"
#include "torrent_struct.h"



extern "C" {
	
	TorrentStruct* getTorrentFile(char* path, Error* error = nullptr);
	TorrentStruct* getTorrentStr(std::string str, Error* error = nullptr);
	TrackRespStruct* getPeers(char* path, Error* error = nullptr);
	char* addCharArray(char* c, int n);

}