//  match_block.cpp
//  hdiff
/*
 The MIT License (MIT)
 Copyright (c) 2021 HouSisong
 
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
#include "match_block.h"
#include "../diff.h"
#include "mem_buf.h" //TAutoMem
#include <algorithm>
#include <stdexcept>  //std::runtime_error
#define _check(value,info) { if (!(value)) { throw std::runtime_error(info); } }

namespace hdiff_private {
    typedef TMatchBlockBase::TPackedCover TPackedCover;

    template<class T> inline static
    void _clearV(std::vector<T>& v){
        std::vector<T> tmp;
        v.swap(tmp);
    }
    
    #define _cover_pos(isNew,pcover) (isNew?(pcover)->newPos:(pcover)->oldPos)

    #define _kMinMoveLen_new    1  // must 1
    #define _kMinMoveLen_old    16
    #define kMinMoveLen   (isNew?_kMinMoveLen_new:_kMinMoveLen_old)

    template<bool isNew> static 
    void _getPackedCovers(hpatch_StreamPos_t dataSize,const std::vector<TCover>& blockCovers,
                          std::vector<TPackedCover>& out_packedCovers){
        out_packedCovers.clear();
        const hpatch_TCover* cover=blockCovers.data();
        const hpatch_TCover* cover_end=cover+blockCovers.size();
        hpatch_StreamPos_t dst=0;
        hpatch_StreamPos_t srcPos=0;
        for (;cover<cover_end;++cover){
            if (_cover_pos(isNew,cover)<=srcPos){
                srcPos=std::max(srcPos,_cover_pos(isNew,cover)+cover->length);
                continue;
            }
            hpatch_StreamPos_t moveLen=_cover_pos(isNew,cover)-srcPos;
            if (moveLen>=kMinMoveLen){
                TPackedCover pkcover={srcPos,dst,moveLen};
                out_packedCovers.push_back(pkcover);
                dst+=moveLen;
            }
            srcPos=_cover_pos(isNew,cover)+cover->length;
        }
        assert(dataSize>=srcPos);
        hpatch_StreamPos_t moveLen=dataSize-srcPos;
        if (moveLen>=kMinMoveLen){
            TPackedCover pkcover={srcPos,dst,moveLen};
            out_packedCovers.push_back(pkcover);
            dst+=moveLen;
        }
        //return dst;
    }

TOldInvalidFilter::TOldInvalidFilter(const unsigned char* newData,const unsigned char* newData_end,
                                     const hpatch_TStreamInput* oldStream,size_t _threadNum,
                                     size_t _minOldInvalidSize,size_t _kBloomZoom,size_t _rollLen,
                                     size_t _R,size_t _hitRateThreshold,size_t _cacheBlockSize)
 :threadNum(_threadNum),minOldInvalidSize(_minOldInvalidSize),kBloomZoom(_kBloomZoom),rollLen(_rollLen),
  R(_R),hitRateThreshold(_hitRateThreshold),cacheBlockSize(_cacheBlockSize),oldSize(oldStream?oldStream->streamSize:0){
    if (((size_t)(newData_end-newData)>=rollLen)&&(oldSize>=rollLen)&&(minOldInvalidSize>0)){
        bloomFilter.buildMatchCache(newData,newData_end,threadNum,kBloomZoom,rollLen);
        _scanAndSmooth(oldStream);
    }
}

void TOldInvalidFilter::_processHit(unsigned char hit,hpatch_StreamPos_t p,std::vector<unsigned char>& ring,
                                    size_t& hitCount,hpatch_StreamPos_t& curInvalidStart){
    const size_t kWin=R*2+1;
    const size_t ri=(size_t)(p%kWin);
    if (p<=R){
        ring[kWin-1-ri]=hit;
        ring[ri]=hit;
        if (p==R){ //ring[0..R] filled now
            hitCount=0;
            for (size_t k=0;k<kWin;++k) hitCount+=ring[k];
        }
    }else{
        const hpatch_StreamPos_t i=p-R-1;
        _checkInvalid(i,hitCount,curInvalidStart);
        hitCount+=(size_t)hit-ring[ri];
        ring[ri]=hit;
    }
}

void TOldInvalidFilter::_checkInvalid(hpatch_StreamPos_t i, size_t hitCount,hpatch_StreamPos_t& curInvalidStart){
    const size_t kWin=R*2+1;
    bool isInvalid=((hpatch_StreamPos_t)hitCount*256 < (hpatch_StreamPos_t)kWin*hitRateThreshold);
    if (isInvalid){
        if (curInvalidStart==hpatch_kNullStreamPos) curInvalidStart=(i>R)?i-R:0;
    }else{
        if (curInvalidStart!=hpatch_kNullStreamPos){
            hdiff_TRange r={curInvalidStart,(hpatch_StreamPos_t)(i+R-1)};
            if (r.endPos-r.beginPos>=minOldInvalidSize) invalidRanges.push_back(r);
            curInvalidStart=hpatch_kNullStreamPos;
        }
    }
}

void TOldInvalidFilter::_scanAndSmooth(const hpatch_TStreamInput* oldStream){
    if (oldSize<minOldInvalidSize) return;
    if (oldSize<(R+1)) return;

    const size_t kWin=R*2+1;
    std::vector<unsigned char> ring(kWin,0);
    size_t hitCount=0;
    hpatch_StreamPos_t curInvalidStart=hpatch_kNullStreamPos;

    size_t _cacheBlockSize=cacheBlockSize;
    if (_cacheBlockSize<rollLen) _cacheBlockSize=rollLen;
    cacheBlock.realloc(_cacheBlockSize+rollLen-1);

    unsigned char* cache=cacheBlock.data();
    hpatch_StreamPos_t curPos=0;
    hpatch_StreamPos_t p=0;

    while (curPos<oldSize){
        unsigned char* readDst=(curPos==0)?cache:(cache+(rollLen-1));
        size_t readLen=_cacheBlockSize;
        if (curPos+readLen>oldSize) readLen=(size_t)(oldSize-curPos);
        if (!oldStream->read(oldStream,curPos,readDst,readDst+readLen))
            throw std::runtime_error("TOldInvalidFilter::_scanAndSmooth() oldStream read error!");

        const size_t totalBytes=readDst+readLen-cache;
        if (totalBytes<rollLen) break;

        const unsigned char* cur=cache;
        TFastMatchForSString::THash h=TFastMatchForSString::getHash(cur,rollLen);
        _processHit((unsigned char)bloomFilter.isHit(h),p,ring,hitCount,curInvalidStart);
        ++p;
        cur+=rollLen;
        for (size_t j=1;j<(totalBytes-rollLen+1);++j,++p,++cur){
            h=TFastMatchForSString::rollHash(h,cur,rollLen);
            _processHit((unsigned char)bloomFilter.isHit(h),p,ring,hitCount,curInvalidStart);
        }

        curPos+=readLen;
        memmove(cache,cache+totalBytes-(rollLen-1),(rollLen-1));
    }
    for (;p<oldSize+R;++p){
        const size_t ri=(size_t)(p%kWin);
        _processHit(ring[ri],p,ring,hitCount,curInvalidStart);
    }

    if (curInvalidStart!=hpatch_kNullStreamPos){
        hdiff_TRange r={curInvalidStart,oldSize};
        if (r.endPos-r.beginPos>=minOldInvalidSize) invalidRanges.push_back(r);
    }
}

void TMatchBlockMem::getBlockCovers(){
    if (matchBlockSize==0) return;
    get_match_covers_by_stream(newData,newData_end,oldData,oldData_end,
                               blockCovers,matchBlockSize,threadNum);
}

void TMatchBlockStream::getBlockCovers(){
    if (matchBlockSize==0) return;
    const hdiff_TMTSets_s mtsets={threadNum,threadNumForStream,false,false};
    get_match_covers_by_stream(newStream,oldStream,blockCovers,matchBlockSize,&mtsets);
}


void TMatchBlockBase::_getOldPackedCover(hpatch_StreamPos_t oldDataSize){
    const size_t blockCount=blockCovers.size();
    if (!invalidOldRanges.empty()){
        //append invalidOldRanges as virtual covers with invalid newPos, to exclude them from packedCoversForOld
        const hpatch_StreamPos_t kInvalidMaxNewPos=~(hpatch_StreamPos_t)0;
        for (size_t i=0;i<invalidOldRanges.size();++i){
            TCover c={invalidOldRanges[i].beginPos,kInvalidMaxNewPos,
                      invalidOldRanges[i].endPos-invalidOldRanges[i].beginPos};
            blockCovers.push_back(c);
        }
    }
    std::sort(blockCovers.begin(),blockCovers.end(),cover_cmp_by_old_t<hpatch_TCover>());
    _getPackedCovers<false>(oldDataSize,blockCovers,packedCoversForOld);
    std::sort(blockCovers.begin(),blockCovers.end(),cover_cmp_by_new_t<hpatch_TCover>());
    blockCovers.resize(blockCount);
}
void TMatchBlockBase::_getNewPackedCover(hpatch_StreamPos_t newDataSize){
    _getPackedCovers<true>(newDataSize,blockCovers,packedCoversForNew);
}

    
    static unsigned char* doPackData(unsigned char* data,unsigned char* data_end,
                                     const std::vector<TPackedCover>& packedCovers){
        unsigned char* dst=data;
        for (size_t i=0;i<packedCovers.size();++i){
            const TPackedCover& cv=packedCovers[i];
            assert(dst==data+cv.newPos);
            memmove(dst,data+cv.oldPos,(size_t)cv.length);
            dst+=cv.length;
        }
        return dst;
    }
void TMatchBlockMem::packData(){
    _getNewPackedCover(newData_end-newData);
    newData_end_cur=doPackData(newData,newData_end,packedCoversForNew);

    invalidOldRanges.clear();
    if (isRemoveOldInvalid){
        hpatch_TStreamInput oldStream;
        mem_as_hStreamInput(&oldStream,oldData,oldData_end);
        TOldInvalidFilter filter(newData,newData_end_cur,&oldStream,threadNum);
        invalidOldRanges.swap(filter.getInvalidRanges());
    }
    _getOldPackedCover(oldData_end-oldData);
    _clearV(invalidOldRanges);
    oldData_end_cur=doPackData(oldData,oldData_end,packedCoversForOld);
}

    static inline hpatch_StreamPos_t _getPackedSize(const std::vector<TPackedCover>& packedCovers){
        const TPackedCover* cv=packedCovers.empty()?0:&packedCovers[packedCovers.size()-1];
        return cv?cv->newPos+cv->length:0;
    }
    static bool loadPackData(unsigned char* dst_begin,unsigned char* dst_end,
                             const hpatch_TStreamInput* srcStream,const std::vector<TPackedCover>& packedCovers){
        unsigned char* dst=dst_begin;
        for (size_t i=0;i<packedCovers.size();++i){
            const TPackedCover& cv=packedCovers[i];
            assert(dst==dst_begin+cv.newPos);
            if (!srcStream->read(srcStream,cv.oldPos,dst,dst+cv.length)) return false;
            dst+=cv.length;
        }
        assert(dst==dst_end);
        return true;
    }
void TMatchBlockStream::packData(){
    _getNewPackedCover(newStream->streamSize);
    const hpatch_StreamPos_t packedNewSize=_getPackedSize(packedCoversForNew);
    _check(packedNewSize==(size_t)packedNewSize,"TMatchBlockStream::packData() packedNewSize");
    _packedNewMem.realloc((size_t)packedNewSize);
    newData=_packedNewMem.data();
    newData_end_cur=newData+packedNewSize;
    _out_diff_info("  load new data into memory from new stream ...\n");
    _check(loadPackData(newData,newData_end_cur,newStream,packedCoversForNew),"loadPackData(newStream)");

    invalidOldRanges.clear();
    if (isRemoveOldInvalid){
        TOldInvalidFilter filter(newData,newData_end_cur,oldStream,threadNum);
        invalidOldRanges.swap(filter.getInvalidRanges());
    }
    _getOldPackedCover(oldStream->streamSize);
    _clearV(invalidOldRanges);
    const hpatch_StreamPos_t packedOldSize=_getPackedSize(packedCoversForOld);
    if (packedOldSize) _check(packedOldSize==(size_t)packedOldSize,"TMatchBlockStream::packData() packedOldSize");
    _packedOldMem.realloc((size_t)packedOldSize);
    oldData=_packedOldMem.data();
    oldData_end_cur=oldData+packedOldSize;
    _out_diff_info("  load old data into memory from old stream ...\n");
    _check(loadPackData(oldData,oldData_end_cur,oldStream,packedCoversForOld),"loadPackData(oldStream)");
}

TMatchBlockStream::TMatchBlockStream(const hpatch_TStreamInput* _newStream,const hpatch_TStreamInput* _oldStream,
                                     size_t _matchBlockSize,size_t _threadNumForMem,size_t _threadNumForStream,
                                     bool _isRemoveOldInvalid)
:TMatchBlockBase(_matchBlockSize,_threadNumForMem),
 newData(0),newData_end_cur(0),oldData(0),oldData_end_cur(0),newStream(_newStream),oldStream(_oldStream),
 threadNumForStream(_threadNumForStream),isRemoveOldInvalid(_isRemoveOldInvalid),
 _newStreamMap(_newStream,packedCoversForNew),
 _oldStreamMap(_oldStream,packedCoversForOld),_isUnpacked(false){
    assert(_oldStream);
}
TMatchBlockStream::~TMatchBlockStream(){
}

    template<bool isNew> static 
    void sortCover(hpatch_TCover* pcovers,size_t coverCount){
        if (coverCount==0) return;
        if (isNew)
            std::sort(pcovers,pcovers+coverCount,cover_cmp_by_new_t<hpatch_TCover>());
        else
            std::sort(pcovers,pcovers+coverCount,cover_cmp_by_old_t<hpatch_TCover>());
    }


    template<bool isNew> static 
    void _clipCover(hpatch_TCover* pcovers,size_t coverCount,
                    const std::vector<TPackedCover>& packedCovers,
                    std::vector<TCover>& out_clipCovers){
        if (packedCovers.empty()) return;
        const TPackedCover* clipCur=packedCovers.data();
        hpatch_StreamPos_t  clipPos=clipCur->newPos;
        hpatch_StreamPos_t  unpackLen=0;
        const TPackedCover* clipEnd=clipCur+packedCovers.size();
        for (size_t i=0;i<coverCount;++i){
            hpatch_TCover& s=pcovers[i];
            hpatch_StreamPos_t sbegin=_cover_pos(isNew,&s);
            while (sbegin>=clipPos){
                _check(clipCur!=clipEnd,"error clip cover");
                unpackLen=clipCur->oldPos-clipCur->newPos;
                ++clipCur;
                if (clipCur!=clipEnd)
                    clipPos=clipCur->newPos;
                else
                    clipPos=hpatch_kNullStreamPos;
            }
            const TPackedCover* clipCuri=clipCur;
            hpatch_StreamPos_t  clipPosi=clipPos;
            hpatch_StreamPos_t  unpackLeni=unpackLen;
            do{
                if (sbegin+s.length<=clipPosi){
                    _cover_pos(isNew,&s)+=unpackLeni;
                    break; //ok next cover
                }
                hpatch_TCover _c={s.oldPos,s.newPos,clipPosi-sbegin};
                _cover_pos(isNew,&_c)+=unpackLeni;
                out_clipCovers.push_back(_c);
                s.oldPos+=_c.length;
                s.newPos+=_c.length;
                s.length-=_c.length;
                sbegin=_cover_pos(isNew,&s);
                while (sbegin>=clipPosi){
                    _check(clipCuri!=clipEnd,"error clip cover");
                    unpackLeni=clipCuri->oldPos-clipCuri->newPos;
                    ++clipCuri;
                    if (clipCuri!=clipEnd)
                        clipPosi=clipCuri->newPos;
                    else
                        clipPosi=hpatch_kNullStreamPos;
                }
            } while (true);
        }
    }
    template<bool isNew> static 
    void doClipCover(hpatch_TCover* pcovers,size_t coverCount,
                    const std::vector<TPackedCover>& packedCovers,
                    std::vector<TCover>& out_clipCovers){
        if (coverCount==0) return;
        _clipCover<isNew>(pcovers,coverCount,packedCovers,out_clipCovers);
    }

    static void doUnpackData(unsigned char* data,unsigned char* data_end,
                             const std::vector<TPackedCover>& packedCovers){
        unsigned char* lastEnd=data_end;
        for (size_t i=packedCovers.size();i>0;--i){
            const TPackedCover& cv=packedCovers[i-1];
            unsigned char* dst=data+cv.oldPos;
            memmove(dst,data+cv.newPos,(size_t)cv.length);
            unsigned char* dstEnd=dst+cv.length;
            memset(dstEnd,0,lastEnd-dstEnd);
            lastEnd=dst;
        }
        memset(data+0,0,lastEnd-data);
    }

    #define _insertCovers(icovers){ \
        pcovers=diffi->insertCover(diffi,icovers.data(),icovers.size()); \
        coverCount+=icovers.size(); \
    }

void TMatchBlockBase::_unpackData(IDiffInsertCover* diffi,hpatch_TCover*& pcovers,size_t& coverCount){
    std::vector<TCover> clipCovers;
    doClipCover<true>(pcovers,coverCount,packedCoversForNew,clipCovers);
    _insertCovers(clipCovers);

    sortCover<false>(pcovers,coverCount);
    clipCovers.clear();
    doClipCover<false>(pcovers,coverCount,packedCoversForOld,clipCovers);
    _insertCovers(clipCovers);
    _clearV(clipCovers);
    _insertCovers(blockCovers);
    _clearV(blockCovers);

    sortCover<true>(pcovers,coverCount);
}

void TMatchBlockMem::unpackData(IDiffInsertCover* diffi,hpatch_TCover* pcovers,size_t coverCount){
    doUnpackData(oldData,oldData_end,packedCoversForOld);
    oldData_end_cur=oldData_end;
    doUnpackData(newData,newData_end,packedCoversForNew);
    newData_end_cur=newData_end;
    _unpackData(diffi,pcovers,coverCount);
    _clearV(packedCoversForOld);
    _clearV(packedCoversForNew);
}

void TMatchBlockStream::unpackData(IDiffInsertCover* diffi,hpatch_TCover* pcovers,size_t coverCount){
    _unpackData(diffi,pcovers,coverCount);
    _isUnpacked=true;
}
    static inline bool _isHitPackedCover(const TPackedCover* pc,hpatch_StreamPos_t readFromPos){
        return (pc->oldPos<=readFromPos)&(readFromPos<pc->oldPos+pc->length);
    }
    typedef TMatchBlockStream::TStreamInputMap TStreamInputMap;
    static hpatch_BOOL _TStreamInputMap_read(const hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                                       unsigned char* out_data,unsigned char* out_data_end){
        TStreamInputMap* self=(TStreamInputMap*)stream->streamImport;
        //return self->baseStream->read(self->baseStream,readFromPos,out_data,out_data_end); //only for debug test
        while (out_data<out_data_end) {
            const TPackedCover* pc=&self->packedCovers[self->curCoverIndex];
            if (!_isHitPackedCover(pc,readFromPos)){//not hit, research
                const TPackedCover vc={readFromPos,0,~(hpatch_StreamPos_t)0};
                const TPackedCover* const pc0=self->packedCovers.data();
                const TPackedCover* uppc=std::upper_bound(pc0,pc0+self->packedCovers.size(),
                                                          vc,cover_cmp_by_old_t<TPackedCover>());
                assert(uppc>pc0);
                pc=uppc-1;
                self->curCoverIndex=pc-pc0;
                if (!_isHitPackedCover(pc,readFromPos)){//can't hit
                    self->curCoverIndex++;
                    size_t readLen=out_data_end-out_data;
                    readLen=(readFromPos+readLen<=uppc->oldPos)?readLen:(size_t)(uppc->oldPos-readFromPos);
                    memset(out_data,0,readLen);
                    readFromPos+=readLen;
                    out_data+=readLen;
                    continue;
                }
            }

            //hit
            size_t readLen=out_data_end-out_data;
            readLen=(readFromPos+readLen<=pc->oldPos+pc->length)?readLen:(size_t)(pc->oldPos+pc->length-readFromPos);
            memcpy(out_data,self->data+pc->newPos+(readFromPos-pc->oldPos),readLen);
            readFromPos+=readLen;
            self->curCoverIndex+=(readFromPos==pc->oldPos+pc->length)?1:0;
            out_data+=readLen;
        }
        return hpatch_TRUE;
    }
    
    inline static void unpackDataAsStream(TStreamInputMap* self,unsigned char* data,unsigned char* data_end){
        self->streamImport=self;
        self->streamSize=self->baseStream->streamSize;
        self->read=_TStreamInputMap_read;
        self->data=data;
        self->data_end=data_end;
        //sort packedCovers for serach
        std::sort(self->packedCovers.begin(),self->packedCovers.end(),cover_cmp_by_old_t<TPackedCover>());
        TPackedCover pc={0,0,0};
        if (self->packedCovers.empty()||self->packedCovers[0].oldPos>0)
            self->packedCovers.insert(self->packedCovers.begin(),pc);
        pc.oldPos=self->streamSize+1;
        self->packedCovers.push_back(pc);
    }
void TMatchBlockStream::cachedStreams(const hpatch_TStreamInput** pnewData,const hpatch_TStreamInput** poldData){
    //*pnewData=newStream; *poldData=oldStream;    return; //only for debug test
    unpackDataAsStream(&_newStreamMap,newData,newData_end_cur);
    unpackDataAsStream(&_oldStreamMap,oldData,oldData_end_cur);
    *pnewData=&_newStreamMap;
    *poldData=&_oldStreamMap;
}

} //namespace hdiff_private

