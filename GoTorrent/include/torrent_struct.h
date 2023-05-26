struct TorrentStruct{
    long long length;
    int piece_length;
    char* announce;
    int announce_length;
    char* infoSHA;
    char* name;
    int name_length;
    char* pieceSHA;  
    long long piece_count;
};

struct TrackRespStruct{
    int interval;
    int peers_length;
    char* peers;
    int peers6_length;
    char* peers6;
};