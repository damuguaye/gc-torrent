
#pragma once
#ifndef TORRENT_FILE_DOWNLOAD_BENCODE_H
#define TORRENT_FILE_DOWNLOAD_BENCODE_H

#include<iostream>
#include<vector>
#include<string>
#include<memory>
#include<variant>
#include<unordered_map>



namespace bencode {

    enum class ErrorClass {
        NoError,
        ErrNum,
        ErrCol,
        ErrEpI,
        ErrEpE,
        ErrTyp,
        ErrIvd,
        ErrNov,
        ErrOth
    };

    enum class BType {
        BSTR,
        BINT,
        BLIST,
        BDICT
    };

    class Error {
    public:
        Error() { //默认为 NoError
            this->err = ErrorClass::NoError;
            //this -> info = nullptr;
        }

        Error(ErrorClass e) {
            this->err = e;
            //this -> info = nullptr;
        }

        void set(ErrorClass e, const char* errInfo = nullptr);

        bool check() {
            return (this->err == ErrorClass::NoError);
        }

        void handleError();

    private:
        ErrorClass err;
        //std::string* info;
    };
    void Error::set(ErrorClass e, const char* errInfo) {
        this->err = e;
        if (errInfo) std::cerr << errInfo << std::endl;
    }

    void Error::handleError() {
        //if (info && !info->empty()) std::cerr << "info : " << info << std::endl;
        switch (this->err)
        {
        case ErrorClass::NoError: {
            std::cerr << "No Error" << std::endl;
            return;
        }
        case ErrorClass::ErrCol: {
            std::cerr << "expect colon" << std::endl;
            exit(-1);
        }
        case ErrorClass::ErrEpE: {
            std::cerr << "expect char e" << std::endl;
            exit(-1);
        }
        case ErrorClass::ErrEpI: {
            std::cerr << "expect char i" << std::endl;
            exit(-1);
        }
        case ErrorClass::ErrIvd: {
            std::cerr << "invalid bencode" << std::endl;
            exit(-1);
        }
        case ErrorClass::ErrNum: {
            std::cerr << "expect num" << std::endl;
            exit(-1);
        }
        case ErrorClass::ErrTyp: {
            std::cerr << "wrong type" << std::endl;
            exit(-1);
        }
        case ErrorClass::ErrNov: {
            std::cerr << "No value" << std::endl;
            exit(-1);
        }
        case ErrorClass::ErrOth: {
            std::cerr << "Other error" << std::endl;
            exit(-1);
        }

        default:
            std::cerr << "err of class Error error" << std::endl;
            exit(-1);
        }
    }



    class BObject {



    public:

        using LIST = std::vector<std::shared_ptr<BObject>>;
        using DICT = std::vector<std::pair<std::string, std::shared_ptr<BObject>>>;   
        using BValue = std::variant<int64_t, std::string, LIST, DICT>;

        BObject() = default; // 默认构造函数

        // 隐式转化五件套
        BObject(std::string); // 类型为BSTR的构造函数

        BObject(const char* str);

        BObject(int64_t v);

        BObject(LIST list);

        BObject(DICT dict);

        operator std::string(); //转化函数，将此类转化为string
         // 强制类型转换函数，当进行强制类型转换到string时生效

        operator int64_t();

        BObject& operator=(int64_t);//即=运算符重载
        //前面加上&主要是用于连续赋值
        //如 a = b = c

        BObject& operator=(std::string);

        BObject& operator=(LIST);

        BObject& operator=(DICT);
        /*

        BObject& operator+(int64_t);

        BObject& operator+(std::string);

        BObject& operator+(LIST);

        BObject& operator+(DICT);
        */

        std::string* Str(Error* Error); //Bobject转换为string

        int64_t* Int(Error* error);

        LIST* List(Error* error);

        DICT* Dict(Error* error);

        int64_t Bencode(std::ostream& os, Error* error); //BObject -> 输出流 字符

        static std::shared_ptr<BObject> Parse(std::istream& in, Error* error);
        // static 静态函数，只能在本文件中使用

        static int64_t EncodeString(std::ostream& os, std::string_view val);
        // 使用string，当length小于16时，会在栈上分配内存，大于16会在堆上分配
        // string_view 没有堆内存的分配，但不可修改，可将其转换为string后修改

        static std::string DecodeString(std::istream& in, Error* error);

        static int64_t EncodeInt(std::ostream& os, int64_t val);

        static int64_t DecodeInt(std::istream& in, Error* error);

        static int64_t getIntLen(int64_t val);


        using UMAPDICT = std::unordered_map<std::string, std::shared_ptr<BObject>>;

        void DICTVectorToUnorderedMap(UMAPDICT* umapdict,Error* error);

        friend void ToTorrent(BObject &obj, int64_t torrent);



        //private:
        BType type;
        BValue value;
    };

    std::string* BObject::Str(Error* error) {
        if (this->type != BType::BSTR) {
            if (error->check()) error->set(ErrorClass::ErrTyp);
            //error -> handleError("BObject to string error");
            return nullptr;
        }

        return std::get_if<std::string>(&this->value);
        // std::get_if<I>(&v) 如果变体类型 v 存放的数据类型下标为 I，那么返回所存放数据的指针，否则返回空指针。
        // std::get_if<T>(&v) 如果变体类型 v 存放的数据类型为 T，那么返回所存放数据的指针，否则返回空指针。
    }


    int64_t* BObject::Int(Error* error) {
        if (this->type != BType::BINT) {
            if (error->check()) error->set(ErrorClass::ErrTyp);
            //error -> handleError("BObject to int error");
            return nullptr;
        }

        return std::get_if<int64_t>(&this->value);
    }

    BObject::LIST* BObject::List(Error* error) {
        if (this->type != BType::BLIST) {
            if (error->check()) error->set(ErrorClass::ErrTyp);
            //error -> handleError("BObject to list error");
            return nullptr;
        }

        return std::get_if<BObject::LIST>(&this->value);
    }

    BObject::DICT* BObject::Dict(Error* error) {
        if (this->type != BType::BDICT) {
            if (error->check()) error->set(ErrorClass::ErrTyp);
            //error -> handleError("BObject to dict error");
            return nullptr;
        }

        return std::get_if<BObject::DICT>(&this->value);
    }

    int64_t BObject::Bencode(std::ostream& os, Error* error) {
        int64_t wlen = 0;

        if (!os) return wlen;

        void* val;
        switch (this->type) {
        case BType::BSTR: {
            val = this->Str(error);
            wlen += EncodeString(os, *((std::string*)val));
            break;
        }
        case BType::BINT: {
            val = this->Int(error);
            wlen += EncodeInt(os, *((int64_t*)val));
            break;
        }
        case BType::BLIST: {
            val = this->List(error);
            os << 'l';
            ++wlen;
            for (auto& item : *((LIST*)val)) {
                wlen += item->Bencode(os, error);
                if (!error->check()) return wlen;
            }
            os << 'e';
            ++wlen;
            break;
        }
        case BType::BDICT: {
            val = this->Dict(error);
            os << 'd';
            ++wlen;
            auto xx = *((DICT*)val);


            for (auto item : *((DICT*)val)) {
                wlen += EncodeString(os, item.first);
                wlen += item.second->Bencode(os, error);
                if (!error->check()) return wlen;
            }

            os << 'e';
            ++wlen;
            break;
        }
        }
        //os.flush();
        return wlen;
    }

    int64_t BObject::getIntLen(int64_t val) {
        int64_t len = 1;
        int64_t bigger = 10;
        while (bigger <= val) {
            bigger *= 10;
            ++len;
        }
        return len;
    }

    int64_t BObject::EncodeInt(std::ostream& os, int64_t val) {
        os << 'i';
        int64_t wlen = 1;
        if (val < 0) {
            val *= -1;
            os << '-';
            ++wlen;
        }
        os << val << 'e';
        wlen += (getIntLen(val) + 1);
        return  wlen;
    }

    int64_t BObject::EncodeString(std::ostream& os, std::string_view val) {

        int64_t string_length = val.length();
        int64_t wlen = getIntLen(string_length);
        os << string_length;
        os << ':';
        ++wlen;
        os << val;
        wlen += string_length;
        return wlen;
    }

    std::shared_ptr<BObject> BObject::Parse(std::istream& in, Error* error) {
        char c = in.peek();
        BObject* obj = new BObject();

        if (std::isdigit(c)) {
            auto str = BObject::DecodeString(in, error);
            if (!error->check() || str.empty()) {
                if (error->check()) error->set(ErrorClass::ErrNov, "Parse string error");
                return nullptr;
            }
            obj->type = BType::BSTR;
            obj->value = std::move(str);
        }
        else if (c == 'i') {
            auto integer = BObject::DecodeInt(in, error);
            if (!error->check() || integer == 0) {
                if (error->check()) error->set(ErrorClass::ErrNov, "Parse int error");
                return nullptr;
            }
            obj->type = BType::BINT;
            obj->value = integer;
        }
        else if (c == 'l') {
            in.get();
            BObject::LIST list;
            while (in.peek() != 'e') {

                list.emplace_back(Parse(in, error));
                if (!error->check() || !list.back()) {
                    if (error->check()) error->set(ErrorClass::ErrNov, "Parse list error");
                    return nullptr;
                }
            }
            obj->type = BType::BLIST;
            obj->value = std::move(list);
            in.get(); //读掉'e'
        }
        else if (c == 'd') {
            in.get(); //'d'
            BObject::DICT dict;
            std::string key;
            std::shared_ptr<BObject> val;
            while (in.peek() != 'e') {
                key = BObject::DecodeString(in, error);
                if (!error->check() || key.empty()) {
                    if (error->check()) error->set(ErrorClass::ErrNov, "Parse string(dict-key) error!");
                    return nullptr;
                }
                //std::cout<<"key: "<<key<<std::endl;
                val = Parse(in, error);
                if (!error->check() || !val) {
                    if (error->check()) error->set(ErrorClass::ErrNov, "Parse dict-value error!");
                    return nullptr;
                }
                dict.emplace_back(std::pair<std::string, std::shared_ptr<BObject>>(std::move(key), std::move(val)));
            }
            
            obj->type = BType::BDICT;
            obj->value = std::move(dict);
            in.get();//'e'
        }
        else {
            if (error->check()) error->set(ErrorClass::ErrIvd, "Invalid Parse error!");
            return nullptr;
        }
        return std::shared_ptr<BObject>(obj);
    }

    int64_t BObject::DecodeInt(std::istream& in, Error* error) {
        char c;
        in.get(c);


        int64_t val;
        if (c != 'i') {
            if (error->check()) error->set(ErrorClass::ErrEpI, "Decode Int error");
            return 0;
        }


        in >> val;
        in.get(c);
        if (c != 'e') {
            if (error->check()) error->set(ErrorClass::ErrEpE, "Decode Int error");
            return 0;
        }
        return val;
    }

    std::string BObject::DecodeString(std::istream& in, Error* error) {
        int64_t len = 0;
        int64_t decodedlen = -1;
        char c = in.peek();
        if (!isdigit(c)) {
            if (error->check()) error->set(ErrorClass::ErrNum, "Decode String(digit) error");
        }
        in >> len;
        if (len <= 0) {
            if (error->check()) error->set(ErrorClass::ErrNum, "Decode String(len) error");
        }
        std::string s;
        s.reserve(len);
        in.get(c);
        if (c != ':') {
            if (!error->check()) error->set(ErrorClass::ErrCol, "Decode String(:) error");
            return "";
        }
        while (++decodedlen < len && in.get(c)) {
            s.push_back(c);
        }
        if (decodedlen != len) {
            if (error->check()) error->set(ErrorClass::ErrIvd, "Decode String(decodelen) error");
            return "";
        }
        return s;
    }

    BObject& BObject::operator=(int64_t val) {
        this->type = BType::BINT;
        this->value = val;
        return *this;
    }
    BObject& BObject::operator=(std::string str) {
        this->type = BType::BSTR;
        this->value = std::move(str);
        return *this;
    }

    BObject& BObject::operator=(LIST list) {
        this->type = BType::BLIST;
        this->value = std::move(list);
        return *this;
    }

    BObject& BObject::operator=(DICT dict) {
        this->type = BType::BDICT;
        this->value = std::move(dict);
        return *this;
    }





}








#endif 