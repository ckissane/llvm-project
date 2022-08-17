//===-- llvm/Support/Compression.h ---Compression----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains basic functions for compression/uncompression.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_COMPRESSION_H
#define LLVM_SUPPORT_COMPRESSION_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {
template <typename T> class SmallVectorImpl;
class Error;

namespace compression {

enum class CompressionKind : uint8_t { Zlib = 1, ZStd = 2, Unknown = 255 };

struct CompressionSpec;
struct CompressionImpl;

typedef CompressionSpec *CompressionSpecRef;
typedef CompressionImpl *CompressionImplRef;

CompressionSpecRef getCompressionSpec(uint8_t Kind);
CompressionSpecRef getCompressionSpec(CompressionKind Kind);

struct CompressionSpec {
  const CompressionKind Kind;
  CompressionImpl *Implementation;
  const StringRef Name;
  const StringRef Status; // either "supported", or "unsupported: REASON"

protected:
  friend CompressionSpecRef getCompressionSpec(uint8_t Kind);
  CompressionSpec(CompressionKind Kind, CompressionImpl *Implementation,
                  StringRef Name, bool Supported, StringRef Status)
      : Kind(Kind), Implementation(Supported ? Implementation : nullptr),
        Name(Name), Status(Supported ? "supported" : Status) {}
};

struct CompressionImpl {
  const CompressionKind Kind;
  const int BestSpeedLevel;
  const int DefaultLevel;
  const int BestSizeLevel;
  virtual void compress(ArrayRef<uint8_t> Input,
                        SmallVectorImpl<uint8_t> &CompressedBuffer,
                        int Level) = 0;
  virtual Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                           size_t &UncompressedSize) = 0;
  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer) {
    return compress(Input, CompressedBuffer, DefaultLevel);
  }

  Error decompress(ArrayRef<uint8_t> Input,
                   SmallVectorImpl<uint8_t> &UncompressedBuffer,
                   size_t UncompressedSize) {
    UncompressedBuffer.resize_for_overwrite(UncompressedSize);
    Error E = decompress(Input, UncompressedBuffer.data(), UncompressedSize);
    if (UncompressedSize < UncompressedBuffer.size())
      UncompressedBuffer.truncate(UncompressedSize);
    return E;
  }

  CompressionSpecRef spec() { return getCompressionSpec(Kind); }

protected:
  CompressionImpl(CompressionKind Kind, int BestSpeedLevel, int DefaultLevel,
                  int BestSizeLevel)
      : Kind(Kind), BestSpeedLevel(BestSpeedLevel), DefaultLevel(DefaultLevel),
        BestSizeLevel(BestSizeLevel) {}
};

} // End of namespace compression

} // End of namespace llvm

#endif
