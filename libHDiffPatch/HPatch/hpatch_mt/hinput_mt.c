//  hinput_mt_t.c
//  hpatch
/*
 The MIT License (MIT)
 Copyright (c) 2025 HouSisong
 
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
#include "hinput_mt.h"
#include "patch_private_mt.h"
#if (_IS_USED_MULTITHREAD)

typedef struct hinput_mt_t{
    hpatch_TStreamInput         base;
    const hpatch_TStreamInput*  base_stream;
    struct hpatch_mt_t*         h_mt;
    size_t                      workBufSize;
    hpatch_TWorkBuf*            curDataBuf;
    size_t                      curDataBuf_pos;
    hpatch_StreamPos_t          endReadPos;
    volatile hpatch_StreamPos_t curReadPos;
    volatile hpatch_TWorkBuf*   freeBufList;
    volatile hpatch_TWorkBuf*   dataBufList;
    volatile hpatch_BOOL        isOnError;
#if (defined(_DEBUG) || defined(DEBUG))
    hpatch_StreamPos_t          curOutedPos;
    volatile hpatch_BOOL        threadIsRunning;
#endif
    HLocker     _locker;
    HCondvar    _waitCondvar;
} hinput_mt_t;

    static hpatch_BOOL hinput_mt_read_(const struct hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                                       unsigned char* out_data,unsigned char* out_data_end);

hpatch_inline static
hpatch_BOOL _hinput_mt_init(hinput_mt_t* self,struct hpatch_mt_t* h_mt,hpatch_TWorkBuf* freeBufList,
                            const hpatch_TStreamInput* base_stream,hpatch_StreamPos_t curReadPos,hpatch_StreamPos_t endReadPos){
    memset(self,0,sizeof(*self));
    assert(endReadPos<=base_stream->streamSize);
    assert(freeBufList);
    self->base.streamImport=self;
    self->base.streamSize=base_stream->streamSize;
    self->base.read=hinput_mt_read_;
    self->h_mt=h_mt;
    self->freeBufList=freeBufList;
    self->workBufSize=hpatch_mt_workBufSize(h_mt);
    self->curReadPos=curReadPos;
    self->endReadPos=endReadPos;
#if (defined(_DEBUG) || defined(DEBUG))
    self->curOutedPos=curReadPos;
#endif
    
    self->_locker=c_locker_new();
    self->_waitCondvar=c_condvar_new();
    return (self->_locker)&&(self->_waitCondvar);
}
static void _hinput_mt_free(hinput_mt_t* self){
    if (self==0) return;
#if (defined(_DEBUG) || defined(DEBUG))
    c_locker_enter(self->_locker);
    assert(!self->threadIsRunning);
    c_locker_leave(self->_locker);
#endif
    if (self->_waitCondvar) c_condvar_delete(self->_waitCondvar);
    if (self->_locker) c_locker_delete(self->_locker);
}

static void hinput_mt_setOnError_(hinput_mt_t* self) {
    hpatch_BOOL isNeedSetOnError=hpatch_FALSE;
    c_locker_enter(self->_locker);
    isNeedSetOnError=(!self->isOnError);
    if (isNeedSetOnError){
        self->isOnError=hpatch_TRUE;
        c_condvar_signal(self->_waitCondvar);
    }
    c_locker_leave(self->_locker);
    if (isNeedSetOnError)
        hpatch_mt_setOnError(self->h_mt);
}

static hpatch_BOOL _hinput_mt_readAData(hinput_mt_t* self,hpatch_TWorkBuf* data){
    hpatch_StreamPos_t readPos=self->curReadPos;
    size_t readLen=self->workBufSize;
    if (readPos+readLen>self->endReadPos)
        readLen=(size_t)(self->endReadPos-readPos);
    self->curReadPos+=readLen;
    data->data_size=readLen;
    return self->base_stream->read(self->base_stream,readPos,TWorkBuf_data(data),TWorkBuf_data_end(data));
}


static void hinput_thread_(int threadIndex,void* workData){
    hinput_mt_t* self=(hinput_mt_t*)workData;
    while ((!hpatch_mt_isOnError(self->h_mt))&&(self->curReadPos<self->endReadPos)){
        hpatch_TWorkBuf* wbuf=0;
        hpatch_BOOL _isOnError;
        c_locker_enter(self->_locker);
        if (!self->isOnError){
            wbuf=TWorkBuf_popABuf(&self->freeBufList);
            if (wbuf==0)
                c_condvar_wait(self->_waitCondvar,self->_locker);
        }
        _isOnError=self->isOnError;
        c_locker_leave(self->_locker);
        if (_isOnError) break;
        
        if (wbuf){
            if (_hinput_mt_readAData(self,wbuf)){
                c_locker_enter(self->_locker);
                TWorkBuf_pushABufAtEnd(&self->dataBufList,wbuf);
                c_condvar_signal(self->_waitCondvar);
                c_locker_leave(self->_locker);
            }else{
                hinput_mt_setOnError_(self);
                break;
            }
        }
    }

    hpatch_mt_onThreadEnd(self->h_mt);
#if (defined(_DEBUG) || defined(DEBUG))
    c_locker_enter(self->_locker);
    self->threadIsRunning=hpatch_FALSE;
    c_locker_leave(self->_locker);
#endif
}
static hpatch_BOOL hinput_mt_read_(const hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                                   unsigned char* out_data,unsigned char* out_data_end){
    hinput_mt_t* self=(hinput_mt_t*)stream->streamImport;
    hpatch_BOOL result=hpatch_TRUE;
#if (defined(_DEBUG) || defined(DEBUG))
    assert(self->curOutedPos==readFromPos);
    self->curOutedPos+=(out_data_end-out_data);
#endif
    while (result&&(out_data<out_data_end)){
        if (self->curDataBuf){
            size_t readLen=self->curDataBuf->data_size-self->curDataBuf_pos;
            readLen=(readLen<(size_t)(out_data_end-out_data))?readLen:(size_t)(out_data_end-out_data);
            memcpy(out_data,TWorkBuf_data(self->curDataBuf)+self->curDataBuf_pos,readLen);
            self->curDataBuf_pos+=readLen;
            out_data+=readLen;
            if (self->curDataBuf_pos==self->curDataBuf->data_size){
                c_locker_enter(self->_locker);
                TWorkBuf_pushABufAtHead(&self->freeBufList,self->curDataBuf);
                c_condvar_signal(self->_waitCondvar);
                result=(!self->isOnError);
                c_locker_leave(self->_locker);
                self->curDataBuf=0;
            }
        }else{
            c_locker_enter(self->_locker);
            self->curDataBuf=TWorkBuf_popABuf(&self->dataBufList);
            if (self->curDataBuf==0)
                c_condvar_wait(self->_waitCondvar,self->_locker);
            c_locker_leave(self->_locker);
        }
    }
    return result;
}

size_t hinput_mt_t_memSize(){
    return sizeof(hinput_mt_t);
}

hpatch_TStreamInput* hinput_mt_open(void* pmem,size_t memSize,struct hpatch_mt_t* h_mt,hpatch_TWorkBuf* freeBufList,
                                    const hpatch_TStreamInput* base_stream,hpatch_StreamPos_t curReadPos,hpatch_StreamPos_t endReadPos){
    if (memSize<hinput_mt_t_memSize()) return 0;
    hinput_mt_t* self=(hinput_mt_t*)pmem;
    if (!_hinput_mt_init(self,h_mt,freeBufList,base_stream,curReadPos,endReadPos))
        goto _on_error;

    //start a thread to read
    if (!hpatch_mt_beforeThreadBegin(self->h_mt))
        goto _on_error;
#if (defined(_DEBUG) || defined(DEBUG))
    self->threadIsRunning=hpatch_TRUE;
#endif
    if (!c_thread_parallel(1,hinput_thread_,self,hpatch_FALSE,0)){
        hpatch_mt_onThreadEnd(self->h_mt);
    #if (defined(_DEBUG) || defined(DEBUG))
        self->threadIsRunning=hpatch_FALSE;
    #endif
        goto _on_error;
    }

    return &self->base;

_on_error:
    _hinput_mt_free(self);
    return 0;
}

hpatch_BOOL hinput_mt_close(const hpatch_TStreamInput* hinput_mt_stream){
    hpatch_BOOL result;
    hinput_mt_t* self=0;
    if (!hinput_mt_stream) return hpatch_TRUE;
    self=(hinput_mt_t*)hinput_mt_stream->streamImport;
    if (!self) return hpatch_TRUE;

    result=(!self->isOnError)&&(self->curReadPos==self->endReadPos);
    _hinput_mt_free(self);
    ((hpatch_TStreamInput*)hinput_mt_stream)->streamImport=0;
    return result;
}

#endif //_IS_USED_MULTITHREAD
