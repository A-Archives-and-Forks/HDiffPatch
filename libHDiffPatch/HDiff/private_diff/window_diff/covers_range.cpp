//covers_range.cpp
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

#include "covers_range.h"
#include <assert.h>
#include <algorithm>
#include <stdlib.h>
#include <stdexcept>  //std::runtime_error
namespace hdiff_private{

template<class T>
    static inline T _abs_sub(T x,T y){ return (x>=y)?x-y:y-x; }

    static inline hpatch_StreamPos_t _x_begin(const TCover* cover){
        return cover->newPos;
    }
    static inline hpatch_StreamPos_t _x_end(const TCover* cover){
        return cover->newPos+cover->length;
    }

    static inline hpatch_StreamPos_t _z_off(const TCover* cover,hpatch_StreamPos_t kMaxNewPos){
        return kMaxNewPos+cover->oldPos-cover->newPos;
    }

    static inline hpatch_StreamPos_t _y_end(const TCover* cover){
        return cover->oldPos+cover->length;
    }

    static inline hpatch_StreamPos_t _x_size(const TCover* cover_begin,const TCover* cover_end){
        assert(cover_begin<cover_end);
        return _x_end(cover_end-1)-cover_begin->newPos;
    } 

    struct Tz{
        hpatch_StreamPos_t z;
        hpatch_StreamPos_t sum_w;
        const TCover*      cover;
        inline bool operator < (const Tz& a)const{ return z<a.z; }
    };
    static const TCover* _get_best_clip(const TCover* cover,const TCover* cover_end,
                                        hpatch_StreamPos_t windowSize,std::vector<Tz>& zs);
static void _get_best_clips(std::vector<TCoversRange>& ranges,hpatch_StreamPos_t windowSize,
                            const TCover* cover,const TCover* cover_end,hpatch_StreamPos_t kSegSize){
    
    ranges.clear();
    ranges.reserve((size_t)(_x_size(cover,cover_end)/kSegSize)+1);
    ranges.push_back(TCoversRange(cover,cover_end));
    size_t i=0;
    std::vector<Tz> temp_zs;
    while (i<ranges.size()){
        TCoversRange& seg=ranges[i];
        const hpatch_StreamPos_t segSize=_x_size(seg.cover_begin,seg.cover_end);
        if ((segSize<=kSegSize)||(seg.cover_end-seg.cover_begin==1)) { ++i; continue; }
        const TCover* best_clip=_get_best_clip(seg.cover_begin,seg.cover_end,windowSize,temp_zs);
        if (best_clip==0) { assert(segSize<=windowSize); ++i; continue; }

        assert((best_clip>=seg.cover_begin)&&(best_clip<seg.cover_end));
        const TCover* cur_cover_end= seg.cover_end;
        seg.cover_end=best_clip;
        ranges.push_back(TCoversRange(best_clip,cur_cover_end));
    }
    std::sort(ranges.begin(),ranges.end());
}

static size_t _get_best_z(std::vector<Tz>& zs,const TCover* cover,const TCover* cover_end,
                          const hpatch_StreamPos_t R,hpatch_StreamPos_t kMaxNewPos){
    assert(cover<cover_end);
    zs.resize(cover_end-cover);
    for (size_t i=0;i<zs.size();i++){
        zs[i].cover=cover+i;
        zs[i].z=_z_off(cover+i,kMaxNewPos);
    }
    std::sort(zs.begin(),zs.end());
    
    hpatch_StreamPos_t cur_w=zs[0].cover->length;
    size_t z_left=0,z_right=0;
    for (size_t i=0;i<zs.size();i++){
        while ((z_right+1<zs.size())&&(zs[z_right+1].z-zs[i].z<=R)){ cur_w+=zs[++z_right].cover->length; }
        while ((z_left<i)&&(zs[i].z-zs[z_left+1].z>R)) { cur_w-=zs[++z_left].cover->length; }
        zs[i].sum_w=cur_w;
    }
    
    size_t best_w_i=0;
    for (size_t i=1;i<zs.size();i++){
        if (zs[i].sum_w>zs[best_w_i].sum_w)
            best_w_i=i;
    }
    return best_w_i;
}

static void _get_valid_by_z(unsigned char* out_valid,const TCover* cover,const TCover* cover_end,
                            hpatch_StreamPos_t best_z,const hpatch_StreamPos_t R,hpatch_StreamPos_t kMaxNewPos){
    for (;cover<cover_end;cover++,out_valid++){
        *out_valid =(_abs_sub(_z_off(cover,kMaxNewPos),best_z)<R)?1:0;
    }
}


    static inline hpatch_StreamPos_t _R(const TCover* cover,const TCover* cover_end,hpatch_StreamPos_t oldWindowSize){
        return std::max(oldWindowSize/8,_x_size(cover,cover_end)/2+64);
    }
    static inline hpatch_StreamPos_t _R(const TCoversRange& seg,hpatch_StreamPos_t oldWindowSize){
        return _R(seg.cover_begin,seg.cover_end,oldWindowSize);
    }

    static inline hpatch_StreamPos_t _R_min(const TCover* cover,const TCover* cover_end,hpatch_StreamPos_t oldWindowSize){
        return std::min(oldWindowSize/4,_R(cover,cover_end,oldWindowSize));
    }
    
static const TCover* _get_best_clip(const TCover* cover,const TCover* cover_end,
                                    hpatch_StreamPos_t windowSize,std::vector<Tz>& zs){
    const size_t N=cover_end-cover;
    if (N<2) return 0;
    const hpatch_StreamPos_t kMaxNewPos=_x_end(cover_end-1);

    hpatch_StreamPos_t R=_R_min(cover,cover_end,windowSize);
    size_t best_zi=_get_best_z(zs,cover,cover_end,(size_t)R,kMaxNewPos);
    hpatch_StreamPos_t best_z=zs[best_zi].z;
    hpatch_StreamPos_t total_near_w=zs[best_zi].sum_w;
    if (total_near_w==0) return 0;

    hpatch_StreamPos_t total_w=0;
    for (size_t i=0;i<N;i++)
        total_w+=cover[i].length;

    const hpatch_uint64_t kRatioScale=(1<<16);
    hpatch_StreamPos_t left_near_w=0;
    hpatch_StreamPos_t left_w=0;
    hpatch_uint64_t best_score=0;
    size_t best_mid=0;

    for (size_t mid=1;mid<N;mid++){
        hpatch_StreamPos_t z=_z_off(cover+mid-1,kMaxNewPos);
        left_w+=cover[mid-1].length;
        if (_abs_sub(z,best_z)<=R)
            left_near_w+=cover[mid-1].length;

        hpatch_StreamPos_t right_near_w=total_near_w-left_near_w;
        hpatch_StreamPos_t right_w=total_w-left_w;

        hpatch_uint64_t left_ratio=left_near_w*kRatioScale/(left_w+R);
        hpatch_uint64_t right_ratio=right_near_w*kRatioScale/(right_w+R);
        hpatch_uint64_t ratio_diff=_abs_sub(left_ratio,right_ratio);

        hpatch_StreamPos_t min_w=std::min(left_w,right_w);
        hpatch_uint64_t score=1+ratio_diff*min_w+(kRatioScale/(1<<13))*(cover[mid].newPos-_x_end(cover+mid-1));

        if (score>best_score){
            best_score=score;
            best_mid=mid;
        }
    }

    if ((best_mid>0) && (best_mid<N))
        return cover+best_mid;
    else
        return 0;
}

    static void _update_window(hpatch_TWindow& out_window,const TCover* cover_begin,const TCover* cover_end,
                               unsigned char* coverValid){
        hpatch_StreamPos_t oldMin = ~(hpatch_StreamPos_t)0;
        hpatch_StreamPos_t oldMax = 0;
        hpatch_StreamPos_t newMin = ~(hpatch_StreamPos_t)0;
        hpatch_StreamPos_t newMax = 0;
        bool hasValidCover = false;
        
        const TCover* cover = cover_begin;
        unsigned char* valid = coverValid;
        for (; cover != cover_end; ++cover, ++valid){
            if (*valid==0) continue;  // skip invalid cover
            
            hasValidCover = true;
            oldMin = (oldMin < cover->oldPos) ? oldMin : cover->oldPos;
            hpatch_StreamPos_t yend = _y_end(cover);
            oldMax = (oldMax > yend) ? oldMax : yend;
            newMin = (newMin < cover->newPos) ? newMin : cover->newPos;
            newMax = _x_end(cover);
        }
        
        if (hasValidCover){
            out_window.oldPos = oldMin;
            out_window.oldLength = oldMax - oldMin;
            out_window.newPos = newMin;
            out_window.newLength = newMax - newMin;
        } else {
            memset(&out_window,0,sizeof(out_window));
        }
    }
    static inline void _update_window(TCoversRange& range){
        _update_window(range.window,range.cover_begin,range.cover_end,range.coverValid_begin);
    }

    
    static const hpatch_uint64_t kExtendCostScale=1024;
    static const hpatch_uint64_t kExtendMaxCost_defaut=kExtendCostScale*4;
    static const hpatch_uint64_t kExtendMaxCost_max=kExtendCostScale*8;

    static inline hpatch_StreamPos_t _old_gap(const TCover* c,const hpatch_TWindow& window){
        hpatch_StreamPos_t cOldEnd=_y_end(c);
        if (cOldEnd<=window.oldPos)
            return window.oldPos-cOldEnd;
        if (c->oldPos>=window.oldPos+window.oldLength)
            return c->oldPos-(window.oldPos+window.oldLength);
        return 0;
    }
    static inline hpatch_uint64_t _cover_cost(const TCover* c,const hpatch_TWindow& window){
        hpatch_StreamPos_t gap=_old_gap(c,window);
        return (gap*kExtendCostScale)/c->length;
    }
    static bool _can_fit_cover(const TCover* c,const hpatch_TWindow& window,
                               hpatch_StreamPos_t oldWindowSize){
        hpatch_StreamPos_t newOldPos=std::min(window.oldPos,c->oldPos);
        hpatch_StreamPos_t newOldEnd=std::max(window.oldPos+window.oldLength,_y_end(c));
        return (newOldEnd-newOldPos)<=oldWindowSize;
    }
    static hpatch_StreamPos_t _cover_score(const TCover* c,const hpatch_TWindow& window,
                                           hpatch_StreamPos_t oldWindowSize){
        if (!_can_fit_cover(c,window,oldWindowSize)) return 0;
        hpatch_StreamPos_t overlapBegin=std::max(c->oldPos,window.oldPos);
        hpatch_StreamPos_t overlapEnd=std::min(_y_end(c),window.oldPos+window.oldLength);
        hpatch_StreamPos_t overlap=(overlapEnd>overlapBegin)?(overlapEnd-overlapBegin):0;
        if (overlap==c->length)
            return c->length;
        else if (overlap>0)
            return overlap+(c->length-overlap)/2;
        else{
            hpatch_StreamPos_t gap=_old_gap(c,window);
            hpatch_StreamPos_t clampedGap=std::min(gap,oldWindowSize);
            hpatch_StreamPos_t score_factor=4*oldWindowSize-3*clampedGap;
            assert(oldWindowSize>0);
            return (hpatch_uint64_t)c->length*score_factor/(8*oldWindowSize);
        }
    }

static void _get_better_clip(TCoversRange& range_a,TCoversRange& range_b,const TCover* firstValid,const TCover* endValid,
                             hpatch_StreamPos_t oldWindowSize,const TCover* cover,unsigned char* valid,
                             std::vector<hpatch_StreamPos_t>& score_a,std::vector<hpatch_StreamPos_t>& score_b,
                             std::vector<hpatch_uint64_t>& prefix_a,std::vector<hpatch_uint64_t>& suffix_b){
    const size_t N=endValid-firstValid;
    if (N==0) return;
    score_a.resize(N);
    score_b.resize(N);
    for (size_t k=0;k<N;k++){
        const TCover* c=firstValid+k;
        score_a[k]=_cover_score(c,range_a.window,oldWindowSize);
        score_b[k]=_cover_score(c,range_b.window,oldWindowSize);
    }
    prefix_a.resize(N+1);
    suffix_b.resize(N+1);
    prefix_a[0]=0;
    for (size_t k=0;k<N;k++)
        prefix_a[k+1]=prefix_a[k]+score_a[k];
    suffix_b[N]=0;
    for (size_t k=N;k>0;k--)
        suffix_b[k-1]=suffix_b[k]+score_b[k-1];
    hpatch_uint64_t best_score=0;
    size_t best_k=0;
    for (size_t k=0;k<=N;k++){
        hpatch_uint64_t total=prefix_a[k]+suffix_b[k];
        if (total>best_score){
            best_score=total;
            best_k=k;
        }
    }
    const TCover* best_split=firstValid+best_k;
    if (best_split==range_a.cover_end) return;
    range_a.cover_end=best_split;
    range_b.cover_begin=best_split;
    range_b.coverValid_begin=valid+(best_split-cover);
}

static void _clip_edge_ranges(std::vector<TCoversRange>& ranges,hpatch_StreamPos_t oldWindowSize,
                                const TCover* cover,unsigned char* valid){
    std::vector<hpatch_StreamPos_t> temp_score_a,temp_score_b;
    std::vector<hpatch_uint64_t>    temp_prefix_a,temp_suffix_b;
    for (size_t i=1;i<ranges.size();i++){
        const TCover* firstValid=ranges[i-1].cover_end;
        while ((firstValid>ranges[i-1].cover_begin)&&(!valid[firstValid-1-cover]))
            --firstValid;
        const TCover* endValid=ranges[i].cover_begin;
        while ((endValid<ranges[i].cover_end)&&(!valid[endValid-cover]))
            ++endValid;
        if (firstValid==endValid)
            continue;
        _get_better_clip(ranges[i-1],ranges[i],firstValid,endValid,oldWindowSize,cover,valid,
                         temp_score_a,temp_score_b,temp_prefix_a,temp_suffix_b);
    }
}

static void _extend_interior_ranges(std::vector<TCoversRange>& ranges,hpatch_StreamPos_t oldWindowSize,
                                    const TCover* cover,unsigned char* valid,hpatch_StreamPos_t kExtendMaxCost){
    for (size_t i=0;i<ranges.size();i++){
        while (true){
            TCoversRange& range=ranges[i];
            bool rangeChanged=false;
            for (const TCover* c=range.cover_begin;c<range.cover_end;++c){
                size_t idx=c-cover;
                if (valid[idx]) continue;
                if (!_can_fit_cover(c,range.window,oldWindowSize)) continue;
                hpatch_uint64_t cost=_cover_cost(c,range.window);
                if (cost<=kExtendMaxCost){
                    valid[idx]=1;
                    _update_window(range);
                    rangeChanged=true;
                }
            }
            if (!rangeChanged)
                break;
        }
    }
}

static void _merge_nearby_ranges(std::vector<TCoversRange>& ranges,hpatch_StreamPos_t oldWindowSize,hpatch_StreamPos_t kMaxGap){
    for (size_t i=1;i<ranges.size();i++){
        TCoversRange& a=ranges[i-1];
        TCoversRange& b=ranges[i];
        if ((a.window.oldLength==0)||(b.window.oldLength==0)) continue;
        hpatch_StreamPos_t oldEnd_a=a.window.oldPos+a.window.oldLength;
        hpatch_StreamPos_t oldEnd_b=b.window.oldPos+b.window.oldLength;
        hpatch_StreamPos_t merged_oldPos=std::min(a.window.oldPos,b.window.oldPos);
        hpatch_StreamPos_t merged_oldEnd=std::max(oldEnd_a,oldEnd_b);
        hpatch_StreamPos_t merged_oldLength=merged_oldEnd-merged_oldPos;
        hpatch_StreamPos_t ab_length=(a.window.oldLength+b.window.oldLength);
        hpatch_StreamPos_t gap=(merged_oldLength>ab_length)?(merged_oldLength-ab_length):0;

        if ((gap<=kMaxGap)&&(merged_oldLength<=oldWindowSize)){
            b.cover_begin=a.cover_begin;
            b.coverValid_begin=a.coverValid_begin;
            _update_window(b);
            memset(&a,0,sizeof(a));
        }
    }
    // remove empty ranges
    size_t write=0;
    for (size_t read=0;read<ranges.size();read++){
        if (ranges[read].window.oldLength>0){
            if (write!=read) ranges[write]=ranges[read];
            ++write;
        }
    }
    ranges.resize(write);
}

void clipRangeByOld(std::vector<TCoversRange>& ranges,hpatch_StreamPos_t oldWindowSize,
                    const TCover* cover,const TCover* cover_end,unsigned char* valid,hpatch_StreamPos_t kSegSize){
    _get_best_clips(ranges,oldWindowSize,cover,cover_end,kSegSize);
    const hpatch_StreamPos_t kMaxNewPos=_x_end(cover_end-1);

    {
        std::vector<Tz> zs;
        for (size_t i=0;i<ranges.size();i++){
            hpatch_StreamPos_t R=_R(ranges[i],oldWindowSize);
            size_t _best_zi=_get_best_z(zs,ranges[i].cover_begin,ranges[i].cover_end,R,kMaxNewPos);
            hpatch_StreamPos_t best_z=zs[_best_zi].z;
            ranges[i].coverValid_begin=valid+(ranges[i].cover_begin-cover);
            while(true){
                _get_valid_by_z(ranges[i].coverValid_begin,ranges[i].cover_begin,ranges[i].cover_end,
                                best_z,R,kMaxNewPos);
                _update_window(ranges[i]);
                if (ranges[i].window.oldLength<=oldWindowSize) break;
                //kSegSize too large?
                R=(R>6)?(R+1)/2:R-1;
                if (R==0) throw std::runtime_error("kSegSize error!");
            }
        }
    }

    _clip_edge_ranges(ranges,oldWindowSize,cover,valid);
    _extend_interior_ranges(ranges,oldWindowSize,cover,valid,kExtendMaxCost_defaut);
 
    _merge_nearby_ranges(ranges,oldWindowSize,oldWindowSize*2/4);

    _clip_edge_ranges(ranges,oldWindowSize,cover,valid);
    _extend_interior_ranges(ranges,oldWindowSize,cover,valid,kExtendMaxCost_max);
}


    static inline const TCover* _find_split_point(const TCover* cover_begin,const TCover* cover_end,
                                                  hpatch_StreamPos_t splitNewPos){
        return std::lower_bound(cover_begin,cover_end,splitNewPos,cover_cmp_by_new_t<TCover>());
    }

    static void _split_range_by_new(TCoversRange& range,size_t newWindowSize,
                                     std::vector<TCoversRange>& ranges){
        assert(range.window.newLength>newWindowSize);
        const size_t totalCovers=range.cover_end-range.cover_begin;
        const size_t minCovers=std::max((size_t)1,totalCovers/8);
        const hpatch_StreamPos_t splitNewPos=range.window.newPos+range.window.newLength/2;

        const TCover* split=_find_split_point(range.cover_begin,range.cover_end,splitNewPos);
        if ((size_t)(split-range.cover_begin)<minCovers)
            split=range.cover_begin+minCovers;
        if ((size_t)(range.cover_end-split)<minCovers)
            split=range.cover_end-minCovers;
        if ((split<=range.cover_begin)||(split>=range.cover_end))
            split=range.cover_begin+totalCovers/2;

        TCoversRange right;
        right.cover_begin=split;
        right.cover_end=range.cover_end;
        right.coverValid_begin=range.coverValid_begin+(right.cover_begin-range.cover_begin);

        range.cover_end=split;
        _update_window(range);
        _update_window(right);

        ranges.push_back(right);
    }

void clipRangeByNew(std::vector<TCoversRange>& ranges,size_t newWindowSize){
    size_t i=0;
    while (i<ranges.size()){
        TCoversRange& range=ranges[i];
        if (range.window.newLength<=newWindowSize) { i++; continue;} 
        _split_range_by_new(range,newWindowSize,ranges);
    }
    std::sort(ranges.begin(),ranges.end());
}


}//namespace hdiff_private
