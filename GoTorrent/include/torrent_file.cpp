
#include <iostream>
#include <fstream>
#include <sstream>
#include "sha1.h"
#include "bencode.h"
#include "marshal.h"
#include "torrent_file.h"

using namespace std;

using bencode::BObject;
using bencode::Error;


shared_ptr<BObject> is2BObj(istream& in, Error* error) {
    shared_ptr<BObject> obj;
    obj = BObject::Parse(in, error);
    error -> handleError();
    return obj;

}

TorrentStruct* tf2ts(TorrentFile *tf){
    TorrentStruct *ts = new TorrentStruct;

    ts -> length = tf -> length;
    ts -> piece_length = tf -> piece_length;
    ts -> piece_count = tf -> piece_count;
    ts -> infoSHA = tf -> infoSHA;
    ts -> pieceSHA = tf -> pieceSHA;
    
    ts -> announce_length = tf -> announce.size();
    ts -> announce = new char[ts->announce_length+1];

    strcpy_s(ts->announce, ts->announce_length+1, std::move(tf -> announce.c_str()));

    ts -> name_length = tf -> name.size();
    ts -> name = new char[ts->name_length+1];
    strcpy_s(ts->name, ts->name_length+1, std::move(tf -> name.c_str()));

    

    
    return ts;
}

extern "C"{
    TorrentFile getTorrent(shared_ptr<BObject> obj, Error* error){
        if(obj == nullptr){
            if (error->check()) error->set(ErrorClass::ErrNov, "BObject NULL");
            error -> handleError();         
        }
        TorrentFile torrent =  Unmarshal(obj, error);
        return std::move(torrent);
    }

    TorrentStruct* getTorrentFile(char* path, Error* error){
        if(!error) error = new Error();
        string rpath(path);
        std::cout<<rpath<<'\n';
        fstream tfile;
        tfile.open(rpath, ios::in | ios::binary);
        if (!tfile.is_open()) cerr << "file open failed" << endl;

        shared_ptr<BObject> obj = is2BObj(tfile, error);

        TorrentFile *torrent = new TorrentFile(getTorrent(obj, error));

        auto ts = tf2ts(torrent);

        std::cout<<ts->announce<<std::endl;
        return ts;
    }

    TorrentStruct* getTorrentStr(string str, Error* error){
        if(!error) error = new Error();
        if(str == ""){
            if (error->check()) error->set(ErrorClass::ErrNov, "Torrent Str NULL");
            error -> handleError(); 
        }
        std::stringstream ss;
        ss << str;
        shared_ptr<BObject> obj = is2BObj(ss, error);
        TorrentFile *torrent = new TorrentFile(getTorrent(obj, error));
        return tf2ts(torrent);
    }



    TrackRespStruct* getPeers(char* path, Error* error){    
        if(!error) error = new Error();
        fstream tfile;
        string rpath(path);
        tfile.open(rpath, ios::in | ios::binary);
        if (!tfile.is_open()) cerr << "file open failed" << endl;


        
       
        shared_ptr<BObject> obj = is2BObj(tfile, error);

        BObject::UMAPDICT* umapdict = new BObject::UMAPDICT;
        obj -> DICTVectorToUnorderedMap(umapdict, error); 

        TrackRespStruct* res = new TrackRespStruct;
        
        auto val = (*umapdict)["interval"];
        if(!val){
            if (error->check()) error->set(ErrorClass::ErrNov, "found res.interval error");
            return res;
        }
        res->interval = std::get<int64_t>(val -> value);
        
        val = (*umapdict)["peers"];
        if(!val){
            if (error->check()) error->set(ErrorClass::ErrNov, "found res.peers error");
            return res;
        }

        
        string peers_s;
        if(val->type == bencode::BType::BSTR){
            peers_s = std::get<std::string>(val -> value);
        }else if(val->type == bencode::BType::BLIST){
            cout<<"PEERS: "<<endl;
            // 转换为*{ip}&{port}*...的形式
            peers_s += "*";
            auto tplist = std::get<BObject::LIST>(val -> value);
            string tpstr;
            int64_t tpint;
            for(auto &item : tplist){
                auto tpdict = std::get<BObject::DICT>(item -> value);
                
                for(auto &it : tpdict){
                    if(it.first == "ip"){ // skip "peer id"
                        cout<<it.first<<" : ";
                        tpstr = std::get<std::string>(it.second->value);
                        peers_s += tpstr;
                        peers_s += "&";
                        cout<<tpstr<<"   ";
                    }else if(it.first == "port"){
                        cout<<it.first<<" : ";
                        tpint = get<int64_t>(it.second->value);
                        peers_s += to_string(tpint);
                        peers_s += "*";
                        cout<<tpint<<"   ";
                    }
                    
                }
                cout<<std::endl;
            }
            
        }
            
        
        res->peers_length = peers_s.size();
        std::cout<<res->peers_length<<"\n\n\n\n";
        res->peers = new char[peers_s.size() + 1];
        strcpy_s(res->peers, peers_s.size() + 1, peers_s.c_str());

        val = (*umapdict)["peers6"];
        if(!val){
            if (error->check()) error->set(ErrorClass::ErrNov, "found res.peers6 error");
            return res;
        }
        peers_s = std::get<std::string>(val -> value);
        res->peers6_length = peers_s.size();
        res->peers6 = new char[peers_s.size() + 1];
        strcpy_s(res->peers6, peers_s.size() + 1, peers_s.c_str());


        return res;
    }

    char* addCharArray(char* c, int64_t n){
		c += n;
		return c;
	}

}




