#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <bitset>
#include <stdlib.h>
#include <functional>
#include "bencode.h"

#define SHA1_RESULT_BIT 160
#define SHA1_RESULT_UINT8 20 //(SHA1_RESULT_BIT / 8)         // 20
#define SHA1_RESULT_UINT32 5 //(SHA1_RESULT_UINT8 / 4)      // 5

#define SHA1_BLOB_BIT 512
#define SHA1_BLOB_UINT8 64 //(SHA1_BLOB_BIT / 8)             // 64
#define SHA1_BLOB_UINT32 16 //(SHA1_BLOB_UINT8 / 4)          // 16

#define SHA1_REST_BIT 448
#define SHA1_REST_UINT8 56 //(SHA1_REST_BIT / 8)             // 56
#define SHA1_REST_UINT32 14 //(SHA1_REST_UINT8 / 4)          // 14

#define SHA1_OPERATION_CNT 80
#define SHA1_ROUND_CNT 4
#define SHA1_ROUND_LEN 20 //(SHA1_OPERATION_CNT / SHA1_ROUND_CNT) // 20

//typedef std::bitset<SHA1_BLOB_BIT> wlen;
using Error = bencode::Error;

class SHA1
{
public:
    SHA1();
    SHA1(std::string *s);


    void Init(); //初始化
    void getStr(std::string *s);
    void resetABCDE();
    uint32_t strToU32(char * ch);
    uint32_t rotl_uint_32(uint32_t x, int n);
    uint32_t f(uint32_t x, uint32_t y, uint32_t z, int t);  
    void caculateOnce();
    void complement();
    void hashString();
    uint32_t* getSHA1();
    char* getStrSHA1();

    char* hash(std::string *s = nullptr);   
    char* hashStream(std::istream& in);

private:
    uint32_t _H[SHA1_RESULT_UINT32];
    uint32_t _K[SHA1_ROUND_CNT];
    uint32_t _data[SHA1_BLOB_BIT];
    uint32_t W[SHA1_OPERATION_CNT];


    uint32_t A;
    uint32_t B;
    uint32_t C;
    uint32_t D;
    uint32_t E;
    uint32_t T;


    int64_t strLen; //原始数据总长度
    int64_t length; //最终总长度
    int64_t pieces; //补足后长度除以512

    std::string *str;

};

void SHA1::getStr(std::string *s){
    str = s;
}

SHA1::SHA1()
{
    Init();
}

SHA1::SHA1(std::string *s){
    Init();
    getStr(s);
}

void SHA1::resetABCDE(){
    A = _H[0];
    B = _H[1];
    C = _H[2];
    D = _H[3];
    E = _H[4];
}

void SHA1::Init(){
    _H[0] = 0x67452301;
    _H[1] = 0xEFCDAB89;
    _H[2] = 0x98BADCFE;
    _H[3] = 0x10325476;
    _H[4] = 0xC3D2E1F0;

    _K[0] = 0x5A827999;
    _K[1] = 0x6ED9EBA1;
    _K[2] = 0x8F1BBCDC;
    _K[3] = 0xCA62C1D6;

    resetABCDE();

    str = nullptr;

}

uint32_t SHA1::rotl_uint_32(uint32_t x, int n){
    return (x << n) | (x >> (32 - n));
}

void SHA1::complement(){
    
    
    strLen = str -> length();
    int64_t len = strLen + 1;
    int remainder = len % SHA1_BLOB_UINT8;


    length = remainder > SHA1_REST_UINT8 ? 
        (len - remainder + 2 * SHA1_BLOB_UINT8) : (len - remainder + SHA1_BLOB_UINT8);

    str -> reserve(length);

    str -> push_back(0x80); //len = strLen + 1

    for(; len < length - 8; ++len){
        str -> push_back(0x00);
    }

    int64_t slen = strLen * 8;
    
    for(int i = SHA1_REST_UINT8; i >= 0; i -= 8){
        str -> push_back(0xFF & (slen >> i));
    }

    pieces = str -> length() / SHA1_BLOB_UINT8;

}



uint32_t SHA1::f(uint32_t x, uint32_t y, uint32_t z, int t){
    if(t <= 19) return (x & y) | (~x & z);
    else if(t <= 39) return x ^ y ^ z;
    else if(t <= 59) return (x & y) | (x & z) | (y & z);
    else return x ^ y ^ z;
}


uint32_t SHA1::strToU32(char *ch){
    uint32_t W = 0;
    for(int i = 0; i < 3; ++i){

        W |= (unsigned char)(*ch);
        
        W <<= 8;
        ++ch;
    }
    W |= (unsigned char)(*ch); 

    return W;
}


void SHA1::hashString(){
    
    char* ch = &((*str)[0]);



    for(int64_t i = 0; i < pieces; ++i){
        resetABCDE();

        for(int j = 0; j < SHA1_BLOB_UINT32; ++j){
            W[j] = strToU32(ch);
            ch += 4;
        }

        caculateOnce();
        
    }
    


}

void SHA1::caculateOnce(){
    for(int j = SHA1_BLOB_UINT32; j < SHA1_OPERATION_CNT; ++j){
        W[j] = rotl_uint_32((W[j - 3] ^ W[j - 8] ^ W[j - 14] ^ W[j - 16]), 1);
    }
    for(int t = 0; t < SHA1_OPERATION_CNT; ++t){
        T = rotl_uint_32(A, 5) + f(B, C, D, t) + E + _K[t / 20] + W[t];
        E = D;
        D = C;
        C = rotl_uint_32(B, 30);
        B = A;
        A = T;

    }
    _H[0] += A;
    _H[1] += B;
    _H[2] += C;
    _H[3] += D;
    _H[4] += E;
}


uint32_t* SHA1::getSHA1(){
    for(int i = 0; i < SHA1_RESULT_UINT32; ++i){
        std::cout<<std::hex<<_H[i];
    }
    std::cout<<std::endl;
    return _H;
} 

char* SHA1::getStrSHA1(){
    char * ret = new char[SHA1_RESULT_UINT8];
    for(int i = 0; i < SHA1_RESULT_UINT32; ++i){
        for(int j = 0; j < 4; ++j){
  
            ret[i*4+j] = 0xFF & (_H[i] >> (24 - j * 8));
        }
    }



    return ret;
}

char* SHA1::hash(std::string *s){
    this -> Init();
    
    if(s) str = s;
    if(str == nullptr){
        std::cerr<<"hash str is nullptr"<<std::endl;
        exit(-1);
    }

    this -> complement();
    this -> hashString();
    str -> resize(strLen);
    return this -> getStrSHA1();

}

char* SHA1::hashStream(std::istream &in){
    this -> Init();
    strLen = 0;
    char c;
    uint32_t ww = 0;
    int W_count = 0;

    while(true){
        if(in.get(c))
        {
            ++strLen;
      
            
            ww |= (c & 0xFF); 
          
            if(strLen % 4){
                ww <<= 8;
                
            }else{
                W[W_count++] = ww;
                ww = 0;
                if(W_count == SHA1_BLOB_UINT32){
                    resetABCDE();
                    caculateOnce();
                    W_count = 0;
                }
            }
        }
        else{
            c = 0x80;
            ++strLen;
            ww |= (c & 0xFF); 
            
            if(strLen % 4){
                ww <<= ((4 - (strLen % 4)) * 8);
            }
       
            W[W_count++] = ww;
            
            if(W_count == SHA1_BLOB_UINT32){
                resetABCDE();
                caculateOnce();
                W_count = 0;
            }
            
            break;
        }
        
    }
    
    int64_t count = strLen;
    if(count % 4)
        count += (4 - (count % 4));
    --strLen;
    int remainder = count % SHA1_BLOB_UINT8;

    length = remainder > SHA1_REST_UINT8 ? 
    (count - remainder + 2 * SHA1_BLOB_UINT8) : (count - remainder + SHA1_BLOB_UINT8);
       

    for(int64_t i = count; i < length - 8; i += 4){
        W[W_count++] = 0;

        if(W_count == SHA1_BLOB_UINT32){
            resetABCDE();
            caculateOnce();
            W_count = 0;
        }
    }
    int64_t strLen64 = strLen * 8;
    std::cout<<"char len: "<<strLen<<std::endl;
    std::cout<<"length64:   "<<std::dec<<strLen64<<std::endl;   
    W[W_count++] = (strLen64 >> 32) & 0xFFFFFFFF;
    W[W_count] = strLen64 & 0xFFFFFFFF;
    resetABCDE();
    caculateOnce();


    return this -> getStrSHA1();

}




