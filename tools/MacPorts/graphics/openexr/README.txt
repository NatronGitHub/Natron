The following crash happens on opening Robot__0001.exr in Natron when openexr is compiler with clang-3.9.1 and
-Os
but not
-O
-O2
-O3

#0  Imf_2_2::DwaCompressor::initializeFuncs () at ImfDwaCompressor.cpp:2865
#1  0x00000001a724655d in Imf_2_2::staticInitialize () at ImfHeader.cpp:1276
#2  0x00000001a724624a in Imf_2_2::Header::Header (this=0x7fff5fbe8a78, width=64, height=64, pixelAspectRatio=1, screenWindowCenter=@0x7fff5fbe8aa8, screenWindowWidth=1, lineOrder=Imf_2_2::INCREASING_Y, compression=Imf_2_2::ZIP_COMPRESSION) at ImfHeader.cpp:143
#3  0x00000001a72a6d72 in Imf_2_2::MultiPartInputFile::initialize (this=<value temporarily unavailable, due to optimizations>) at ImfMultiPartInputFile.cpp:315
#4  0x00000001a72a7596 in Imf_2_2::MultiPartInputFile::MultiPartInputFile (this=0x1acccec10, is=@0x1acc1d9e0, numThreads=16, reconstructChunkOffsetTable=true) at ImfMultiPartInputFile.cpp:161
#5  0x00000001a93af9fa in OpenImageIO::v1_6::OpenEXRInput::open ()

