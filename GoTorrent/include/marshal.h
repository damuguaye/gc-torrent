#pragma once


#include<unordered_map>
#include<vector>
#include<sstream>
#include<fstream>
#include "bencode.h"
#include "sha1.h"




using namespace bencode;

const int SHALEN = 20;

struct TorrentInfo {
    int64_t length;
    std::string name;
    int piece_length; //每个片段的长度
    char* pieces;
    int64_t piece_count; //length / piece_length
};

struct TorrentFile{
    std::string announce;
    char* infoSHA;
    std::string name;
    int64_t length;
    int piece_length;
    int64_t piece_count;
    char* pieceSHA;
};


void BObject::DICTVectorToUnorderedMap(BObject::UMAPDICT* umapdict, Error* error){
    if(this -> type != BType::BDICT) {
        if (error->check()) error->set(ErrorClass::ErrTyp, "DICT(vector) to UMAPDICT(unordered_map) failed");
        return;
    }
    DICT* dict = std::get_if<DICT>(&this -> value);
    if(!dict){
        if (error->check()) error->set(ErrorClass::ErrNov, "DICT(vector) to UMAPDICT(unordered_map) failed");
        return;
    }

    for(auto item : (*dict)){
        umapdict -> emplace(std::move(item));
    }
}


void ToTorrentInfo(BObject::UMAPDICT* umapdict, TorrentInfo &torrentInfo, Error* error){
    auto val = (*umapdict)["name"];
    if(!val){
        if (error->check()) error->set(ErrorClass::ErrNov, "found info-name error");
        return;
    }
    torrentInfo.name = std::get<std::string>(val -> value);

    val = (*umapdict)["length"];
    if(!val){
        if (error->check()) error->set(ErrorClass::ErrNov, "found info-length error");
        return;
    }
    torrentInfo.length = std::get<int64_t>(val -> value);

    val = (*umapdict)["piece length"];
    if(!val){
        if (error->check()) error->set(ErrorClass::ErrNov, "found info-piece length error");
        return;
    }
    torrentInfo.piece_length = std::get<int64_t>(val -> value);

    val = (*umapdict)["pieces"];
    if(!val){
        if (error->check()) error->set(ErrorClass::ErrNov, "found info-pieces error");
        return;
    }

    torrentInfo.piece_count = torrentInfo.length / torrentInfo.piece_length;
    if(torrentInfo.length % torrentInfo.piece_length) ++torrentInfo.piece_count;
    torrentInfo.pieces = new char[torrentInfo.piece_count * SHALEN + 1];
    std::string pieces = std::get<std::string>(val -> value);
    for(int64_t i = 0; i < pieces.size(); ++i){
        torrentInfo.pieces[i] = pieces[i];
    }

}

void ToTorrent(BObject::UMAPDICT* umapdict, TorrentFile &torrent, Error* error){
    auto val = (*umapdict)["announce"];
    if(!val){
        if (error->check()) error->set(ErrorClass::ErrNov, "found announce error");
        return;
    }
    torrent.announce = std::get<std::string>(val -> value);
    

    val = (*umapdict)["info"];
    if(!val || val -> type != BType::BDICT){
        if (error->check()) error->set(!val ? ErrorClass::ErrNov : ErrorClass::ErrOth, "found info error");
        return;
    }

    BObject::UMAPDICT* infodict = new BObject::UMAPDICT;
    val -> DICTVectorToUnorderedMap(infodict, error);

    TorrentInfo info;
    ToTorrentInfo(infodict, info, error);

    torrent.name = info.name;
    torrent.length = info.length;
    torrent.piece_length = info.piece_length;
    torrent.pieceSHA = info.pieces;
    torrent.piece_count = info.piece_count;

    SHA1 sha;
    std::string infoStr;
    std::stringstream ss;

    
    int64_t coutlen = val -> Bencode(ss, error);


    torrent.infoSHA = sha.hashStream(ss);
}

TorrentFile Unmarshal(std::shared_ptr<BObject> obj, Error* error){
    BObject::UMAPDICT* umapdict = new BObject::UMAPDICT;
    obj -> DICTVectorToUnorderedMap(umapdict, error);  
    TorrentFile torrent;
    ToTorrent(umapdict, torrent, error);
    return torrent;
}







