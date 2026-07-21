# [HDiffPatch]
[![release](https://img.shields.io/badge/release-v5.1.0-blue.svg)](https://github.com/sisong/HDiffPatch/releases) 
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/sisong/HDiffPatch/blob/master/LICENSE) 
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-blue.svg)](https://github.com/sisong/HDiffPatch/pulls)
[![+issue Welcome](https://img.shields.io/github/issues-raw/sisong/HDiffPatch?color=green&label=%2Bissue%20welcome)](https://github.com/sisong/HDiffPatch/issues)   
[![release](https://img.shields.io/github/downloads/sisong/HDiffPatch/total?color=blue)](https://github.com/sisong/HDiffPatch/releases)
[![Build Status](https://github.com/sisong/HDiffPatch/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/sisong/HDiffPatch/actions?query=branch%3Amaster)
[![Build status](https://ci.appveyor.com/api/projects/status/t9ow8dft8lt898cv/branch/master?svg=true)](https://ci.appveyor.com/project/sisong/hdiffpatch/branch/master)   
 english | [中文版](README_cn.md)   

[HDiffPatch] is a C\C++ library and command-line tools for **diff** & **patch** between binary files or directories(folder); cross-platform; fast running; create small delta/differential; support large files and limit memory requires when diff & patch.   

[HDiffPatch] defines its own patch file format, this lib is also compatible with the [bsdiff4] & [endsley/bsdiff] patch file format and [compatible](https://github.com/sisong/HDiffPatch/issues/369#issuecomment-1869798843) with the [open-vcdiff] & [xdelta3] patch file format [VCDIFF(RFC 3284)].   

if need patch (OTA) on embedded systems,MCU,NB-IoT..., see demo [HPatchLite], with [tinyuz] decompressor can run on 1KB RAM devices! HPatchLite also supports a simple inplace-patch implementation to support storage-constrained devices.   

update your own Android Apk? Jar or Zip file diff & patch? try [ApkDiffPatch], to create smaller delta/differential! NOTE: *ApkDiffPatch can't be used by Android app store, because it requires re-signing apks before diff.*   

[sfpatcher] not require re-signing apks (like [archive-patcher]), is designed for Android app store, create smaller patch file for apk, patch speed up by a factor of xx than archive-patcher & run with O(1) memory.   

if you not have the old versions(too many or not obtain or have been modified), thus cannot create the delta in advance.
you can see sync demo [hsynz] (like [zsync]), the new version is only need released once,
and the owners of the old version get the information about the new version and do the diff&patch themselves.
hsynz support zstd compressor & run much faster than zsync; also compatible with the zsync's file format.   
additional, if you have the new file locally & not the old file, but can get a hash certificate file(.hsyni) of the old file,
you can also create a hpatchz format patch file(usage scenario like [rsync]); see the demo cmdline **hsign_diff** in [hsynz].   
   
NOTE: *This library does not deal with file metadata, such as file last write time, permissions, link file, etc... To this library, a file is just as a stream of bytes; You can extend this library or use other tools.*   
   

[HDiffPatch]: https://github.com/sisong/HDiffPatch
[hsynz]: https://github.com/sisong/hsynz
[ApkDiffPatch]: https://github.com/sisong/ApkDiffPatch
[sfpatcher]: https://github.com/sisong/sfpatcher
[HPatchLite]: https://github.com/sisong/HPatchLite
[tinyuz]: https://github.com/sisong/tinyuz
[bsdiff4]: http://www.daemonology.net/bsdiff/
[endsley/bsdiff]: https://github.com/mendsley/bsdiff
[xdelta3]: https://github.com/jmacd/xdelta
[open-vcdiff]: https://github.com/google/open-vcdiff
[archive-patcher]: https://github.com/google/archive-patcher
[zsync]: http://zsync.moria.org.uk
[rsync]: https://github.com/RsyncProject/rsync
[VCDIFF(RFC 3284)]: https://www.rfc-editor.org/rfc/rfc3284

---
## Releases/Binaries
[Download from latest release](https://github.com/sisong/HDiffPatch/releases) : Command line app for Windows, Linux, MacOS; and .so patch lib for Android.   
use cmdline to create a delta:   
`$hdiffz oldPath newPath outDiffFile -WD -s-64`   
Tip: If you want a more precise match (which might use more memory), you could try changing `-s-64` to `-m-4`   
apply the delta:   
`$hpatchz oldPath diffFile outNewPath`   
Tip: `-WD` format supports multi-thread to speed up patching, for example add `-p-5` parameter (multi-thread not recommended on HDD)   
## Build it yourself
```
git clone --recurse-submodules https://github.com/sisong/HDiffPatch5.git <code_dir>
cd <code_dir>
```
or
```
cd <code_dir>
git clone https://github.com/sisong/HDiffPatch.git  HDiffPatch
git clone https://github.com/sisong/libmd5.git  libmd5
git clone https://github.com/sisong/xxHash.git  xxHash
git clone https://github.com/sisong/lzma.git  lzma
git clone https://github.com/sisong/zstd.git  zstd
git clone https://github.com/sisong/zlib.git  zlib
git clone https://github.com/sisong/libdeflate.git  libdeflate
git clone https://github.com/sisong/bzip2.git  bzip2
```

### Linux or MacOS ###
```
cd HDiffPatch
make -j
```
on MacOS, also can build `HDiffPatch/builds/xcode/HDiffPatch.xcworkspace` by [`xcode`](https://developer.apple.com/xcode)   
   
### Windows ###
build `HDiffPatch/builds/vc/HDiffPatch.sln` by [`Visual Studio`](https://visualstudio.microsoft.com)
   
### libhpatchz.so for Android ###   
* install [Android NDK](https://developer.android.google.cn/ndk/downloads)
* `$ cd HDiffPatch/builds/android_ndk_jni_mk`
* `$ build_libs.sh`  (or `$ build_libs.bat` on windows, then got \*.so files)
* import file `com/github/sisong/HPatch.java` (from `HDiffPatch/builds/android_ndk_jni_mk/java/`) & .so files, java code can call the patch function in libhpatchz.so
   

---
   
## **diff** command line usage:   
diff     usage: **hdiffz** [options] **oldPath newPath outDiffFile**   
compress usage: **hdiffz** [-c-...]  **"" newPath outDiffFile**   
test     usage: **hdiffz**    -t     **oldPath newPath testDiffFile**   
resave   usage: **hdiffz** [-c-...]  **diffFile outDiffFile**   
print     info: **hdiffz** -info **diffFile**   
get   manifest: **hdiffz** [-g#...] [-C-checksumType] **inputPath -M#outManifestTxtFile**   
manifest  diff: **hdiffz** [options] **-M-old#oldManifestFile -M-new#newManifestFile oldPath newPath outDiffFile**   
```
  oldPath newPath inputPath can be file or directory(folder),
  oldPath can empty, and input parameter ""
options:
  -m[-matchScore]
      DEFAULT; all file load into Memory; best diffFileSize;
      requires (newFileSize+ oldFileSize*5(or *9 when oldFileSize>=2GB))+O(1)
        bytes of memory;
      matchScore>=0, DEFAULT -m-4, recommended bin: 0--4 text: 4--9 etc...
  -s[-matchBlockSize]
      all file load as Stream; fast;
      requires O(oldFileSize*16/matchBlockSize+matchBlockSize*5*parallelThreadNumber)bytes of memory;
      matchBlockSize>=4, DEFAULT -s-64, recommended 16,32,48,1k,64k,1m etc...
  -block-fastMatchBlockSize
      must run with -m;
      set block match befor slow byte-by-byte match, DEFAULT -block-1k;
      if set -block-0, means don't use block match;
      fastMatchBlockSize>=4, recommended 128,4k,64k, etc...
      if newData similar to oldData then diff speed++ & diff memory--,
      but small possibility outDiffFile size+
  -cache
      must run with -m;
      set is use a big cache for slow match, DEFAULT false;
      if newData not similar to oldData then diff speed++,
      big cache max used O(oldFileSize) memory, and build slow(diff speed--)
  -w[-oldWinSize-segSize]
      diff by window mode; optimize read old data when patch;
        in -m mode, outDiffFile size+, in -s mode, outDiffFile size--;
        if oldWinSize+ or segSize-, outDiffFile size-, but diff & patch speed--
      oldWinSize: max window bytes on old data, DEFAULT -w-2m;
        for big file, recommended -w-4m-128k, -w-16m-256k, etc...
      segSize: initial data granularity during window matching, DEFAULT oldWinSize/64;
        recommended oldWinSize/128<=segSize<=oldWinSize/4;
  -WD[-stepSize]                (need v5.0 patcher)
      create window diffData(HDIFFW26), optimize read old data when patch;
      recommended as the primary diff format; supports step-by-step patching
        during step-by-step downloading, and supports multi-thread patching!
      requires diff by window mode, auto enable -w-2m if not set;
      default compress by zstd, can set -c-no for no compress;
      default checksum by xxh128, can set -C-no for no checksum;
      stepSize>=4096, DEFAULT -WD-256k, recommended 128k,512k etc...
  -SD[-stepSize]
      create single compressed diffData(HDIFFSF20 format), only need one decompress buffer
      when patch, and support step by step patching when step by step downloading!
        and supports multi-thread patching!
      stepSize>=4096, DEFAULT -SD-256k, recommended 64k,2m etc...
  -BSD
      create diffFile compatible with bsdiff4, unsupport input directory(folder).
      also support run with -SD (not used stepSize), then create single compressed
      diffFile compatible with endsley/bsdiff (https://github.com/mendsley/bsdiff).
  -VCD[-compressLevel[-dictSize]]
      create diffFile compatible with VCDIFF, unsupport input directory(folder).
      DEFAULT no compress, out format same as $open-vcdiff ... or $xdelta3 -S -e -n ...
      if set compressLevel, out format same as $xdelta3 -S lzma -e -n ...
      compress by 7zXZ(xz,lzma), compressLevel in {0..9}, DEFAULT level 7;
      dictSize can like 4096 or 4k or 4m or 16m etc..., DEFAULT 8m
      support compress by multi-thread parallel.
      NOTE: default out diffFile used large source window size!
        need used -w[-oldWinSize-segSize-newWinSize] to set source/segment/target window size.
  -p-parallelThreadNumber
      if parallelThreadNumber>1 then open multi-thread Parallel mode;
      DEFAULT -p-4; requires more memory!
  -p-search-searchThreadNumber
      DEFAULT searchThreadNumber same as parallelThreadNumber;
      old file on HDD hard drives WARNING: multi-thread search need frequent random
        disk reads when -s-matchBlockSize or -block-fastMatchBlockSize(run with -m),
        causes slowdown; at this time, need to close(searchThreadNumber<=1) multi-thread
        search mode or reduce the number of searchThreadNumber!
  -c-compressType[-compressLevel]
      set outDiffFile Compress type, DEFAULT uncompress;
      for -WD, DEFAULT compress by zstd;
      for resave diffFile,recompress diffFile to outDiffFile by new set;
      support compress type & level & dict:
        -c-no                           no compress
        -c-zlib[-{1..9}[-dictBits]]     DEFAULT level 9
            dictBits can 9--15, DEFAULT 15.
            support run by multi-thread parallel, fast!
        -c-ldef[-{1..12}]               DEFAULT level 12
            compatible with -c-zlib, faster or better compress than zlib;
            used libdeflate compressor, & dictBits always 15.
            support run by multi-thread parallel, fast!
        -c-bzip2[-{1..9}]               (or -bz2) DEFAULT level 9
        -c-pbzip2[-{1..9}]              (or -pbz2) DEFAULT level 8
            support run by multi-thread parallel, fast!
            NOTE: code not compatible with it compressed by -c-bzip2!
        -c-lzma[-{0..9}[-dictSize]]     DEFAULT level 7
            dictSize can like 4096 or 4k or 4m or 128m etc..., DEFAULT 8m
            support run by 2-thread parallel.
        -c-lzma2[-{0..9}[-dictSize]]    DEFAULT level 7
            dictSize can like 4096 or 4k or 4m or 128m etc..., DEFAULT 8m
            support run by multi-thread parallel, fast!
            NOTE: code not compatible with it compressed by -c-lzma!
        -c-zstd[-{0..22}[-dictBits]]    DEFAULT level 20
            dictBits can 10--30, DEFAULT 23.
            support run by multi-thread parallel, fast!
  -C-checksumType
      set outDiffFile Checksum type, for directory diff, DEFAULT -C-fadler64; for -WD, DEFAULT -C-xxh128;
      support checksum type:
        -C-no                   no checksum
        -C-crc32
        -C-fadler64
        -C-md5
        -C-xxh3                 (need v4.12 patcher)
        -C-xxh128               recommended (need v4.12 patcher)
  -n-maxOpenFileNumber
      limit Number of open files at same time when stream directory diff;
      maxOpenFileNumber>=16, DEFAULT -n-48, the best limit value by different
        operating system.
  -g#ignorePath[#ignorePath#...]
      set iGnore path list when Directory Diff; ignore path list such as:
        #.DS_Store#desktop.ini#*thumbs*.db#.git*#.svn/#cache_*/00*11/*.tmp
      # means separator between names; (if char # in name, need write #: )
      * means can match any chars in name; (if char * in name, need write *: );
      / at the end of name means match the directory (including its contents);
  -g-old#ignorePath[#ignorePath#...]
      set iGnore path list in oldPath when Directory Diff;
      if oldFile can be changed, need add it in old ignore list;
  -g-new#ignorePath[#ignorePath#...]
      set iGnore path list in newPath when Directory Diff;
      in general, new ignore list should is empty;
  -M#outManifestTxtFile
      create a Manifest file for inputPath; it is a text file, saved infos of
      all files and directoriy list in inputPath; this file while be used in
      manifest diff, support re-checksum data by manifest diff;
      can be used to protect historical versions be modified!
  -M-old#oldManifestFile
      oldManifestFile is created from oldPath; if no oldPath not need -M-old;
  -M-new#newManifestFile
      newManifestFile is created from newPath;
  -D  force run Directory diff between two files; DEFAULT (no -D) run
      directory diff need oldPath or newPath is directory.
  -neq
      open check: if newPath & oldPath's all datas are equal, then return error;
      DEFAULT not check equal.
  -d  Diff only, don't run patch check, DEFAULT run patch check.
  -t  Test only, run patch check, patch(oldPath,testDiffFile)==newPath ?
  -f  Force overwrite, ignore write path already exists;
      DEFAULT (no -f) not overwrite and then return error;
      if used -f and write path is exist directory, will always return error.
  --patch
      swap to hpatchz mode.
  -info
      print infos of diffFile.
  -v  print Version info.
  -h (or -?)
      print usage info.
```
   
## **patch** command line usage:   
patch usage: **hpatchz** [options] **oldPath diffFile outNewPath**   
uncompress usage: **hpatchz** [options] **"" diffFile outNewPath**   
print  info: **hpatchz** -info **diffFile**   
create  SFX: **hpatchz** [-X-exe#selfExecuteFile] **diffFile -X#outSelfExtractArchive**   
run     SFX: **selfExtractArchive** [options] **oldPath -X outNewPath**   
extract SFX: **selfExtractArchive**  (same as: $selfExtractArchive -f {""|".\"} -X ".\")
```
  if oldPath is empty input parameter ""
options:
  -s[-cacheSize]
      DEFAULT -s-8m; oldPath loaded as Stream;
      cacheSize can like 262144 or 256k or 64m or 512m etc....
      if diffFile is window diffData(created by hdiffz -WD-stepSize -w-oldWinSize), then requires
        (cacheSize+ oldWinSize+stepSize + 1*decompress buffer size)+O(1) bytes of memory;
      if diffFile is single compressed diffData(created by hdiffz -SD-stepSize), then requires
        (cacheSize+ stepSize + 1*decompress buffer size)+O(1) bytes of memory;
      if diffFile is compressed diffData(created by hdiffz), then requires
        (cacheSize + 4*decompress buffer size)+O(1) bytes of memory.
      if diffFile is created by hdiffz -BSD,bsdiff4, hdiffz -VCD, then requires
        (cacheSize+ 3*decompress buffer size)+O(1) bytes of memory;
      if diffFile is created by hdiffz -VCD -w,xdelta3,open-vcdiff, then requires
        (cacheSize+ sourceWindowSize+targetWindowSize + 3*decompress buffer size)+O(1) bytes of memory.
  -m  oldPath all loaded into Memory;
      if diffFile is window diffData(created by hdiffz -WD-stepSize -w-oldWinSize), then requires
        (oldWinSize + stepSize + 1*decompress buffer size)+O(1) bytes of memory;
      if diffFile is single compressed diffData(created by hdiffz -SD-stepSize), then requires
        (oldFileSize + stepSize + 1*decompress buffer size)+O(1) bytes of memory.
      if diffFile is compressed diffData(created by hdiffz), then requires
        (oldFileSize + 4*decompress buffer size)+O(1) bytes of memory.
      if diffFile is created by hdiffz -BSD,bsdiff4, then requires
        (oldFileSize + 3*decompress buffer size)+O(1) bytes of memory.
      if diffFile is VCDIFF(created by hdiffz -VCD,hdiffz -VCD -w,xdelta3,open-vcdiff), then requires
        (sourceWindowSize+targetWindowSize + 3*decompress buffer size)+O(1) bytes of memory.
  -p-parallelThreadNumber
      if parallelThreadNumber>1 then open multi-thread Parallel mode;
      now support window diffData(created by hdiffz -WD) and single compressed diffData(created by hdiffz -SD);
      can set 1..5, DEFAULT -p-1!
  -C-checksumSets
      set Checksum data for window patch & directory patch & VCDIFF patch, DEFAULT -C-new-copy;
      checksumSets support (can choose multiple):
        -C-no           no checksum;
        -C-new          checksum new files edited from old reference files;
        -C-diff         checksum diffFile; (VCDIFF unsupport)
        -C-old          checksum old reference files; (VCDIFF unsupport)
        -C-copy         checksum new files copy from old same files, for directory patch;
        -C-all          same as: -C-new-copy-diff-old;
  -n-maxOpenFileNumber
      limit Number of open files at same time when stream directory patch;
      maxOpenFileNumber>=8, DEFAULT -n-24, the best limit value by different
        operating system.
  -f  Force overwrite, ignore write path already exists;
      DEFAULT (no -f) not overwrite and then return error;
      support oldPath outNewPath same path!(patch to tempPath and overwrite old)
      if used -f and outNewPath is exist file:
        if patch output file, will overwrite;
        if patch output directory, will always return error;
      if used -f and outNewPath is exist directory:
        if patch output file, will always return error;
        if patch output directory, will overwrite, but not delete
          needless existing files in directory.
  -info
      print infos of diffFile.
  -v  print Version info.
  -h  (or -?)
      print usage info.
```
   
---
## library API usage:
**diff**&**patch** function in file: `libHDiffPatch/HDiff/diff.h` & `libHDiffPatch/HPatch/patch.h`   
**dir_diff()** & **dir patch** in: `dirDiffPatch/dir_diff/dir_diff.h` & `dirDiffPatch/dir_patch/dir_patch.h`   
### manual:
* **create diff**(in newData,in oldData,out diffData);
   release the diffData for update oldData.  
* **patch**(out newData,in oldData,in diffData);
   ok , get the newData. 
### v1 API, uncompressed diffData:
* **create_diff()**
* **patch()**
* **patch_stream()**
* **patch_stream_with_cache()**
### v2 API, compressed diffData (HDIFF13):
* **create_compressed_diff()**
* **create_compressed_diff_stream()**
* **resave_compressed_diff()**
* **patch_decompress()**
* **patch_decompress_with_cache()**
* **patch_decompress_mem()**
### v3 API, **diff**&**patch** between directories (HDIFF19):
* **dir_diff()**
* **TDirPatcher_\*()** functions with **struct TDirPatcher**
### v4 API, single compressed diffData (HDIFFSF20):
* **create_single_compressed_diff()**
* **create_single_compressed_diff_stream()**
* **resave_single_compressed_diff()**
* **patch_single_stream()**
* **patch_single_stream_mem()**
* **patch_single_compressed_diff()**
* **patch_single_stream_diff()**
### v5 API, window diffData (HDIFFW26):
* **create_window_diff()**
* **patch_window_diff()**
* **resave_window_diff()**
#### hpatch lite API, optimized hpatch on MCU,NB-IoT... (demo [HPatchLite]): 
* **create_lite_diff()**
* **hpatch_lite_open()**
* **hpatch_lite_patch()**
* **create_inplaceB_lite_diff()**
* **hpatchi_inplace_open()**
* **hpatchi_inplaceB()**
#### bsdiff ([bsdiff4] & [endsley/bsdiff]) wrapper API:
* **create_bsdiff()**
* **create_bsdiff_stream()** 
* **bspatch_with_cache()**
#### vcdiff ([open-vcdiff] & [xdelta3]) wrapper API: 
* **create_vcdiff()**
* **create_vcdiff_stream()**
* **vcpatch_with_cache()**
#### hsynz API, diff&patch by sync (demo [hsynz]): 
* **create_sync_data()**
* **create_dir_sync_data()**
* **sync_patch...()**
* **sync_local_diff...()**
* **sync_local_patch...()**
* **create_hdiff_by_sign()** (patched by patch_single_stream(), scenario like [rsync])
#### [zsync] wrapper API, (demo [hsynz]): 
* **create_zsync_data()**
* **zsync_patch...()**
* **zsync_local_diff...()**
* **zsync_local_patch...()**

---
## [HDiffPatch] vs [bsdiff4] & [xdelta3]:
case list([download from OneDrive](https://1drv.ms/u/s!Aj8ygMPeifoQgULlawtabR9lhrQ8)):   
| |newFile <-- oldFile|newSize|oldSize|
|----:|:----|----:|----:|
|1|7-Zip_22.01.win.tar <-- 7-Zip_21.07.win.tar|5908992|5748224|
|2|Chrome_107.0.5304.122-x64-Stable.win.tar <-- 106.0.5249.119|278658560|273026560|
|3|cpu-z_2.03-en.win.tar <-- cpu-z_2.02-en.win.tar|8718336|8643072|
|4|curl_7.86.0.src.tar <-- curl_7.85.0.src.tar|26275840|26030080|
|5|douyin_1.5.1.mac.tar <-- douyin_1.4.2.mac.tar|407940608|407642624|
|6|Emacs_28.2-universal.mac.tar <-- Emacs_27.2-3-universal.mac.tar|196380160|257496064|
|7|FFmpeg-n_5.1.2.src.tar <-- FFmpeg-n_4.4.3.src.tar|80527360|76154880|
|8|gcc_12.2.0.src.tar <-- gcc_11.3.0.src.tar|865884160|824309760|
|9|git_2.33.0-intel-universal-mavericks.mac.tar <-- 2.31.0|73302528|70990848|
|10|go_1.19.3.linux-amd64.tar <-- go_1.19.2.linux-amd64.tar|468835840|468796416|
|11|jdk_x64_mac_openj9_16.0.1_9_openj9-0.26.0.tar <-- 9_15.0.2_7-0.24.0|363765760|327188480|
|12|jre_1.8.0_351-linux-x64.tar <-- jre_1.8.0_311-linux-x64.tar|267796480|257996800|
|13|linux_5.19.9.src.tar <-- linux_5.15.80.src.tar|1269637120|1138933760|
|14|Minecraft_175.win.tar <-- Minecraft_172.win.tar|166643200|180084736|
|15|OpenOffice_4.1.13.mac.tar <-- OpenOffice_4.1.10.mac.tar|408364032|408336896|
|16|postgresql_15.1.src.tar <-- postgresql_14.6.src.tar|151787520|147660800|
|17|QQ_9.6.9.win.tar <-- QQ_9.6.8.win.tar|465045504|464837120|
|18|tensorflow_2.10.1.src.tar <-- tensorflow_2.8.4.src.tar|275548160|259246080|
|19|VSCode-win32-x64_1.73.1.tar <-- VSCode-win32-x64_1.69.2.tar|364025856|340256768|
|20|WeChat_3.8.0.41.win.tar <-- WeChat_3.8.0.33.win.tar|505876992|505018368|
   

**test PC**: Windows11, CPU R9-7945HX, SSD PCIe4.0x4 4T, DDR5 5200MHz 32Gx2   
**Program version**: HDiffPatch5.0.1, hsynz1.1.0, BsDiff4.3, xdelta3.1, zstd1.5.7  
**test Program**:   
**zstd --patch-from** diff with `--ultra -21 --long=24 -f --patch-from={old} {new} -o {pat}`   
 zstd patch with `-d -f --memory=2047MB --patch-from={old} {pat} -o {new}`  
**xdelta3** diff with `-S lzma -e -9 -n -f -s {old} {new} {pat}`   
 xdelta3 patch with `-n -d -f -s {old} {pat} {new}`   
 & adding **hpatchz** test: `hpatchz -f {old} {xdelta3-pat} {new}`   
**xdelta3 -B** diff with `-S lzma -B {oldSize} -e -9 -n -f -s {old} {new} {pat}`   
xdelta3 -B patch with `-B {oldSize} -d -f -s {old} {pat} {new}`   
 & adding **hpatchz** test: `hpatchz -f {old} {xdelta3-B-pat} {new}`   
**bsdiff** diff with `{old} {new} {pat}`   
 bspatch patch with `{old} {new} {pat}`   
 & adding **hpatchz** test: `hpatchz -m -f {old} {bsdiff-pat} {new}`   
 & adding **hpatchz** test: `hpatchz -s -f {old} {bsdiff-pat} {new}`   
**hdiffz -BSD** diff with `-m-6 -BSD -d -f -p-1 {old} {new} {pat}`   
**hdiffz zlib** diff with `-m-6 -SD -d -f -p-1 -c-zlib-9 {old} {new} {pat}`   
**hdiffz lzma2** diff with `-m-6 -SD -d -f -p-1 -c-lzma2-9-16m {old} {new} {pat}`   
**hdiffz zstd** diff with `-m-6 -SD -d -f -p-1 -c-zstd-21-24 {old} {new} {pat}`   
**hdiffz s zlib** diff with `-s-64 -SD -d -f -p-1 -c-zlib-9 {old} {new} {pat}`   
**hdiffz s lzma2** diff with `-s-64 -SD -d -f -p-1 -c-lzma2-9-16m {old} {new} {pat}`   
**hdiffz s zstd** diff with `-s-64 -SD -d -f -p-1 -c-zstd-21-24 {old} {new} {pat}`   
 & adding all **hdiffz**  test with -p-8   
 hpatchz patch with `-f {old} {pat} {new}`    
**hsynz** test, make sync info by `hsync_make -s-2k {new} {out_newi} {out_newz}`,    
 client sync diff&patch by `hsync_demo {old} {newi} {newz} {out_new} -p-1`   
**hsynz p1 zlib** run hsync_make with `-p-1 -c-zlib-9`   
**hsynz p8 zlib** run hsync_make with `-p-8 -c-zlib-9` (run `hsync_demo` with `-p-8`)   
**hsynz p1 zstd** run hsync_make with `-p-1 -c-zstd-21-24`   
**hsynz p8 zstd** run hsync_make with `-p-8 -c-zstd-21-24` (run `hsync_demo` with `-p-8`)   
   
adding test for Window Diff(optimized patch speed)   
`s` mean diff with `-s-64`, `w` mean diff with `-w-2m`, `WD` mean diff with `-WD -w-2m`   
**hdiffz sw p1 -VCD** diff with `-s-64 -w-2m -VCD-9-8m -d -f -p-1 {old} {new} {pat}`   
**hdiffz sw p8 -VCD** diff with `-s-64 -w-2m -VCD-9-8m -d -f -p-8 {old} {new} {pat}`   
**hdiffz w p8 lzma2** diff with `-m-6 -SD -w-2m -d -f -p-8 -c-lzma2-9-16m {old} {new} {pat}`   
**hdiffz w p8 zstd** diff with `-m-6 -SD -w-2m -d -f -p-8 -c-zstd-21-24 {old} {new} {pat}`   
**hdiffz sw p8 lzma2** diff with `-s-64 -SD -w-2m -d -f -p-8 -c-lzma2-9-16m {old} {new} {pat}`   
**hdiffz sw p8 zstd** diff with `-s-64 -SD -w-2m -d -f -p-8 -c-zstd-21-24 {old} {new} {pat}`   
**hdiffz WD p1 lzma2** diff with `-m-6 -WD -w-2m -d -f -p-1 -c-lzma2-9-16m {old} {new} {pat}`   
**hdiffz WD p8 lzma2** diff with `-m-6 -WD -w-2m -d -f -p-8 -c-lzma2-9-16m {old} {new} {pat}`   
**hdiffz WD p1 zstd** diff with `-m-6 -WD -w-2m -d -f -p-1 -c-zstd-21-24 {old} {new} {pat}`   
**hdiffz WD p8 zstd** diff with `-m-6 -WD -w-2m -d -f -p-8 -c-zstd-21-24 {old} {new} {pat}`   
**hdiffz sWD p1 lzma2** diff with `-s-64 -WD -w-2m -d -f -p-1 -c-lzma2-9-16m {old} {new} {pat}`   
**hdiffz sWD p8 lzma2** diff with `-s-64 -WD -w-2m -d -f -p-8 -c-lzma2-9-16m {old} {new} {pat}`   
**hdiffz sWD p1 zstd** diff with `-s-64 -WD -w-2m -d -f -p-1 -c-zstd-21-24 {old} {new} {pat}`   
**hdiffz sWD p8 zstd** diff with `-s-64 -WD -w-2m -d -f -p-8 -c-zstd-21-24 {old} {new} {pat}`   
 & hpatchz patch with `-C-no -p-1 -f {old} {pat} {new}` when diff with `-WD -p-1`   
 & hpatchz patch with `-C-no -p-5 -f {old} {pat} {new}` when diff with `-WD -p-8` or `-SD -w-2m -p-8`   
   
**test result average**:
|Program|compress|diff mem|speed|patch mem|max mem|speed|
|:----|----:|----:|----:|----:|----:|----:|
|bzip2-9 |33.67%||22.9MB/s|||66MB/s|
|zlib-9 |36.53%||19.8MB/s|||539MB/s|
|lzma2-9-16m |25.85%||5.3MB/s|||215MB/s|
|zstd-21-24|27.17%||4.0MB/s|||935MB/s|
||
|zstd --patch-from|7.57%|2730M|5.5MB/s|629M|2306M|1462MB/s|
|xdelta3|13.60%|409M|6.7MB/s|85M|102M|154MB/s|
|xdelta3 +hpatchz|13.60%|409M|6.7MB/s|74M|88M|372MB/s|
|xdelta3 -B|9.63%|2282M|10.5MB/s|460M|2071M|256MB/s|
|xdelta3 -B +hpatchz|9.63%|2282M|10.5MB/s|320M|1106M|467MB/s|
|hdiffz sw p1 -VCD|9.22%|329M|15.9MB/s|34M|36M|526MB/s|
|hdiffz sw p8 -VCD|9.22%|505M|52.0MB/s|32M|36M|522MB/s|
|bsdiff|8.17%|2773M|2.5MB/s|637M|2313M|146MB/s|
|bsdiff +hpatchz -m|8.17%|2773M|2.5MB/s|327M|1107M|188MB/s|
|bsdiff +hpatchz -s|8.17%|2773M|2.5MB/s|19M|19M|84MB/s|
|hdiffz p1 -BSD|7.74%|676M|14.3MB/s|19M|19M|164MB/s|
|hdiffz p8 -BSD|7.73%|662M|33.8MB/s|19M|19M|164MB/s|
||
|hsynz p1 zlib|20.05%|6M|17.3MB/s|6M|22M|273MB/s|
|hsynz p8 zlib|20.05%|30M|115.1MB/s|13M|29M|435MB/s|
|hsynz p1 zstd|14.96%|532M|1.9MB/s|24M|34M|264MB/s|
|hsynz p8 zstd|14.95%|3349M|10.1MB/s|24M|34M|410MB/s|
||
|hdiffz p1 zlib|7.81%|676M|15.3MB/s|8M|9M|674MB/s|
|hdiffz p8 zlib|7.80%|663M|50.5MB/s|8M|9M|727MB/s|
|hdiffz p1 lzma2|6.45%|674M|12.0MB/s|20M|25M|480MB/s|
|hdiffz p8 lzma2|6.45%|666M|36.2MB/s|21M|25M|525MB/s|
|hdiffz p1 zstd|6.74%|682M|12.0MB/s|20M|26M|716MB/s|
|hdiffz p8 zstd|6.74%|1117M|25.3MB/s|20M|26M|752MB/s|
|hdiffz w p8 lzma2|6.49%|666M|26.5MB/s|20M|25M|761MB/s|
|hdiffz w p8 zstd|6.78%|965M|20.4MB/s|20M|26M|1326MB/s|
|hdiffz WD p1 lzma2|6.52%|674M|8.0MB/s|22M|27M|758MB/s|
|hdiffz WD p8 lzma2|6.52%|665M|26.9MB/s|24M|29M|1027MB/s|
|hdiffz WD p1 zstd|6.82%|682M|8.0MB/s|22M|28M|1611MB/s|
|hdiffz WD p8 zstd|6.82%|970M|20.6MB/s|24M|30M|2529MB/s|
||
|hdiffz s p1 zlib|12.52%|80M|45.2MB/s|8M|9M|831MB/s|
|hdiffz s p8 zlib|12.52%|85M|184.1MB/s|8M|9M|828MB/s|
|hdiffz s p1 lzma2|9.11%|169M|17.8MB/s|22M|25M|474MB/s|
|hdiffz s p8 lzma2|9.13%|375M|50.1MB/s|22M|25M|474MB/s|
|hdiffz s p1 zstd|9.58%|195M|17.5MB/s|21M|26M|979MB/s|
|hdiffz s p8 zstd|9.58%|980M|27.4MB/s|21M|26M|969MB/s|
|hdiffz sw p8 lzma2|6.76%|252M|36.1MB/s|20M|25M|609MB/s|
|hdiffz sw p8 zstd|7.07%|851M|26.2MB/s|20M|26M|921MB/s|
|hdiffz sWD p1 lzma2|6.80%|177M|13.0MB/s|22M|27M|721MB/s|
|hdiffz sWD p8 lzma2|6.80%|269M|43.2MB/s|23M|29M|987MB/s|
|hdiffz sWD p1 zstd|7.12%|211M|13.2MB/s|21M|28M|1497MB/s|
|hdiffz sWD p8 zstd|7.12%|870M|29.8MB/s|23M|30M|2440MB/s|
    

## input Apk Files for test: 
case list:
| |app|newFile <-- oldFile|newSize|oldSize|
|----:|:---:|:----|----:|----:|
|1|<img src="https://github.com/sisong/sfpatcher/raw/master/img/cn.wps.moffice_eng.png" width="36">|cn.wps.moffice_eng_13.30.0.apk <-- 13.29.0|95904918|94914262|
|2|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.achievo.vipshop.png" width="36">|com.achievo.vipshop_7.80.2.apk <-- 7.79.9|127395632|120237937|
|3|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.adobe.reader.png" width="36">|com.adobe.reader_22.9.0.24118.apk <-- 22.8.1.23587|27351437|27087718|
|4|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.alibaba.android.rimet.png" width="36">|com.alibaba.android.rimet_6.5.50.apk <-- 6.5.45|195314449|193489159|
|5|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.amazon.mShop.android.shopping.png" width="36">|com.amazon.mShop.android.shopping_24.18.2.apk <-- 24.18.0|76328858|76287423|
|6|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.baidu.BaiduMap.png" width="36">|com.baidu.BaiduMap_16.5.0.apk <-- 16.4.5|131382821|132308374|
|7|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.dragon.read.png" width="36">|com.dragon.read_5.5.3.33.apk <-- 5.5.1.32|45112658|43518658|
|8|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.ebay.mobile.png" width="36">|com.ebay.mobile_6.80.0.1.apk <-- 6.79.0.1|61202587|61123285|
|9|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.eg.android.AlipayGphone.png" width="36">|com.eg.android.AlipayGphone_10.3.0.apk <-- 10.2.96|122073135|119046208|
|10|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.google.android.apps.translate.png" width="36">|com.google.android.apps.translate_6.46.0.apk <-- 6.45.0|48892967|48843378|
|11|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.google.android.googlequicksearchbox.png" width="36">|com.google.android.googlequicksearchbox_13.38.11.apk <-- 13.37.10|190539272|189493966|
|12|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.jingdong.app.mall.png" width="36">|com.jingdong.app.mall_11.3.2.apk <-- 11.3.0|101098430|100750191|
|13|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.netease.cloudmusic.png" width="36">|com.netease.cloudmusic_8.8.45.apk <-- 8.8.40|181914846|181909451|
|14|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.reddit.frontpage.png" width="36">|com.reddit.frontpage_2022.36.0.apk <-- 2022.34.0|50205119|47854461|
|15|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.sankuai.meituan.takeoutnew.png" width="36">|com.sankuai.meituan.takeoutnew_7.94.3.apk <-- 7.92.2|74965893|74833926|
|16|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.sankuai.meituan.png" width="36">|com.sankuai.meituan_12.4.207.apk <-- 12.4.205|93613732|93605911|
|17|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.sina.weibo.png" width="36">|com.sina.weibo_12.10.0.apk <-- 12.9.5|156881776|156617913|
|18|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.smile.gifmaker.png" width="36">|com.smile.gifmaker_10.8.40.27845.apk <-- 10.8.30.27728|102403847|101520138|
|19|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.ss.android.article.news.png" width="36">|com.ss.android.article.news_9.0.7.apk <-- 9.0.6|54444003|53947221|
|20|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.ss.android.ugc.aweme.png" width="36">|com.ss.android.ugc.aweme_22.6.0.apk <-- 22.5.0|171683897|171353597|
|21|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.taobao.taobao.png" width="36">|com.taobao.taobao_10.18.10.apk <-- 10.17.0|117218670|117111874|
|22|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.tencent.mm.png" width="36">|com.tencent.mm_8.0.28.apk <-- 8.0.27|266691829|276603782|
|23|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.tencent.mobileqq.png" width="36">|com.tencent.mobileqq_8.9.15.apk <-- 8.9.13|311322716|310529631|
|24|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.tencent.mtt.png" width="36">|com.tencent.mtt_13.2.0.0103.apk <-- 13.2.0.0045|97342747|97296757|
|25|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.tripadvisor.tripadvisor.png" width="36">|com.tripadvisor.tripadvisor_49.5.apk <-- 49.3|28744498|28695346|
|26|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.twitter.android.png" width="36">|com.twitter.android_9.61.0.apk <-- 9.58.2|36141840|35575484|
|27|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.ubercab.png" width="36">|com.ubercab_4.442.10002.apk <-- 4.439.10002|69923232|64284150|
|28|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.ximalaya.ting.android.png" width="36">|com.ximalaya.ting.android_9.0.66.3.apk <-- 9.0.62.3|115804845|113564876|
|29|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.xunmeng.pinduoduo.png" width="36">|com.xunmeng.pinduoduo_6.30.0.apk <-- 6.29.1|30896833|30951567|
|30|<img src="https://github.com/sisong/sfpatcher/raw/master/img/com.youdao.dict.png" width="36">|com.youdao.dict_9.2.29.apk <-- 9.2.28|110624682|110628778|
|31|<img src="https://github.com/sisong/sfpatcher/raw/master/img/org.mozilla.firefox.png" width="36">|org.mozilla.firefox_105.2.0.apk <-- 105.1.0|83078464|83086656|
|32|<img src="https://github.com/sisong/sfpatcher/raw/master/img/tv.danmaku.bili.png" width="36">|tv.danmaku.bili_7.1.0.apk <-- 7.0.0|104774723|104727005|
   

**changed test Program**:   
**hdiffz ...** `-m-6` changed to `-m-2 -cache`, `-s-64` changed to `-s-16`   
**hsynz ...** make `-s-2k` changed to `-s-1k`   
& adding **hsynz p1**, **hsynz p8** make without compressor   
**archive-patcher** v1.0, diff with `--generate --old {old} --new {new} --patch {pat}`,   
  patch with `--apply --old {old} --patch {pat} --new {new}`   
  NOTE: archive-patcher's delta file compressed by lzma2-9-16m, diff&patch time not include compress&decompress delta file's memory&time.   
**sfpatcher -1 zstd** v1.1.1 diff with `-o-1 -c-zstd-21-23 -m-1 -step-3m -lp-512k -p-8 -cache -d {old} {new} {pat}`   
& **sfpatcher -2 lzma2** diff with `-o-2 -c-lzma2-9-4m -m-1 -step-2m -lp-8m -p-8 -cache -d {old} {new} {pat}`   
 sfpatcher patch with `-lp -p-8 {old} {pat} {new}`   
**sfpatcher -1 clA zstd** v1.3.0 used `$sf_normalize -cl-A` normalized apks before diff   
adding test hpatchz v4.6.9 & sfpatcher on Android, arm CPU Kirin980(2×A76 2.6G + 2×A76 1.92G + 4×A55 1.8G)   
( [archive-patcher], [sfpatcher] optimized diff&patch between apk files )   

**test result average**:
|Program|compress|diff mem|speed|patch mem|max mem|speed|arm Kirin980|
|:----|----:|----:|----:|----:|----:|----:|----:|
|zstd --patch-from|53.15%|2251M|3.2MB/s|204M|599M|1266MB/s|
|xdelta3|54.51%|422M|3.8MB/s|97M|100M|162MB/s|
|xdelta3 +hpatchz|54.51%|422M|3.8MB/s|76M|87M|417MB/s|
|hdiffz sw p1 -VCD|53.73%|288M|9.6MB/s|36M|36M|325MB/s|
|hdiffz sw p8 -VCD|53.73%|459M|27.8MB/s|36M|36M|323MB/s|
|bsdiff|53.84%|931M|1.2MB/s|218M|605M|52MB/s|
|bsdiff +hpatchz -m|53.84%|931M|1.2MB/s|123M|316M|54MB/s|
|bsdiff +hpatchz -s|53.84%|931M|1.2MB/s|19M|19M|51MB/s|
|hdiffz p1 -BSD|53.70%|412M|7.2MB/s|19M|19M|53MB/s|
|hdiffz p8 -BSD|53.72%|416M|15.5MB/s|19M|19M|53MB/s|
||
|hsynz p1|62.43%|4M|1533.5MB/s|4M|10M|236MB/s|
|hsynz p8|62.43%|18M|2336.4MB/s|12M|18M|394MB/s|
|hsynz p1 zlib|58.67%|5M|22.7MB/s|4M|11M|243MB/s|
|hsynz p8 zlib|58.67%|29M|138.6MB/s|12M|19M|410MB/s|
|hsynz p1 zstd|57.74%|534M|2.7MB/s|24M|28M|234MB/s|
|hsynz p8 zstd|57.74%|3434M|13.4MB/s|24M|28M|390MB/s|
||
|hdiffz p1 zlib|53.22%|412M|8.8MB/s|10M|11M|827MB/s|343MB/s|
|hdiffz p8 zlib|53.23%|415M|32.6MB/s|10M|11M|916MB/s|
|hdiffz p1 lzma2|52.92%|429M|4.3MB/s|26M|27M|310MB/s|131MB/s|
|hdiffz p8 lzma2|52.94%|464M|19.4MB/s|26M|27M|327MB/s|
|hdiffz p1 zstd|53.04%|442M|5.7MB/s|25M|27M|1072MB/s|371MB/s|
|hdiffz p8 zstd|53.05%|1164M|11.2MB/s|25M|28M|1104MB/s|
|hdiffz w p8 lzma2|53.24%|426M|14.4MB/s|26M|27M|322MB/s|
|hdiffz w p8 zstd|53.35%|1044M|9.3MB/s|24M|28M|1135MB/s|
|hdiffz WD p1 lzma2|53.27%|417M|3.2MB/s|27M|27M|312MB/s|
|hdiffz WD p8 lzma2|53.27%|426M|14.6MB/s|29M|29M|353MB/s|
|hdiffz WD p1 zstd|53.38%|425M|3.9MB/s|25M|28M|1266MB/s|
|hdiffz WD p8 zstd|53.38%|1045M|9.4MB/s|22M|30M|1877MB/s|
||
|hdiffz s p1 zlib|53.73%|100M|27.0MB/s|9M|11M|780MB/s|
|hdiffz s p8 zlib|53.73%|104M|97.1MB/s|9M|11M|885MB/s|
|hdiffz s p1 lzma2|53.30%|193M|6.5MB/s|25M|27M|296MB/s|
|hdiffz s p8 lzma2|53.30%|312M|32.4MB/s|25M|27M|317MB/s|
|hdiffz s p1 zstd|53.44%|218M|10.3MB/s|24M|27M|1021MB/s|
|hdiffz s p8 zstd|53.44%|1049M|14.3MB/s|23M|28M|1091MB/s|
|hdiffz sw p8 lzma2|53.26%|302M|22.4MB/s|26M|27M|322MB/s|
|hdiffz sw p8 zstd|53.37%|1044M|12.1MB/s|25M|28M|1086MB/s|
|hdiffz sWD p1 lzma2|53.29%|196M|4.7MB/s|27M|27M|315MB/s|
|hdiffz sWD p8 lzma2|53.29%|303M|23.1MB/s|29M|29M|352MB/s|
|hdiffz sWD p1 zstd|53.41%|222M|6.5MB/s|25M|28M|1281MB/s|
|hdiffz sWD p8 zstd|53.41%|1045M|12.2MB/s|21M|30M|1874MB/s|
||
|archive-patcher|31.65%|1448M|0.9MB/s|558M|587M|14MB/s|
|sfpatcher-1 p1 zstd|31.08%|818M|2.3MB/s|15M|19M|201MB/s|92MB/s|
|sfpatcher-1 p8 zstd|31.07%|1025M|4.6MB/s|18M|25M|424MB/s|189MB/s|
|sfpatcher-2 p1 lzma2|24.11%|976M|2.1MB/s|15M|20M|37MB/s|19MB/s|
|sfpatcher-2 p8 lzma2|24.15%|968M|5.0MB/s|20M|26M|108MB/s|45MB/s|
|sfpatcher-1 p1 clA zstd|21.72%|1141M|2.5MB/s|19M|22M|85MB/s|63MB/s|
|sfpatcher-1 p8 clA zstd|21.74%|1156M|5.4MB/s|26M|29M|240MB/s|129MB/s|
    

---
## Contact
housisong@hotmail.com  

