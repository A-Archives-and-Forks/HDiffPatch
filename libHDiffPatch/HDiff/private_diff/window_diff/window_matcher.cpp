//window_matcher.cpp
/*
 The MIT License (MIT)
 Copyright (c) 2025-2026 HouSisong
 
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
#include "window_matcher.h"
#include "covers_range.h"
#include <algorithm> //std::sort
#include <stdexcept>  //std::runtime_error
#define checki(value,info) { if (!(value)) { \
    assert(0);\
    throw std::runtime_error(info); } \
}
#define check(value) checki(value,"check "#value" error!")

namespace hdiff_private{

void getBigCoversInWindows(std::vector<std::vector<TCover> >& out_bigCoverss,const std::vector<TCover>& covers,
                           const std::vector<hpatch_TWindow>& windows,size_t kBigCoverSize){
    out_bigCoverss.clear();
    out_bigCoverss.resize(windows.size());
    size_t wi=0;
    for (size_t ci=0;ci<covers.size();++ci){
        TCover cover=covers[ci];
        while (cover.length>=kBigCoverSize){
            if (wi>=windows.size()) return; //finish
            if (cover.newPos+cover.length<=windows[wi].newPos) break; //next ci
            if (cover.newPos>=windows[wi].newPos+windows[wi].newLength) { ++wi; continue; } //next wi
            if (cover.newPos<windows[wi].newPos){
                hpatch_StreamPos_t cutLen=windows[wi].newPos-cover.newPos;
                setCover(cover,cover.oldPos+cutLen,cover.newPos+cutLen,cover.length-cutLen);
            }
            if (cover.oldPos+cover.length<=windows[wi].oldPos) break; //next ci
            if (cover.oldPos>=windows[wi].oldPos+windows[wi].oldLength) break; //next ci
            if (cover.oldPos<windows[wi].oldPos){
                hpatch_StreamPos_t cutLen=windows[wi].oldPos-cover.oldPos;
                setCover(cover,cover.oldPos+cutLen,cover.newPos+cutLen,cover.length-cutLen);
            }
            hpatch_StreamPos_t hitNewLen=(cover.newPos+cover.length<=windows[wi].newPos+windows[wi].newLength)?
                                          cover.length:windows[wi].newPos+windows[wi].newLength-cover.newPos;
            hpatch_StreamPos_t hitOldLen=(cover.oldPos+cover.length<=windows[wi].oldPos+windows[wi].oldLength)?
                                          cover.length:windows[wi].oldPos+windows[wi].oldLength-cover.oldPos;
            hpatch_StreamPos_t hitLen=(hitNewLen<hitOldLen)?hitNewLen:hitOldLen;
            if (hitLen>=kBigCoverSize){
                TCover hitCover={cover.oldPos,cover.newPos,hitLen};
                out_bigCoverss[wi].push_back(hitCover);
            }
            setCover(cover,cover.oldPos+hitLen,cover.newPos+hitLen,cover.length-hitLen);
        }
    }
}

void updateCoversPosIntoWindows(std::vector<TCover>& covers,const hpatch_TWindow& window){
    for (size_t i=0;i<covers.size();++i){
        TCover& cover=covers[i];
        check(cover.oldPos>=window.oldPos);
        check(cover.newPos>=window.newPos);
        cover.oldPos-=window.oldPos;
        cover.newPos-=window.newPos;
        check(cover.oldPos+cover.length<=window.oldLength);
        check(cover.newPos+cover.length<=window.newLength);
    }
}
void updateCoversPosFromWindows(std::vector<TCover>& covers,const hpatch_TWindow& window){
    for (size_t i=0;i<covers.size();++i){
        TCover& cover=covers[i];
        check(cover.oldPos+cover.length<=window.oldLength);
        check(cover.newPos+cover.length<=window.newLength);
        cover.oldPos+=window.oldPos;
        cover.newPos+=window.newPos;
    }
}

void getWindowBoxByCovers(hpatch_TWindow& out_window,const std::vector<TCover>& covers){
    const TCover* cover_begin=covers.data();
    const TCover* cover_end=covers.data()+covers.size();
    hpatch_StreamPos_t oldMin= ~(hpatch_StreamPos_t)0;
    hpatch_StreamPos_t oldMax= 0;
    hpatch_StreamPos_t newMin= ~(hpatch_StreamPos_t)0;
    hpatch_StreamPos_t newMax= 0;
    for (;cover_begin!=cover_end;){
        const TCover& cover = *cover_begin++;
        if (cover.length==0) continue;
        oldMin=(oldMin<cover.oldPos)?oldMin:cover.oldPos;
        oldMax=(oldMax>cover.oldPos+cover.length)?oldMax:cover.oldPos+cover.length;
        newMin=(newMin<cover.newPos)?newMin:cover.newPos;
        newMax=(newMax>cover.newPos+cover.length)?newMax:cover.newPos+cover.length;
    }
    
    if ((oldMin<oldMax)&(newMin<newMax)){
        out_window.oldPos=oldMin;
        out_window.oldLength=oldMax-oldMin;
        out_window.newPos=newMin;
        out_window.newLength=newMax-newMin;
    }else{ //empty window
        out_window.oldPos=0;
        out_window.oldLength=0;
        out_window.newPos=0;
        out_window.newLength=0;
    }
}

    static void _extenPos(hpatch_StreamPos_t& pos,hpatch_StreamPos_t& length,size_t windowSize,
                          hpatch_StreamPos_t limitPos,hpatch_StreamPos_t limitPosEnd,size_t kExtenPosSize){
        hpatch_StreamPos_t posEnd=pos+length;
        assert((length<=windowSize)&&(pos>=limitPos)&&(posEnd<=limitPosEnd));
        size_t extenLen=(windowSize-(size_t)length)>>1;
        extenLen=(extenLen<kExtenPosSize)?extenLen:kExtenPosSize;
        pos=(pos<extenLen)?0:pos-extenLen;
        pos=(pos>limitPos)?pos:limitPos;
        posEnd+=extenLen;
        posEnd=(posEnd<limitPosEnd)?posEnd:limitPosEnd;
        length=posEnd-pos;
    }
void extenWindowsForMatch(std::vector<hpatch_TWindow>& windows,hpatch_StreamPos_t newSize,hpatch_StreamPos_t oldSize,
                          size_t newWindowSize,size_t oldWindowSize,size_t kExtenPosSize){
    hpatch_StreamPos_t newPosEndBck=0;
    for (size_t i=0;i<windows.size();++i){
        hpatch_TWindow& window=windows[i];
        assert(window.newLength>0);
        if (window.oldLength==0) continue;
        _extenPos(window.oldPos,window.oldLength,oldWindowSize,0,oldSize,kExtenPosSize);
        hpatch_StreamPos_t newLimitPosEnd=window.newPos+window.newLength;
        newLimitPosEnd=(i+1<windows.size())?newLimitPosEnd+(windows[i+1].newPos-newLimitPosEnd)/2:newSize;
        _extenPos(window.newPos,window.newLength,newWindowSize,newPosEndBck,newLimitPosEnd,kExtenPosSize);
        newPosEndBck=window.newPos+window.newLength;
    }
}

void rangesToWindows(std::vector<hpatch_TWindow>& out_windows,const std::vector<TCoversRange>& cvRanges){
    out_windows.clear();
    out_windows.reserve(cvRanges.size());
    for (size_t i=0;i<cvRanges.size();i++){
        const hpatch_TWindow& window=cvRanges[i].window;
        if (window.oldLength>0)
            out_windows.push_back(window);
    }
}

static void _check_windows_safe(const std::vector<hpatch_TWindow>& windows,hpatch_StreamPos_t newSize,hpatch_StreamPos_t oldSize,
                                size_t newWindowSize,size_t oldWindowSize){
    hpatch_StreamPos_t newPosEndBck=0;
    for (size_t i=0;i<windows.size();++i){
        const hpatch_TWindow& window=windows[i];
        check((window.newLength<=newWindowSize)&&(window.oldLength<=oldWindowSize));    
        check((window.newLength>0)&&(window.oldLength>0));
        check(window.newPos+window.newLength<=newSize);
        check(window.oldPos+window.oldLength<=oldSize);
        check(window.newPos>=newPosEndBck);
        newPosEndBck=window.newPos+window.newLength;
    }
}

TWindowMatcher::~TWindowMatcher(){
}

    static size_t _getMaxSplitCount(hpatch_StreamPos_t length,hpatch_StreamPos_t maxLen){
        return (size_t)((length+maxLen-1)/maxLen);
    }

    static void _splitLargeCovers(std::vector<TCover>& covers,hpatch_StreamPos_t maxLen){
        assert(maxLen>0);
        const size_t n=covers.size();
        size_t addCount=0;
        for (size_t i=0;i<n;++i){
            if (covers[i].length>maxLen)
                addCount+=_getMaxSplitCount(covers[i].length,maxLen)-1;
        }
        if (addCount==0) return;

        covers.resize(n+addCount);
        size_t src=n-1;
        size_t dst=n+addCount-1;
        while (src!=dst){
            const TCover c=covers[src];
            if (c.length<=maxLen){
                covers[dst--]=c;
                --src;
            }else{
                const size_t splitCount=_getMaxSplitCount(c.length,maxLen);
                const hpatch_StreamPos_t splitLen=(c.length+splitCount-1)/splitCount;
                // write sub-covers from last (highest newPos) at dst, backward to first
                hpatch_StreamPos_t curOldPos=c.oldPos+(splitCount-1)*splitLen;
                hpatch_StreamPos_t curNewPos=c.newPos+(splitCount-1)*splitLen;
                hpatch_StreamPos_t curLen=c.length-(splitCount-1)*splitLen;
                for (size_t k=0;k<splitCount;k++){
                    covers[dst--]={curOldPos,curNewPos,curLen};
                    curOldPos-=splitLen;
                    curNewPos-=splitLen;
                    curLen=splitLen;
                }
                --src;
            }
        }
    }

TWindowMatcher::TWindowMatcher(hpatch_StreamPos_t newSize,hpatch_StreamPos_t oldSize,size_t newWindowSize,
                               size_t oldWindowSize,size_t kSegSize,std::vector<TCover>& covers)
:m_newSize(newSize),m_oldSize(oldSize),m_newWindowSize(newWindowSize),m_oldWindowSize(oldWindowSize),
 m_kSegSize(kSegSize),m_covers(covers){
    assert((kSegSize>0)&&(kSegSize<=oldWindowSize));
    hpatch_StreamPos_t splitLen = std::min(m_kSegSize,newWindowSize);
    splitLen=((newWindowSize<splitLen)?newWindowSize:splitLen);
    _splitLargeCovers(m_covers,splitLen);
}

void TWindowMatcher::search_windows(std::vector<hpatch_TWindow>& out_windows){
    _out_diff_info("    search windows by covers ...\n");
    out_windows.clear();
    if (m_covers.empty()) return;

    std::vector<unsigned char> coverValids(m_covers.size());
    std::vector<TCoversRange>  cvRanges;
    clipRangeByOld(cvRanges,m_oldWindowSize,m_covers.data(),m_covers.data()+m_covers.size(),coverValids.data(),m_kSegSize);
    //rangesToWindows(out_windows,cvRanges);
    //_check_windows_safe(out_windows,m_newSize,m_oldSize,m_newSize,m_oldWindowSize);

    clipRangeByNew(cvRanges,m_newWindowSize);
    rangesToWindows(out_windows,cvRanges);

    _check_windows_safe(out_windows,m_newSize,m_oldSize,m_newWindowSize,m_oldWindowSize);
}


}//namespace hdiff_private
