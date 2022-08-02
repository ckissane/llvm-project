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
#include "llvm/ADT/Optional.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {
template <typename T> class SmallVectorImpl;
class Error;

namespace compression {

struct CompressionAlgorithm {
  const StringRef Name;
  const int BestSpeedLevel;
  const int DefaultLevel;
  const int BestSizeLevel;
  virtual void Compress(ArrayRef<uint8_t> Input,
                        SmallVectorImpl<uint8_t> &CompressedBuffer,
                        int Level) = 0;
  virtual Error Decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                           size_t &UncompressedSize) = 0;
  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer, int Level) {

    return Compress(Input, CompressedBuffer, Level);
  }
  void compress(ArrayRef<uint8_t> Input,
                SmallVectorImpl<uint8_t> &CompressedBuffer) {
    return Compress(Input, CompressedBuffer, this->DefaultLevel);
  }

  Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                   size_t &UncompressedSize) {
    return Decompress(Input, UncompressedBuffer, UncompressedSize);
  }
  Error decompress(ArrayRef<uint8_t> Input,
                   SmallVectorImpl<uint8_t> &UncompressedBuffer,
                   size_t UncompressedSize) {
    UncompressedBuffer.resize_for_overwrite(UncompressedSize);
    Error E = Decompress(Input, UncompressedBuffer.data(), UncompressedSize);
    if (UncompressedSize < UncompressedBuffer.size())
      UncompressedBuffer.truncate(UncompressedSize);
    return E;
  }

protected:
  CompressionAlgorithm(StringRef Name, int BestSpeedLevel, int DefaultLevel,
                       int BestSizeLevel)
      : Name(Name), BestSpeedLevel(BestSpeedLevel), DefaultLevel(DefaultLevel),
        BestSizeLevel(BestSizeLevel) {}
};

class CompressionKind {
private:
  uint8_t x;

protected:
  friend constexpr llvm::Optional<CompressionKind>
  getOptionalCompressionKind(uint8_t y);
  constexpr CompressionKind(uint8_t y) : x(y) {
    if (!(y == 1 || y == 2 || y == 255)) {
      llvm_unreachable("unknown compression id");
    }
  }

public:
  constexpr operator uint8_t() const { return x; }
  CompressionAlgorithm *operator->() const;

  constexpr operator bool() const;

  static const llvm::compression::CompressionKind Unknown, Zlib, ZStd;
};
constexpr inline const llvm::compression::CompressionKind
    llvm::compression::CompressionKind::Unknown{255}, ///< Abstract compression
    llvm::compression::CompressionKind::Zlib{1}, ///< zlib style complession
    llvm::compression::CompressionKind::ZStd{2}; ///< zstd style complession
typedef llvm::Optional<CompressionKind> OptionalCompressionKind;

constexpr CompressionKind::operator bool() const {
  switch (uint8_t(x)) {
  case uint8_t(CompressionKind::Zlib):
#if LLVM_ENABLE_ZLIB
    return true;
#else
    return false;
#endif
  case uint8_t(CompressionKind::ZStd):
#if LLVM_ENABLE_ZSTD
    return true;
#else
    return false;
#endif
  default:
    return false;
  }
}

constexpr bool operator==(CompressionKind left, CompressionKind right) {
  return uint8_t(left) == uint8_t(right);
}

constexpr OptionalCompressionKind operator&&(CompressionKind left, bool right) {
  if (right) {
    return left;
  }
  return NoneType();
}
constexpr OptionalCompressionKind operator&&(OptionalCompressionKind left,
                                             bool right) {
  if (right) {
    return left;
  }
  return NoneType();
}

constexpr OptionalCompressionKind operator||(CompressionKind left,
                                             OptionalCompressionKind right) {
  if (bool(left)) {
    return left;
  }
  return right;
}
constexpr OptionalCompressionKind operator||(OptionalCompressionKind left,
                                             OptionalCompressionKind right) {
  if (!left || (!bool(*left))) {
    return right;
  }
  return left;
}

constexpr OptionalCompressionKind getOptionalCompressionKind(uint8_t y) {
  if (y == 0) {
    return NoneType();
  }
  return CompressionKind(y);
}

} // End of namespace compression

} // End of namespace llvm

#endif
