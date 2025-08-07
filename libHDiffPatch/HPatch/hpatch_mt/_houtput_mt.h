//  houtput_mt.h
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
#ifndef _houtput_mt_h
#define _houtput_mt_h
#include "_hpatch_mt.h"
#ifdef __cplusplus
extern "C" {
#endif
#if (_IS_USED_MULTITHREAD)

struct houtput_mt_t;

size_t               houtput_mt_t_memSize();

// create a new houtput_mt_t* wrapper base_stream;
//   start a thread to write data to base_stream
struct houtput_mt_t* houtput_mt_open(struct houtput_mt_t* pmem,size_t memSize,struct hpatch_mt_t* h_mt,
                                     const hpatch_TStreamOutput* base_stream,hpatch_StreamPos_t curWritePos);
// write data to base_stream
hpatch_BOOL          houtput_mt_write(struct houtput_mt_t* self,struct hpatch_TWorkBuf* data);
// wait write data thread end and free self
hpatch_BOOL          houtput_mt_close(struct houtput_mt_t* self);

#endif
#ifdef __cplusplus
}
#endif
#endif //_houtput_mt_h
