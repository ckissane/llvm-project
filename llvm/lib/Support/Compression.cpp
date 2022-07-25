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

constexpr SupportCompressionType CompressionAlgorithm::AlgorithmId;
constexpr StringRef CompressionAlgorithm::name;
constexpr int CompressionAlgorithm::BestSpeedCompression;
constexpr int CompressionAlgorithm::DefaultCompression;
constexpr int CompressionAlgorithm::BestSizeCompression;

bool CompressionAlgorithm::supported() { return false; };

void CompressionAlgorithm::compress(ArrayRef<uint8_t> Input,
                                    SmallVectorImpl<uint8_t> &CompressedBuffer,
                                    int Level) {
  llvm_unreachable("method:\"compress\" is unsupported for compression "
                   "algorithm:\"base\", reason:\"can't call on base\"");
};
Error CompressionAlgorithm::decompress(ArrayRef<uint8_t> Input,
                                       uint8_t *UncompressedBuffer,
                                       size_t &UncompressedSize) {
  llvm_unreachable("method:\"decompress\" is unsupported for compression "
                   "algorithm:\"base\", reason:\"can't call on base\"");
}

constexpr SupportCompressionType NoneCompressionAlgorithm::AlgorithmId;
constexpr StringRef NoneCompressionAlgorithm::name;
constexpr int NoneCompressionAlgorithm::BestSpeedCompression;
constexpr int NoneCompressionAlgorithm::DefaultCompression;
constexpr int NoneCompressionAlgorithm::BestSizeCompression;

bool NoneCompressionAlgorithm::supported() { return true; };

void NoneCompressionAlgorithm::compress(
    ArrayRef<uint8_t> Input, SmallVectorImpl<uint8_t> &CompressedBuffer,
    int Level) {
  unsigned long CompressedSize = Input.size();
  CompressedBuffer.resize_for_overwrite(CompressedSize);
  // SmallVectorImpl<uint8_t>()
  CompressedBuffer.assign(SmallVector<uint8_t>(Input.begin(), Input.end()));

  // Tell MemorySanitizer that zlib output buffer is fully initialized.
  // This avoids a false report when running LLVM with uninstrumented ZLib.
  __msan_unpoison(CompressedBuffer.data(), CompressedSize);
  if (CompressedSize < CompressedBuffer.size())
    CompressedBuffer.truncate(CompressedSize);
};
Error NoneCompressionAlgorithm::decompress(ArrayRef<uint8_t> Input,
                                           uint8_t *UncompressedBuffer,
                                           size_t &UncompressedSize) {
  // Tell MemorySanitizer that zlib output buffer is fully initialized.
  // This avoids a false report when running LLVM with uninstrumented ZLib.
  if (UncompressedSize < Input.size()) {
    return make_error<StringError>("decompressed buffer target size too small",
                                   inconvertibleErrorCode());
  }
  UncompressedSize = Input.size();
  memcpy(UncompressedBuffer, Input.data(), UncompressedSize);

  __msan_unpoison(UncompressedBuffer, UncompressedSize);
  return Error::success();
}

constexpr SupportCompressionType ZlibCompressionAlgorithm::AlgorithmId;
constexpr StringRef ZlibCompressionAlgorithm::name;
constexpr int ZlibCompressionAlgorithm::BestSpeedCompression;
constexpr int ZlibCompressionAlgorithm::DefaultCompression;
constexpr int ZlibCompressionAlgorithm::BestSizeCompression;

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

bool ZlibCompressionAlgorithm::supported() { return true; };

void ZlibCompressionAlgorithm::compress(
    ArrayRef<uint8_t> Input, SmallVectorImpl<uint8_t> &CompressedBuffer,
    int Level) {
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
Error ZlibCompressionAlgorithm::decompress(ArrayRef<uint8_t> Input,
                                           uint8_t *UncompressedBuffer,
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
bool ZlibCompressionAlgorithm::supported() { return false; };

void ZlibCompressionAlgorithm::compress(
    ArrayRef<uint8_t> Input, SmallVectorImpl<uint8_t> &CompressedBuffer,
    int Level) {
  llvm_unreachable(
      "method:\"compress\" is unsupported for compression algorithm:\"zlib\", "
      "reason:\"llvm not compiled with zlib support\"");
};
Error ZlibCompressionAlgorithm::decompress(ArrayRef<uint8_t> Input,
                                           uint8_t *UncompressedBuffer,
                                           size_t &UncompressedSize) {
  llvm_unreachable(
      "method:\"decompress\" is unsupported for compression "
      "algorithm:\"zlib\", reason:\"llvm not compiled with zlib support\"");
};

#endif

constexpr SupportCompressionType ZStdCompressionAlgorithm::AlgorithmId;
constexpr StringRef ZStdCompressionAlgorithm::name;
constexpr int ZStdCompressionAlgorithm::BestSpeedCompression;
constexpr int ZStdCompressionAlgorithm::DefaultCompression;
constexpr int ZStdCompressionAlgorithm::BestSizeCompression;

#if LLVM_ENABLE_ZSTD

bool ZStdCompressionAlgorithm::supported() { return true; };

void ZStdCompressionAlgorithm::compress(
    ArrayRef<uint8_t> Input, SmallVectorImpl<uint8_t> &CompressedBuffer,
    int Level) {
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
Error ZStdCompressionAlgorithm::decompress(ArrayRef<uint8_t> Input,
                                           uint8_t *UncompressedBuffer,
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
bool ZStdCompressionAlgorithm::supported() { return false; };

void ZStdCompressionAlgorithm::compress(
    ArrayRef<uint8_t> Input, SmallVectorImpl<uint8_t> &CompressedBuffer,
    int Level) {
  llvm_unreachable(
      "method:\"compress\" is unsupported for compression algorithm:\"zstd\", "
      "reason:\"llvm not compiled with zstd support\"");
};
Error ZStdCompressionAlgorithm::decompress(ArrayRef<uint8_t> Input,
                                           uint8_t *UncompressedBuffer,
                                           size_t &UncompressedSize) {
  llvm_unreachable(
      "method:\"decompress\" is unsupported for compression "
      "algorithm:\"zstd\", reason:\"llvm not compiled with zstd support\"");
};

#endif

llvm::compression::CompressionAlgorithm
llvm::compression::CompressionAlgorithmFromId(uint8_t CompressionSchemeId) {
  llvm::compression::CompressionAlgorithm CompressionScheme =
      llvm::compression::CompressionAlgorithm();
  switch (CompressionSchemeId) {
  case static_cast<uint8_t>(
      llvm::compression::NoneCompressionAlgorithm().AlgorithmId):
    CompressionScheme = llvm::compression::NoneCompressionAlgorithm();
    break;
  case static_cast<uint8_t>(
      llvm::compression::ZlibCompressionAlgorithm().AlgorithmId):
    CompressionScheme = llvm::compression::ZlibCompressionAlgorithm();
    break;
  case static_cast<uint8_t>(
      llvm::compression::ZStdCompressionAlgorithm().AlgorithmId):
    CompressionScheme = llvm::compression::ZStdCompressionAlgorithm();
    break;
  default:
    break;
  }
  return CompressionScheme;
}