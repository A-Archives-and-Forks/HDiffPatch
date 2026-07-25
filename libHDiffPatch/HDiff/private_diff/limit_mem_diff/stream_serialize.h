//stream_serialize.h
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
#ifndef stream_serialize_h
#define stream_serialize_h
#include "../../diff_types.h"
#include "../pack_uint.h" //for packUInt_fixSize
#include "../mem_buf.h"
#include "../bytes_rle.h"
#include "../../../HPatch/checksum_plugin.h"

struct hdiff_TCompress;
namespace hdiff_private{

struct TCoversStream:public hpatch_TStreamInput{
    TCoversStream(const TInputCovers& _covers,hpatch_StreamPos_t cover_buf_size);
    ~TCoversStream();
    static hpatch_StreamPos_t getDataSize(const TInputCovers& covers);
private:
    const TInputCovers          covers;
    TAutoMem                    _code_mem;
    size_t                      curCodePos;
    size_t                      curCodePos_end;
    size_t                      readedCoverCount;
    hpatch_StreamPos_t          lastOldEnd;
    hpatch_StreamPos_t          lastNewEnd;
    hpatch_StreamPos_t          _readFromPos_back;
    enum { kCodeBufSize = hdiff_kFileIOBufBestSize };
    
    static hpatch_BOOL _read(const hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                             unsigned char* out_data,unsigned char* out_data_end);
};


struct TNewDataSubDiffStream:public hpatch_TStreamInput{
    TNewDataSubDiffStream(const hdiff_TStreamInput* _newData,const hdiff_TStreamInput* _oldData,
                          const TInputCovers& _covers,bool _isOnlySubCover,bool _isExtendedCover);
    inline ~TNewDataSubDiffStream(){ assert(curReadPos==streamSize); }
private:
    hpatch_StreamPos_t curReadNewPos;
    hpatch_StreamPos_t curReadOldPos;
    hpatch_StreamPos_t curReadPos;
    hpatch_StreamPos_t curDataLen;
    size_t nextCoveri;
    const hdiff_TStreamInput* newData;
    const hdiff_TStreamInput* oldData;
    const TInputCovers covers;
    const bool isOnlySubCover;
    const bool isExtendCover;
    TAutoMem _cache;
    void initRead();
    void readTo(unsigned char* out_data,unsigned char* out_data_end);
    static hpatch_BOOL _read(const struct hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                             unsigned char* out_data,unsigned char* out_data_end);
};

struct TNewDataSubDiffStream_mem:public TNewDataSubDiffStream{
    TNewDataSubDiffStream_mem(const unsigned char* newData,const unsigned char* newData_end,
                              const unsigned char* oldData,const unsigned char* oldData_end,
                              const TInputCovers& _covers,bool _isOnlySubCover,bool _isExtendedCover);
private:
    hdiff_TStreamInput mem_newData;
    hdiff_TStreamInput mem_oldData;
};

struct TNewDataDiffStream:public hpatch_TStreamInput{
    inline TNewDataDiffStream(const TInputCovers& _covers,const hpatch_TStreamInput* _newData,
                              hpatch_StreamPos_t newDataDiff_size)
            :covers(_covers),newData(_newData) { _init(newDataDiff_size); }
    inline TNewDataDiffStream(const TInputCovers& _covers,const hpatch_TStreamInput* _newData)
        :covers(_covers),newData(_newData) { _init(getDataSize(_covers,_newData->streamSize)); }
    inline TNewDataDiffStream(const TInputCovers& _covers,const hpatch_TStreamInput* _newData,
                              size_t coveri,hpatch_StreamPos_t newDataPos,hpatch_StreamPos_t newDataPosEnd)
        :covers(_covers),newData(_newData){ _initByRange(coveri,newDataPos,newDataPosEnd); }
    static hpatch_StreamPos_t getDataSize(const TInputCovers& covers,hpatch_StreamPos_t newDataSize);
private:
    static hpatch_StreamPos_t getDataSizeByRange(const TInputCovers& covers,size_t coveri,
                                                 hpatch_StreamPos_t newDataPos,hpatch_StreamPos_t newDataPosEnd);
    void _init(hpatch_StreamPos_t newDataDiff_size);
    void _initByRange(size_t coveri,hpatch_StreamPos_t newDataPos,hpatch_StreamPos_t newDataPosEnd);
    const TInputCovers          covers;
    const hpatch_TStreamInput*  newData;
    hpatch_StreamPos_t          curNewPos;
    hpatch_StreamPos_t          curNewPos_end;
    hpatch_StreamPos_t          lastNewEnd;
    size_t                      readedCoverCount;
    hpatch_StreamPos_t          _readFromPos_back;

    //by range
    size_t                      _coveri;
    hpatch_StreamPos_t          _newDataPos;

    static hpatch_BOOL _read(const hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                             unsigned char* out_data,unsigned char* out_data_end);
};

struct TNewDataSubDiffCoverStream:public hpatch_TStreamInput{
    TNewDataSubDiffCoverStream(const hpatch_TStreamInput* _newStream,
                               const hpatch_TStreamInput* _oldStream,bool _isExtendedCover);
    void resetCover(const TCover& _cover);
    void resetCoverLen(hpatch_StreamPos_t coverLen);
    const bool isExtendCover;
private:
    hpatch_StreamPos_t inStreamLen;
    size_t curDataLen;
    const hpatch_TStreamInput* newStream;
    const hpatch_TStreamInput* oldStream;
    TCover cover;
    unsigned char* newData;
    unsigned char* oldData;
    TAutoMem _cache;
    void initRead();
    hpatch_BOOL _updateCache(hpatch_StreamPos_t readFromPos);
    hpatch_BOOL readTo(hpatch_StreamPos_t readFromPos,unsigned char* out_data,unsigned char* out_data_end);
    static hpatch_BOOL _read(const hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                             unsigned char* out_data,unsigned char* out_data_end);
};

class TStreamClip:public hpatch_TStreamInput{
public:
    inline explicit TStreamClip():_decompressHandle(0){ clear(); }
    inline TStreamClip(const hpatch_TStreamInput* stream,
                         hpatch_StreamPos_t clipBeginPos,hpatch_StreamPos_t clipEndPos,
                         hpatch_TDecompress* decompressPlugin=0,hpatch_StreamPos_t uncompressSize=0)
                         :_decompressHandle(0){ reset(stream,clipBeginPos,clipEndPos,decompressPlugin,uncompressSize); }
    void reset(const hpatch_TStreamInput* stream,
               hpatch_StreamPos_t clipBeginPos,hpatch_StreamPos_t clipEndPos,
               hpatch_TDecompress* decompressPlugin=0,hpatch_StreamPos_t uncompressSize=0);
    inline ~TStreamClip() { clear(); }
    inline void clear() { closeDecompressHandle(); streamSize=0; }
private:
    const hpatch_TStreamInput*  _src;
    hpatch_StreamPos_t          _src_begin;
    hpatch_StreamPos_t          _src_end;
    hpatch_TDecompress*         _decompressPlugin;
    hpatch_decompressHandle     _decompressHandle;
    hpatch_StreamPos_t          _read_uncompress_pos;
    void closeDecompressHandle();
    void openDecompressHandle();
    static hpatch_BOOL _clip_read(const hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                                  unsigned char* out_data,unsigned char* out_data_end);
};

struct TStepStream:public hpatch_TStreamInput{
    TStepStream(const hpatch_TStreamInput* newStream,const hpatch_TStreamInput* oldStream,
                const TInputCovers& covers,size_t patchStepMemSize,bool isExtendCover);
    inline size_t getCoverCount()const{ return endCoverCount; }
    inline size_t getMaxStepMemSize()const{ return endMaxStepMemSize; }
private:
    TNewDataSubDiffCoverStream  subDiff;
    TNewDataDiffStream          newDataDiff;
    const TInputCovers covers;
    const size_t    patchStepMemSize;
    const hpatch_StreamPos_t newDataSize;
    size_t  curCoverCount;
    size_t  endCoverCount;
    size_t  curMaxStepMemSize;
    size_t  endMaxStepMemSize;
    hpatch_StreamPos_t lastOldEnd;
    hpatch_StreamPos_t lastNewEnd;
    std::vector<unsigned char> step_bufCover;
    TSingleStreamRLE0  step_bufRle;
    hpatch_StreamPos_t step_dataDiffSize;
    hpatch_StreamPos_t newDataDiffReadPos;
    bool    isHaveLastCover;
    bool    isHaveLeftCover;
    bool    isHaveRightCover;
    TCover  lastCover;
    TCover  leftCover;
    TCover  rightCover;
    size_t  cur_i;
    TCover  cover;
    const TCover* pCurCover;
    bool    isInInit;
    hpatch_StreamPos_t  sumBufSize_forInit;
    size_t              step_bufCover_size;
    std::vector<unsigned char> step_buf;
    TStreamClip                step_dataDiff;
    hpatch_StreamPos_t readFromPosBack;
    hpatch_StreamPos_t         readBufPos;
    void initStream();
    void beginStep();
    bool doStep();
    void _last_flush_step();
    void _flush_step_code();
    hpatch_BOOL readTo(unsigned char* out_data,unsigned char* out_data_end);
    static hpatch_BOOL _read(const hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                             unsigned char* out_data,unsigned char* out_data_end);
};


struct TDiffStream{
    explicit TDiffStream(const hpatch_TStreamOutput* _out_diff,
                         hpatch_StreamPos_t out_diff_curPos=0);
    ~TDiffStream();
    
    void pushBack(const unsigned char* src,size_t n);
    size_t packUInt(hpatch_StreamPos_t uValue);
    inline TPlaceholder packUInt_pos(hpatch_StreamPos_t uValue){
        hpatch_StreamPos_t pos=writePos;
        packUInt(uValue);
        return TPlaceholder(pos,writePos);
    }
    void packUInt_update(const TPlaceholder& pos,hpatch_StreamPos_t uValue);
    
    hpatch_StreamPos_t pushStream(const hpatch_TStreamInput* stream,
                                  const hdiff_TCompress*     compressPlugin,
                                  const TPlaceholder&        update_compress_sizePos,
                                  bool isMustCompress=false,const hpatch_StreamPos_t cancelSizeOnCancelCompress=0,
                                  bool* out_isCanceledCompress=0);
    hpatch_StreamPos_t pushStream(const hpatch_TStreamInput* stream,
                                  const hdiff_TCompress*     compressPlugin,
                                  bool isMustCompress=false,const hpatch_StreamPos_t cancelSizeOnCancelCompress=0,
                                  bool* out_isCanceledCompress=0){
                TPlaceholder nullPos(0,0); return pushStream(stream,compressPlugin,nullPos,
                                                    isMustCompress,cancelSizeOnCancelCompress,out_isCanceledCompress); }
    hpatch_StreamPos_t pushStream(const hpatch_TStreamInput* stream){
                _pushStream(stream); return stream->streamSize; }
    hpatch_StreamPos_t getWritedPos()const{ return writePos; }
    void stream_read(const TPlaceholder& pos,hpatch_byte* out_data);
    inline TPlaceholder pushBack_pos(const hpatch_byte* src,size_t n){
        hpatch_StreamPos_t pos=writePos;
        pushBack(src,n);
        return TPlaceholder(pos,writePos);
    }
    void stream_update(const TPlaceholder& pos,const hpatch_byte* in_data);
private:
    const hpatch_TStreamOutput*  out_diff;
    hpatch_StreamPos_t     writePos;
    enum{ kBufSize=hdiff_kFileIOBufBestSize };
    TAutoMem               _temp_mem;
    
    void _packUInt_limit(hpatch_StreamPos_t uValue,size_t limitOutSize);
    
    //stream->read can return currently readed data size,return <0 error
    void _pushStream(const hpatch_TStreamInput* stream);
};

struct TVectorAsStreamOutput:public hpatch_TStreamOutput{
    explicit TVectorAsStreamOutput(std::vector<unsigned char>& _dst);
    std::vector<unsigned char>& dst;
};
struct TVectorAsStreamInput:public hpatch_TStreamInput{
    explicit TVectorAsStreamInput(const std::vector<unsigned char>& _src)
    :src(_src){ mem_as_hStreamInput(this,src.data(),src.data()+src.size()); }
    const std::vector<unsigned char>& src;
};



#define _test_rt(value) { if (!(value)) { LOG_ERR("check "#value" error!\n");  return hpatch_FALSE; } }

struct _TCheckOutNewDataStream:public hpatch_TStreamOutput{
    _TCheckOutNewDataStream(const hpatch_TStreamInput* _newData,unsigned char* _buf,size_t _bufSize);
    bool isWriteFinish()const{ return writedLen==newData->streamSize; }
private:
    const hpatch_TStreamInput*  newData;
    hpatch_StreamPos_t          writedLen;
    unsigned char*              buf;
    size_t                      bufSize;
    static hpatch_BOOL _read_writed(const struct hpatch_TStreamOutput* stream,hpatch_StreamPos_t readFromPos,
                                    unsigned char* out_data,unsigned char* out_data_end);
    static hpatch_BOOL _write_check(const hpatch_TStreamOutput* stream,hpatch_StreamPos_t writeToPos,
                                    const unsigned char* data,const unsigned char* data_end);
};


struct TChecksumOutputStream:public hpatch_TStreamOutput{
    TChecksumOutputStream();
    void init(const hpatch_TStreamOutput* realOut,hpatch_TChecksum* checksumPlugin,
              hpatch_checksumHandle checksumHandle,bool isChecksuming,hpatch_StreamPos_t realOut_offset=0);
    inline void setAutoChecksumBeginPos(hpatch_StreamPos_t autoBeginPos) { _autoChecksumBeginPos=autoBeginPos; }
    inline void setIsChecksuming(bool isChecksuming) { _isChecksuming=isChecksuming; }
private:
    const hpatch_TStreamOutput*  _realOut;
    hpatch_TChecksum*            _checksumPlugin;
    hpatch_checksumHandle        _checksumHandle;
    hpatch_StreamPos_t           _writePos;
    hpatch_StreamPos_t           _realOut_offset;
    hpatch_StreamPos_t           _autoChecksumBeginPos;
    bool                         _isChecksuming;
    static hpatch_BOOL _write(const hpatch_TStreamOutput* stream,hpatch_StreamPos_t writeToPos,
                              const unsigned char* data,const unsigned char* data_end);
};

struct TChecksumInputStream:public hpatch_TStreamInput{
    TChecksumInputStream();
    void init(const hpatch_TStreamInput* realIn,hpatch_TChecksum* checksumPlugin,
              hpatch_checksumHandle checksumHandle,bool isChecksuming);
    inline void setIsChecksuming(bool isChecksuming) { _isChecksuming=isChecksuming; }
    inline void setIsRandomAccess(bool isRandomAccess) { _isRandomAccess=isRandomAccess; }
private:
    const hpatch_TStreamInput*  _realIn;
    hpatch_TChecksum*           _checksumPlugin;
    hpatch_checksumHandle       _checksumHandle;
    hpatch_StreamPos_t          _checkedPos;
    bool                        _isChecksuming;
    bool                        _isRandomAccess;
    static hpatch_BOOL _read(const hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                              unsigned char* out_data,unsigned char* out_data_end);
};


struct TWindowDiffStream:public hpatch_TStreamInput{
    TWindowDiffStream(const hpatch_TStreamInput* newStream,const hpatch_TStreamInput* oldStream,
                      const TInputCovers& covers,const std::vector<hpatch_TWindow>& windows,
                      size_t patchStepMemSize,bool isExtendCover,size_t& outMaxStepMemSize,
                      size_t& outMaxSubCoverCount,hpatch_StreamPos_t& outMaxWindowOldLength,
                      hpatch_StreamPos_t windowMetaCount,const unsigned char* extraData,size_t extraDataSize);
    ~TWindowDiffStream();
private:
    struct WindowData{
        std::vector<TCover>         adjustedCovers;
        hpatch_TWindow              adjustedWindow;
        std::vector<unsigned char>  metadataBuf; // packUInt(subCoverCount) only
        hpatch_StreamPos_t          stepDataSize;
        size_t                      subCoverCount;
    };
    const hpatch_TStreamInput*      _newStream;
    const hpatch_TStreamInput*      _oldStream;
    size_t                          _patchStepMemSize;
    bool                            _isExtendCover;
    std::vector<WindowData>         _windowDatas;
    size_t                          _curWindowIndex;
    size_t                          _curMetadataPos;
    hpatch_StreamPos_t              _curStepReadPos;
    TStepStream*                    _curStepStream;
    TAutoMem                        _stepStreamMem;
    TAutoMem                        _oldStreamMem;
    TStreamClip                     _newClip;
    TStreamClip                     _oldClip;
    hpatch_TStreamInput             _oldMemStream;
    hpatch_StreamPos_t              _readFromPos_back;
    const unsigned char* const      _extraData;
    const size_t                    _extraDataSize;
    hpatch_StreamPos_t                       _windowMetaCount;// metadata batch support
    std::vector<std::vector<unsigned char> > _metaBatchDatas; // pre-packed batch data
    std::vector<hpatch_StreamPos_t>          _metaBatchPositions;// stream pos of each batch
    size_t                                   _nextMetaBatchIdx; // next batch to emit
    void _clear_windowStepStream();
    void _init_windowStepStream(size_t wi,bool isMustLoadOldToMem);
    void _buildMetaBatches(const std::vector<hpatch_TWindow>& windows);
    void _appendMetaBatch(const std::vector<hpatch_TWindow>& windows,
                          size_t start,size_t count,hpatch_StreamPos_t& lastOldPos);
    static hpatch_BOOL _read(const hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                             unsigned char* out_data,unsigned char* out_data_end);
};


void do_compress(std::vector<unsigned char>& out_code,const hpatch_TStreamInput* data,
                 const hdiff_TCompress* compressPlugin,bool isMustCompress=false);
static inline void do_compress(std::vector<unsigned char>& out_code,const std::vector<unsigned char>& data,
                               const hdiff_TCompress* compressPlugin,bool isMustCompress=false){
    hpatch_TStreamInput dataStream;
    mem_as_hStreamInput(&dataStream,data.data(),data.data()+data.size());
    do_compress(out_code,&dataStream,compressPlugin,isMustCompress);
}

}//namespace hdiff_private
#endif
