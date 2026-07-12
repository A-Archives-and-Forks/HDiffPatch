// match_block.h
// hdiff
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
#ifndef hdiff_match_block_h
#define hdiff_match_block_h
#include "../diff_types.h"
#include "suffix_string.h"
#include "mem_buf.h"
#include <vector>
namespace hdiff_private{

    //identify large invalid regions in old data before diffing:
    //build a bloom filter from new data's rolling hashes,
    //stream old data positions against the filter through a small ring buffer,
    //applying a sliding-window hit-rate threshold to output invalid ranges.
    struct TOldInvalidFilter{
        TOldInvalidFilter(const unsigned char* newData,const unsigned char* newData_end,
                          const hpatch_TStreamInput* oldStream,size_t threadNum,
                          size_t minOldInvalidSize=1024/2,size_t kBloomZoom=6,size_t rollLen=5,
                          size_t R=32, size_t hitRateThreshold=64, //hitRateThreshold percent 0-256
                          size_t cacheBlockSize=(1<<20)); //1MB
        inline std::vector<hdiff_TRange>& getInvalidRanges(){ return invalidRanges; }
    private:
        void _scanAndSmooth(const hpatch_TStreamInput* oldStream);
        void _checkInvalid(hpatch_StreamPos_t i, size_t hitCount,hpatch_StreamPos_t& curInvalidStart);
        void _processHit(unsigned char hit,hpatch_StreamPos_t p,std::vector<unsigned char>& ring,
                         size_t& hitCount,hpatch_StreamPos_t& curInvalidStart);
        TFastMatchForSString       bloomFilter;
        TAutoMem                   cacheBlock;     //cacheBlockSize+rollLen-1 bytes
        std::vector<hdiff_TRange>  invalidRanges;
        const size_t               threadNum;
        const size_t               minOldInvalidSize;
        const size_t               kBloomZoom;
        const size_t               rollLen;
        const size_t               R;
        const size_t               hitRateThreshold; //percent 0-100
        const size_t               cacheBlockSize;
        hpatch_StreamPos_t         oldSize;
    };

    struct TMatchBlockBase{
        typedef hpatch_TCover TPackedCover;
        TMatchBlockBase(size_t _matchBlockSize,size_t _threadNum)
        :matchBlockSize(_matchBlockSize),threadNum(_threadNum){}
		inline void swapBlockCovers(std::vector<TCover>& _blockCovers){ blockCovers.swap(_blockCovers); }
    protected:
        void _getNewPackedCover(hpatch_StreamPos_t newDataSize);
        void _getOldPackedCover(hpatch_StreamPos_t oldDataSize);
        void _unpackData(IDiffInsertCover* diffi,hpatch_TCover*& pcovers,size_t& coverCount);
        const size_t   matchBlockSize;
        const size_t   threadNum;
        std::vector<TCover> blockCovers;
        std::vector<TPackedCover> packedCoversForOld;
        std::vector<TPackedCover> packedCoversForNew;
        std::vector<hdiff_TRange> invalidOldRanges;
    };

    //remove some big match block befor diff, in memory
    struct TMatchBlockMem:public TMatchBlockBase{
        unsigned char* newData;
        unsigned char* newData_end;
        unsigned char* newData_end_cur;
        unsigned char* oldData;
        unsigned char* oldData_end;
        unsigned char* oldData_end_cur;
        const bool     isRemoveOldInvalid;
        TMatchBlockMem(unsigned char* _newData,unsigned char* _newData_end,
                       unsigned char* _oldData,unsigned char* _oldData_end,
                       size_t _matchBlockSize,size_t _threadNumForMem,bool _isRemoveOldInvalid=false)
        :TMatchBlockBase(_matchBlockSize,_threadNumForMem),
         newData(_newData),newData_end(_newData_end),newData_end_cur(_newData_end),
         oldData(_oldData),oldData_end(_oldData_end),oldData_end_cur(_oldData_end),
         isRemoveOldInvalid(_isRemoveOldInvalid){ }
        inline hpatch_StreamPos_t curNewDataSize()const{ return (size_t)(newData_end_cur-newData); }
        inline hpatch_StreamPos_t curOldDataSize()const{ return (size_t)(oldData_end_cur-oldData); }
        void getBlockCovers();
        void packData();
        void unpackData(IDiffInsertCover* diffi,hpatch_TCover* pcovers,size_t coverCount);
    };

    //remove some big match block befor diff, used stream
    struct TMatchBlockStream:public TMatchBlockBase{
        unsigned char* newData;
        unsigned char* newData_end_cur;
        unsigned char* oldData;
        unsigned char* oldData_end_cur;
        const hpatch_TStreamInput* newStream;
        const hpatch_TStreamInput* oldStream;
        const size_t    threadNumForStream;
        const bool      isRemoveOldInvalid;
        TMatchBlockStream(const hpatch_TStreamInput* _newStream,const hpatch_TStreamInput* _oldStream,
                          size_t _matchBlockSize,size_t _threadNumForMem,size_t _threadNumForStream,
                          bool _isRemoveOldInvalid=false);
        ~TMatchBlockStream();
        inline hpatch_StreamPos_t curNewDataSize()const{ return _isUnpacked?newStream->streamSize:(size_t)(newData_end_cur-newData); }
        inline hpatch_StreamPos_t curOldDataSize()const{ return _isUnpacked?oldStream->streamSize:(size_t)(oldData_end_cur-oldData); }
        void getBlockCovers();
        void packData();
        void unpackData(IDiffInsertCover* diffi,hpatch_TCover* pcovers,size_t coverCount);
        void cachedStreams(const hpatch_TStreamInput** pnewData,const hpatch_TStreamInput** poldData);

        struct TStreamInputMap:public hpatch_TStreamInput{
            const hpatch_TStreamInput*  baseStream;
            std::vector<TPackedCover>&  packedCovers;
            size_t                      curCoverIndex;
            const unsigned char*        data; //packed data
            const unsigned char*        data_end;
            inline TStreamInputMap(const hpatch_TStreamInput* _baseStream,std::vector<TPackedCover>& _packedCovers)
                :baseStream(_baseStream),packedCovers(_packedCovers),curCoverIndex(0){}
        };
    protected:
        TStreamInputMap _newStreamMap;
        TStreamInputMap _oldStreamMap;
        TAutoMem        _packedNewMem;
        TAutoMem        _packedOldMem;
        bool            _isUnpacked;
    };

    template<class _TMatchBlock>
    struct TCoversOptim:public ICoverLinesListener{
        explicit TCoversOptim(_TMatchBlock* _matchBlock):matchBlock(_matchBlock){
            ICoverLinesListener* listener=this;
            memset(listener,0,sizeof(*listener));
            insert_cover=_insert_cover;
        }
        _TMatchBlock* matchBlock;
    protected:
        static void _insert_cover(ICoverLinesListener* listener,IDiffInsertCover* diffi,hpatch_TCover* pcovers,size_t coverCount,
                                  hpatch_StreamPos_t* newSize,hpatch_StreamPos_t* oldSize){
            TCoversOptim<_TMatchBlock>* self=(TCoversOptim<_TMatchBlock>*)listener;
            if (self->matchBlock!=0){
                assert(self->matchBlock->curNewDataSize()==*newSize);
                assert(self->matchBlock->curOldDataSize()==*oldSize);
                self->matchBlock->unpackData(diffi,pcovers,coverCount);
                *newSize=self->matchBlock->curNewDataSize();
                *oldSize=self->matchBlock->curOldDataSize();
            }
        }
    };

    struct TCoversOptimMem:public TCoversOptim<TMatchBlockMem>{
        TCoversOptimMem(unsigned char* newData,unsigned char* newData_end,
                       unsigned char* oldData,unsigned char* oldData_end,
                       size_t matchBlockSize,size_t threadNum,bool isRemoveOldInvalid=false)
        :TCoversOptim<TMatchBlockMem>(&_matchBlock),
         _matchBlock(newData,newData_end,oldData,oldData_end,matchBlockSize,threadNum,isRemoveOldInvalid){
            matchBlock->getBlockCovers();
            matchBlock->packData();
        }
    protected:
        TMatchBlockMem _matchBlock;
    };

    struct TCoversOptimStream:public TCoversOptim<TMatchBlockStream>{
        TCoversOptimStream(const hpatch_TStreamInput* newStream,const hpatch_TStreamInput* oldStream,
                       size_t matchBlockSize,size_t threadNumForMem,size_t threadNumForStream,
                       bool isRemoveOldInvalid=false)
        :TCoversOptim<TMatchBlockStream>(&_matchBlock),
         _matchBlock(newStream,oldStream,matchBlockSize,threadNumForMem,threadNumForStream,isRemoveOldInvalid){
            matchBlock->getBlockCovers();
            matchBlock->packData();
        }
        inline void cachedStreams(const hpatch_TStreamInput** pnewData,const hpatch_TStreamInput** poldData){
            _matchBlock.cachedStreams(pnewData,poldData);
        }
    protected:
        TMatchBlockStream _matchBlock;
    };
    
    struct TCoversOptimMem_blockCovers:public TCoversOptim<TMatchBlockMem>{
        TCoversOptimMem_blockCovers(unsigned char* newData,unsigned char* newData_end,
                                    unsigned char* oldData,unsigned char* oldData_end,
                                    std::vector<TCover>& _blockCovers,size_t threadNum)
        :TCoversOptim<TMatchBlockMem>(&_matchBlock),
         _matchBlock(newData,newData_end,oldData,oldData_end,0,threadNum){
            matchBlock->swapBlockCovers(_blockCovers);//got blockCovers
            matchBlock->packData();
        }
    protected:
        TMatchBlockMem _matchBlock;
    };

} //namespace hdiff_private


#endif //hdiff_match_block_h
