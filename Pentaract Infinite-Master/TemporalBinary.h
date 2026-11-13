#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <istream>
#include <ostream>
#include <map>
#include <set>
#include <stdexcept>
#include <type_traits>

// Fixed-width little-endian fields, never native struct padding.
namespace temporalBinary {
class Writer {
    std::ostream& stream;
public:
    explicit Writer(std::ostream& stream):stream(stream) {}
    template<class... T> void operator()(const T&... value) {(write(value),...);}
    template<class T> void write(const T& value) {
        if constexpr(std::is_floating_point_v<T>) write(std::bit_cast<uint64_t>(double(value)));
        else if constexpr(std::is_integral_v<T>) {
            uint64_t n=uint64_t(value);
            for(size_t i=0;i<sizeof(T);++i)stream.put(char(n>>(8*i)));
        } else if constexpr(requires {value.archive(*this);}) value.archive(*this);
        else if constexpr(requires {const_cast<T&>(value).archive(*this);}) const_cast<T&>(value).archive(*this);
        else if constexpr(requires {typename T::key_type;}) {
            write(uint64_t(value.size()));
            for(const auto& entry:value) {
                if constexpr(requires {entry.first;entry.second;}) {write(entry.first);write(entry.second);}
                else write(entry);
            }
        } else for(const auto& entry:value)write(entry);
        if(!stream)throw std::runtime_error("Cannot write temporal save");
    }
};
class Reader {
    std::istream& stream;
    uint64_t remaining;
public:
    Reader(std::istream& stream,uint64_t bytes):stream(stream),remaining(bytes) {}
    template<class... T> void operator()(T&... value) {(read(value),...);}
    template<class T> void read(T& value) {
        if constexpr(std::is_floating_point_v<T>) {uint64_t n;read(n);value=std::bit_cast<double>(n);}
        else if constexpr(std::is_integral_v<T>) {
            if(remaining<sizeof(T))throw std::runtime_error("Truncated temporal save");
            uint64_t n=0;for(size_t i=0;i<sizeof(T);++i) {int c=stream.get();if(c<0)throw std::runtime_error("Truncated temporal save");n|=uint64_t(c)<<(8*i);}
            remaining-=sizeof(T);value=T(n);
        } else if constexpr(requires {value.archive(*this);}) value.archive(*this);
        else if constexpr(requires {typename T::key_type;}) {
            uint64_t count;read(count);
            if(count>remaining)throw std::runtime_error("Invalid temporal record count");
            value.clear();
            for(uint64_t i=0;i<count;++i) {
                typename T::key_type key{};read(key);
                if constexpr(requires {typename T::mapped_type;}) {typename T::mapped_type item{};read(item);if(!value.emplace(key,std::move(item)).second)throw std::runtime_error("Duplicate temporal record");}
                else if(!value.insert(key).second)throw std::runtime_error("Duplicate temporal record");
            }
        } else for(auto& entry:value)read(entry);
    }
    void finish()const {if(remaining)throw std::runtime_error("Unexpected temporal save data");}
};
}
