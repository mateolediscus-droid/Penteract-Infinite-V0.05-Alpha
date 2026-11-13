#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <istream>
#include <ostream>
#include <map>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace GenerationHistory {
    inline void put(std::ostream& out,uint64_t value,unsigned bytes) {
        for(unsigned i=0;i<bytes;++i)out.put(char((value>>(8*i))&255));
    }
    inline uint64_t get(std::istream& in,unsigned bytes) {
        uint64_t value=0;
        for(unsigned i=0;i<bytes;++i) {
            int c=in.get();if(c==std::char_traits<char>::eof())throw std::runtime_error("Truncated generation history");
            value|=uint64_t(uint8_t(c))<<(8*i);
        }
        return value;
    }
    template<size_t N> using Records=std::map<std::array<int32_t,N>,size_t>;
    // Little endian: magic, version:u16, dimensions:u16, two counts:u64,
    // then column records and legacy records, each N*i32 + revision:u64.
    template<size_t N>
    void write(std::ostream& out,const Records<N>& columns,const Records<N>& legacy) {
        out.write("PGENHST\n",8);put(out,1,2);put(out,N,2);
        put(out,columns.size(),8);put(out,legacy.size(),8);
        for(const auto* records:{&columns,&legacy})for(const auto& [p,revision]:*records) {
            for(auto n:p)put(out,uint32_t(n),4);
            put(out,revision,8);
        }
        out.flush();if(!out)throw std::runtime_error("Cannot save generation history");
    }
    template<size_t N>
    void read(std::istream& in,Records<N>& columns,Records<N>& legacy,size_t catalogCount) {
        char magic[8]{};in.read(magic,8);
        if(std::string_view(magic,8)!="PGENHST\n" || get(in,2)!=1 || get(in,2)!=N)
            throw std::runtime_error("Invalid generation history format");
        uint64_t count=get(in,8),oldCount=get(in,8);
        auto begin=in.tellg();in.seekg(0,std::ios::end);auto end=in.tellg();in.seekg(begin);
        if(begin<0 || end<begin || count>UINT64_MAX-oldCount ||
            uint64_t(end-begin)%(N*4+8)!=0 || (count+oldCount)!=uint64_t(end-begin)/(N*4+8))
            throw std::runtime_error("Invalid generation history length");
        Records<N> newColumns,newLegacy;
        auto load=[&](Records<N>& records,uint64_t amount) {
            for(uint64_t i=0;i<amount;++i) {
                std::array<int32_t,N> p{};
                for(auto& n:p)n=std::bit_cast<int32_t>(uint32_t(get(in,4)));
                uint64_t revision=get(in,8);
                if(revision>=catalogCount)throw std::runtime_error("Invalid generation history revision");
                if(!records.emplace(p,size_t(revision)).second)throw std::runtime_error("Duplicate generation history coordinate");
            }
        };
        load(newColumns,count);load(newLegacy,oldCount);
        columns.swap(newColumns);legacy.swap(newLegacy);
    }
}
