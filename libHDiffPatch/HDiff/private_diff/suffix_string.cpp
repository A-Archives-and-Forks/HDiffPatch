//suffix_string.cpp
//
/*
 The MIT License (MIT)
 Copyright (c) 2012-2017 HouSisong
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
*/

#include "suffix_string.h"
#include <assert.h>
#include <string.h> //memset
#include <stdexcept> //std::runtime_error
#include "../../../libParallel/parallel_import.h"
#if (_IS_USED_MULTITHREAD)
#include <thread>   //if used vc++, need >= vc2012
#endif
// Sorting method selection.
#ifndef _SA_SORTBY
#define _SA_SORTBY
//#  define _SA_SORTBY_STD_SORT
//#  define _SA_SORTBY_SAIS
#   define _SA_SORTBY_DIVSUFSORT
#endif//_SA_SORTBY

// Match search method selection, whether to use std::lower_bound or use custom implementation.
//#define _SA_MATCHBY_STD_LOWER_BOUND

#if (defined _SA_SORTBY_STD_SORT) || (defined _SA_MATCHBY_STD_LOWER_BOUND)
    #include <algorithm> //sort,lower_bound
    #include <execution> //std::execution::par_unseq: for C++17 parallel sort
    #include "qsort_parallel.h" //sort_parallel: a simple multi-thread sort; not optimized for sstring
    static const int kMaxCmpLength_sort=1024*4; //WARNING: This is a special handling measure to prevent _suffixString_create from degrading
                      //  to O(n*n) complexity in some cases when using std::sort (unacceptable runtime). Set a maximum comparison length to control the algorithm
                       //  Within O(kMaxCmpLength_sort*n) complexity, note that the sorted result is not a standard suffix array;
#endif
#ifdef _SA_SORTBY_SAIS
    #include "sais.hxx"
#endif
#ifdef _SA_SORTBY_DIVSUFSORT
    #include "libdivsufsort/divsufsort.h"
    #include "libdivsufsort/divsufsort64.h"
#endif

namespace hdiff_private{

template<class T>
static void _clearVector(std::vector<T>& v){
    std::vector<T> _tmp;
    v.swap(_tmp);
}

namespace {
    typedef TSuffixString::TInt   TInt;
    typedef TSuffixString::TInt32 TInt32;
    typedef TSuffixString::TChar  TChar;

    static bool getStringIsLess(const TChar* str0,const TChar* str0End,
                                const TChar* str1,const TChar* str1End){
        size_t L0=(size_t)(str0End-str0);
        size_t L1=(size_t)(str1End-str1);
    #ifdef _SA_SORTBY_STD_SORT
        L0=(L0<=kMaxCmpLength_sort)?L0:kMaxCmpLength_sort;
        L1=(L1<=kMaxCmpLength_sort)?L1:kMaxCmpLength_sort;
    #endif
        size_t LMin=(L0<L1)?L0:L1;
        for (size_t i=0; i<LMin; ++i){
            TInt sub=str0[i]-str1[i];
            if (!sub) continue;
            return sub<0;
        }
        return (L0<L1);
    }

    struct StringToken{
        const TChar* begin;
        const TChar* end;
        inline explicit StringToken(const TChar* _begin,const TChar* _end):begin(_begin),end(_end){}
    };

    class TSuffixString_compare{
    public:
        inline TSuffixString_compare(const TChar* begin,const TChar* end):m_begin(begin),m_end(end){}
        inline bool operator()(const TInt s0,const StringToken& s1)const{
            return getStringIsLess(m_begin+s0,m_end,s1.begin,s1.end);
        }
        inline bool operator()(const StringToken& s0,const TInt s1)const{
            return getStringIsLess(s0.begin,s0.end,m_begin+s1,m_end);
        }
        inline bool operator()(const TInt s0,const TInt s1)const{
            return getStringIsLess(m_begin+s0,m_end,m_begin+s1,m_end);
        }
    private:
        const TChar* m_begin;
        const TChar* m_end;
    };

    template<class TSAInt>
    static void _suffixString_create(const TChar* src,const TChar* src_end,
                                     std::vector<TSAInt>& out_sstring,size_t threadNum){
        TSAInt size=(TSAInt)(src_end-src);
        if (size<0)
            throw std::runtime_error("suffixString_create() error.");
        out_sstring.resize(size);
        if (size==0) return;
    #ifdef _SA_SORTBY_STD_SORT
        for (TSAInt i=0;i<(TSAInt)size;++i)
            out_sstring[i]=i;
        int rt=0;
        try {
            const TSuffixString_compare cmp(src,src_end);
            //std::sort(&out_sstring[0],&out_sstring[0]+size,cmp);
            //std::sort(std::execution::par_unseq,&out_sstring[0],&out_sstring[0]+size,cmp);//need C++17
            sort_parallel<TSAInt,const TSuffixString_compare&,1024*8,137>
                    (&out_sstring[0],&out_sstring[0]+size,cmp,threadNum);
        } catch (...) {
            rt=-1;
        }
    #endif
    #ifdef _SA_SORTBY_SAIS
        TSAInt rt=saisxx(src,&out_sstring[0],size);
    #endif
    #ifdef _SA_SORTBY_DIVSUFSORT
        saint_t rt=-1;
        if (sizeof(TSAInt)==8)
            rt=divsufsort64(src,(saidx64_t*)&out_sstring[0],(saidx64_t)size,(int)threadNum);
        else if (sizeof(TSAInt)==4)
            rt=divsufsort(src,(saidx32_t*)&out_sstring[0],(saidx32_t)size,(int)threadNum);
    #endif
       if (rt!=0)
            throw std::runtime_error("suffixString_create() error.");
    }

    template <class T>
    static hpatch_force_inline const T* _lower_bound(const T* rbegin,const T* rend,
                                                     const TChar* str,const TChar* str_end,
                                                     const TChar* src_begin,const TChar* src_end,
                                                     size_t min_eq=0){
#ifdef _SA_MATCHBY_STD_LOWER_BOUND
        return std::lower_bound<const T*,StringToken,const TSuffixString_compare&>
                    (rbegin,rend,StringToken(str,str_end),TSuffixString_compare(src_begin,src_end));
#else
        size_t left_eq=min_eq;
        size_t right_eq=min_eq;
        while (size_t len=(size_t)(rend-rbegin)) {
            const T* mid=rbegin+(len>>1);
            const TChar* ss=src_begin+(*mid);
            size_t eq_len=(left_eq<=right_eq)?left_eq:right_eq;
        #ifdef _SA_SORTBY_STD_SORT
            const int kMaxCmpLength_forLimitRangeDiff=kMaxCmpLength_sort; 
        #else
            const int kMaxCmpLength_forLimitRangeDiff=1024*8; //only for optimize limitRange match speed
        #endif
            size_t cmp_len=((src_end-ss)<(str_end-str))?(src_end-ss):(str_end-str);
            cmp_len=(cmp_len<kMaxCmpLength_forLimitRangeDiff)?cmp_len:kMaxCmpLength_forLimitRangeDiff;
            while ((eq_len<cmp_len)&&((ss[eq_len])==(str[eq_len]))){
                ++eq_len;
            }
            if (eq_len>=kMaxCmpLength_forLimitRangeDiff)
                return mid;
            
            bool is_less;
            if      (str+eq_len==str_end) is_less=false;
            else if (ss+eq_len==src_end)  is_less=true;
            else                          is_less=ss[eq_len]<str[eq_len];

            if (is_less){
                left_eq=eq_len;
                rbegin=mid+1;
            }else{
                right_eq=eq_len;
                rend=mid;
            }
        }
        return rbegin;
#endif
    }
    
    static TInt _lower_bound_TInt(const TInt* rbegin,const TInt* rend,
                                  const TChar* str,const TChar* str_end,
                                  const TChar* src_begin,const TChar* src_end,
                                  const TInt* SA_begin,size_t min_eq){
        return _lower_bound(rbegin,rend,str,str_end,src_begin,src_end,min_eq) - SA_begin;
    }
    
    static TInt _lower_bound_TInt32(const TInt32* rbegin,const TInt32* rend,
                                  const TChar* str,const TChar* str_end,
                                  const TChar* src_begin,const TChar* src_end,
                                  const TInt32* SA_begin,size_t min_eq){
        return _lower_bound(rbegin,rend,str,str_end,src_begin,src_end,min_eq) - SA_begin;
    }
    
    template<class T>
    static void _build_range256(const T* SA_begin,const T* SA_end,
                                const TChar* src_begin,const TChar* src_end,
                                const T** range){
        TChar str[1];
        const T* pos=SA_begin;
        for (size_t c=0;c<256;++c){
            str[0]=(TChar)(c);
            pos=_lower_bound(pos,SA_end,str,str+1,src_begin,src_end);
            range[c]=pos;
        }
        range[256]=SA_end;
    }

    
    template<class T>
    static void _build_range(const T* SA_begin,const T* SA_end,
                             const TChar* src_begin,const TChar* src_end,
                             T* range){
        TChar str[2];
        str[0]=0;
        str[1]=0;
        const T* pos=SA_begin;
        for (size_t cc=0;cc<256*256;++cc){
            //cc is [c0,c1]
            str[0]=(TChar)(cc>>8);
            str[1]=(TChar)(cc&255);
            pos=_lower_bound(pos,SA_end,str,str+2,src_begin,src_end);
            range[cc]=(T)(pos-SA_begin);
        }
        range[256*256]=(T)(SA_end-SA_begin);
    }

}//end namespace


TSuffixString::TSuffixString(bool isUsedFastMatch)
:m_src_begin(0),m_src_end(0),m_isUsedFastMatch(isUsedFastMatch),m_cached2char_range(0){
     clear_cache();
}

TSuffixString::TSuffixString(const TChar* src_begin,const TChar* src_end,bool isUsedFastMatch,size_t threadNum)
:m_src_begin(0),m_src_end(0),m_isUsedFastMatch(isUsedFastMatch),m_cached2char_range(0){
    clear_cache();
    resetSuffixString(src_begin,src_end,threadNum);
}

TSuffixString::~TSuffixString(){
    clear();
}

void TSuffixString::clear(){
    clear_cache();
    m_src_begin=0;
    m_src_end=0;
    _clearVector(m_SA_limit);
    _clearVector(m_SA_large);
}


void TSuffixString::resetSuffixString(const TChar* src_begin,const TChar* src_end,size_t threadNum){
    assert(src_begin<=src_end);
    m_src_begin=src_begin;
    m_src_end=src_end;
    m_isUseLargeSA=_isNeedUseLargeSA(src_end-src_begin);
    if (isUseLargeSA()){
        _clearVector(m_SA_limit);
        _suffixString_create(m_src_begin,m_src_end,m_SA_large,threadNum);
    }else{
        assert(sizeof(TInt32)==4);
        _clearVector(m_SA_large);
        _suffixString_create(m_src_begin,m_src_end,m_SA_limit,threadNum);
    }
    build_cache(threadNum);
}

TInt TSuffixString::lower_bound(const TChar* str,const TChar* str_end)const{
    //not use any cached range table
    //return m_lower_bound(m_cached_SA_begin,m_cached_SA_end,
    //                     str,str_end,m_src_begin,m_src_end,m_cached_SA_begin,0);
#if (_SSTRING_FAST_MATCH>0)
    #define kMinStrLen _SSTRING_FAST_MATCH
    assert((size_t)(str_end-str)>=kMinStrLen);
    if (m_isUsedFastMatch&&(!m_fastMatch.isHit(TFastMatchForSString::getHash(str,_SSTRING_FAST_MATCH))))
        return -1;
#else
    //assert(str_end-str>=2);
    #define kMinStrLen 2
    assert((size_t)(str_end-str)>=kMinStrLen);
#endif
    if ((kMinStrLen>=2)&&(m_cached2char_range!=0)){
        size_t cc=((size_t)str[1]) | (((size_t)str[0])<<8);
        size_t r0,r1;
        if (isUseLargeSA()){
            r0=((TInt*)m_cached2char_range)[cc]*sizeof(TInt);
            r1=((TInt*)m_cached2char_range)[cc+1]*sizeof(TInt);
        }else{
            r0=((TInt32*)m_cached2char_range)[cc]*sizeof(TInt32);
            r1=((TInt32*)m_cached2char_range)[cc+1]*sizeof(TInt32);
        }
        return m_lower_bound((TChar*)m_cached_SA_begin+r0,(TChar*)m_cached_SA_begin+r1,
                             str,str_end,m_src_begin,m_src_end,m_cached_SA_begin,2);
    }else if (kMinStrLen>0){
        size_t c=str[0];
        return m_lower_bound(m_cached1char_range[c],m_cached1char_range[c+1],
                             str,str_end,m_src_begin,m_src_end,m_cached_SA_begin,1);
    }else{
        return -1;
    }
}

void TSuffixString::clear_cache(){
#if (_SSTRING_FAST_MATCH>0)
    if (m_isUsedFastMatch) m_fastMatch.clear();
#endif
    if (m_cached2char_range){
        delete [](TChar*)m_cached2char_range;
        m_cached2char_range=0;
    }
    memset(&m_cached1char_range[0],0,sizeof(void*)*(256+1));
    m_cached_SA_begin=0;
    m_cached_SA_end=0;
    m_lower_bound=(t_lower_bound_func)_lower_bound_TInt32;//safe
}

void TSuffixString::build_cache(size_t threadNum){
    clear_cache();
#if (_SSTRING_FAST_MATCH>0)
    #define kFMZoom 4  //ctrl memory size & match speed
    if (m_isUsedFastMatch){
        //_out_diff_info("    build cache for sstring fast match ...\n");
        m_fastMatch.buildMatchCache(m_src_begin,m_src_end,threadNum,kFMZoom,_SSTRING_FAST_MATCH);
    }
#endif
    const size_t kUsedCacheMinSASize =2*(1<<20); //Enable large cache table only when string is large.
    if (SASize()>kUsedCacheMinSASize){
        m_cached2char_range=new TChar[(256*256+1)*(isUseLargeSA()?sizeof(size_t):sizeof(TInt32))];
    }
    
    if (isUseLargeSA()){
        m_lower_bound=(t_lower_bound_func)_lower_bound_TInt;
        if (m_SA_large.empty()) return;
        m_cached_SA_begin=&m_SA_large[0];
        m_cached_SA_end=&m_SA_large[0]+m_SA_large.size();
        _build_range256((TInt*)m_cached_SA_begin,(TInt*)m_cached_SA_end,
                        m_src_begin,m_src_end,(const TInt**)&m_cached1char_range[0]);
        if (m_cached2char_range){
            _build_range((TInt*)m_cached_SA_begin,(TInt*)m_cached_SA_end,
                         m_src_begin,m_src_end,(TInt*)m_cached2char_range);
        }
    }else{
        m_lower_bound=(t_lower_bound_func)_lower_bound_TInt32;
        if (m_SA_limit.empty()) return;
        m_cached_SA_begin=&m_SA_limit[0];
        m_cached_SA_end=&m_SA_limit[0]+m_SA_limit.size();
        _build_range256((TInt32*)m_cached_SA_begin,(TInt32*)m_cached_SA_end,
                        m_src_begin,m_src_end,(const TInt32**)&m_cached1char_range[0]);
        if (m_cached2char_range){
            _build_range((TInt32*)m_cached_SA_begin,(TInt32*)m_cached_SA_end,
                         m_src_begin,m_src_end,(TInt32*)m_cached2char_range);
        }
    }
}


    template<bool isMT>
    static void _filter_insert(TBloomFilter<TFastMatchForSString::THash>* filter,
                               const TChar* src_begin,const TChar* src_end,size_t kRollLen){
        const TChar* cur = src_begin;
        TFastMatchForSString::THash h=TFastMatchForSString::getHash(cur,kRollLen);
        cur+=kRollLen;
        do {
    #if (_IS_USED_MULTITHREAD)
            if (isMT)
                filter->insert_MT(h);
            else
    #endif
                filter->insert(h);
            if (cur<src_end)
                h=TFastMatchForSString::rollHash(h,cur++,kRollLen);
            else
                break;
        } while (true);
    }

    void TFastMatchForSString::buildMatchCache(const TChar* src_begin,const TChar* src_end,size_t threadNum,
                                               size_t kBloomZoom,size_t kRollLen){
        size_t srcSize=src_end-src_begin;
        if (srcSize>=kRollLen){
            const size_t rollSize=srcSize-(kRollLen-1);
            bf.init(rollSize,kBloomZoom); //alloc large memory
#if (_IS_USED_MULTITHREAD)
            const size_t kInsertMinParallelSize=4096;
            if ((threadNum>1)&&(rollSize>=kInsertMinParallelSize)) {
                const size_t maxThreanNum=rollSize/(kInsertMinParallelSize/2);
                threadNum=(threadNum<=maxThreanNum)?threadNum:maxThreanNum;

                const size_t step=rollSize/threadNum;
                const size_t threadCount=threadNum-1;
                std::vector<std::thread> threads(threadCount);
                for (size_t i=0;i<threadCount;i++,src_begin+=step)
                    threads[i]=std::thread(_filter_insert<true>,&bf,src_begin,src_begin+step+(kRollLen-1),kRollLen);
                _filter_insert<true>(&bf,src_begin,src_end,kRollLen);
                for (size_t i=0;i<threadCount;i++)
                    threads[i].join();
            }else
#endif
            {
                _filter_insert<false>(&bf,src_begin,src_end,kRollLen);
            }
        }else if ((srcSize>0)||(src_begin!=0))
            bf.init(0,kBloomZoom);
        else{
            bf.clear();
        }
    }
    
}//namespace hdiff_private
