//===--- Compression.cpp - Compression implementation ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file implements compression functions.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Compression.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/config.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"
#if LLVM_ENABLE_ZLIB
#include <zlib.h>
#endif
#if LLVM_ENABLE_ZSTD
#include <zstd.h>
#endif

using namespace llvm;
using namespace llvm::compression;

namespace {

#if LLVM_ENABLE_ZLIB

static StringRef convertZlibCodeToString(int Code) {
  switch (Code) {
  case Z_MEM_ERROR:
    return "zlib error: Z_MEM_ERROR";
  case Z_BUF_ERROR:
    return "zlib error: Z_BUF_ERROR";
  case Z_STREAM_ERROR:
    return "zlib error: Z_STREAM_ERROR";
  case Z_DATA_ERROR:
    return "zlib error: Z_DATA_ERROR";
  case Z_OK:
  default:
    llvm_unreachable("unknown or unexpected zlib status code");
  }
}
#endif
struct ZlibCompressionAlgorithm : public CompressionAlgorithm {
#if LLVM_ENABLE_ZLIB

  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer, int Level) {
    unsigned long CompressedSize = ::compressBound(Input.size());
    CompressedBuffer.resize_for_overwrite(CompressedSize);
    int Res = ::compress2((Bytef *)CompressedBuffer.data(), &CompressedSize,
                          (const Bytef *)Input.data(), Input.size(), Level);
    if (Res == Z_MEM_ERROR)
      report_bad_alloc_error("Allocation failed");
    assert(Res == Z_OK);
    // Tell MemorySanitizer that zlib output buffer is fully initialized.
    // This avoids a false report when running LLVM with uninstrumented ZLib.
    __msan_unpoison(CompressedBuffer.data(), CompressedSize);
    if (CompressedSize < CompressedBuffer.size())
      CompressedBuffer.truncate(CompressedSize);
  };
  Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                   size_t &UncompressedSize) {
    int Res =
        ::uncompress((Bytef *)UncompressedBuffer, (uLongf *)&UncompressedSize,
                     (const Bytef *)Input.data(), Input.size());
    // Tell MemorySanitizer that zlib output buffer is fully initialized.
    // This avoids a false report when running LLVM with uninstrumented ZLib.
    __msan_unpoison(UncompressedBuffer, UncompressedSize);
    return Res ? make_error<StringError>(convertZlibCodeToString(Res),
                                         inconvertibleErrorCode())
               : Error::success();
  };

#else

  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer, int Level) {
    llvm_unreachable("method:\"compress\" is unsupported for compression "
                     "algorithm:\"zlib\", "
                     "reason:\"llvm not compiled with zlib support\"");
  };
  Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                   size_t &UncompressedSize) {
    llvm_unreachable(
        "method:\"decompress\" is unsupported for compression "
        "algorithm:\"zlib\", reason:\"llvm not compiled with zlib support\"");
  };

#endif

protected:
  friend CompressionAlgorithm *CompressionKind::operator->() const;
  ZlibCompressionAlgorithm() : CompressionAlgorithm("zlib", 1, 6, 9) {}
};

struct ZStdCompressionAlgorithm : public CompressionAlgorithm {
#if LLVM_ENABLE_ZSTD

  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer, int Level) {
    unsigned long CompressedBufferSize = ::ZSTD_compressBound(Input.size());
    CompressedBuffer.resize_for_overwrite(CompressedBufferSize);
    unsigned long CompressedSize =
        ::ZSTD_compress((char *)CompressedBuffer.data(), CompressedBufferSize,
                        (const char *)Input.data(), Input.size(), Level);
    if (ZSTD_isError(CompressedSize))
      report_bad_alloc_error("Allocation failed");
    // Tell MemorySanitizer that zstd output buffer is fully initialized.
    // This avoids a false report when running LLVM with uninstrumented ZLib.
    __msan_unpoison(CompressedBuffer.data(), CompressedSize);
    if (CompressedSize < CompressedBuffer.size())
      CompressedBuffer.truncate(CompressedSize);
  };
  Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                   size_t &UncompressedSize) {
    const size_t Res =
        ::ZSTD_decompress(UncompressedBuffer, UncompressedSize,
                          (const uint8_t *)Input.data(), Input.size());
    UncompressedSize = Res;
    // Tell MemorySanitizer that zstd output buffer is fully initialized.
    // This avoids a false report when running LLVM with uninstrumented ZLib.
    __msan_unpoison(UncompressedBuffer, UncompressedSize);
    return ZSTD_isError(Res) ? make_error<StringError>(ZSTD_getErrorName(Res),
                                                       inconvertibleErrorCode())
                             : Error::success();
  };

#else

  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer, int Level) {
    llvm_unreachable("method:\"compress\" is unsupported for compression "
                     "algorithm:\"zstd\", "
                     "reason:\"llvm not compiled with zstd support\"");
  };
  Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                   size_t &UncompressedSize) {
    llvm_unreachable(
        "method:\"decompress\" is unsupported for compression "
        "algorithm:\"zstd\", reason:\"llvm not compiled with zstd support\"");
  };

#endif

protected:
  friend CompressionAlgorithm *CompressionKind::operator->() const;
  ZStdCompressionAlgorithm() : CompressionAlgorithm("zstd", 1, 5, 12) {}
};

struct UnknownCompressionAlgorithm : public CompressionAlgorithm {

  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer, int Level) {
    llvm_unreachable("method:\"compress\" is unsupported for compression "
                     "algorithm:\"unknown\", reason:\"can't call on unknown\"");
  };
  Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                   size_t &UncompressedSize) {
    llvm_unreachable("method:\"decompress\" is unsupported for compression "
                     "algorithm:\"unknown\", reason:\"can't call on unknown\"");
  }

protected:
  friend CompressionAlgorithm *CompressionKind::operator->() const;
  UnknownCompressionAlgorithm()
      : CompressionAlgorithm("unknown", -999, -999, -999) {}
};

} // namespace

CompressionAlgorithm *CompressionKind::operator->() const {
  switch (uint8_t(CompressionID)) {
  case uint8_t(CompressionKind::Zlib):
    static ZlibCompressionAlgorithm ZlibI;
    return &ZlibI;
  case uint8_t(CompressionKind::ZStd):
    static ZStdCompressionAlgorithm ZStdI;
    return &ZStdI;
  default:
    static UnknownCompressionAlgorithm UnknownI;
    return &UnknownI;
  }
}