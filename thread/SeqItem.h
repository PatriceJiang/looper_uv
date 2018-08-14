#pragma once

#include <cstdint>
#include <string>
template<typename T>
struct SeqItem{
    SeqItem(uint64_t id, const std::string &name, T &data): data(data), id(id), name(name){}
    SeqItem(uint64_t id, const std::string &name, T &&data): data(data), id(id), name(name){}
    SeqItem(uint64_t id, T &data) : data(data), id(id){}
    SeqItem(uint64_t id, T &&data) : data(data), id(id){}
    SeqItem(const SeqItem &d):data(d.data), id(d.id), name(d.name){}
    T data;
    uint64_t id;
    std::string name{""};
};

