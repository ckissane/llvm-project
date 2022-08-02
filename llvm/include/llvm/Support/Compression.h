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

class CompressionAlgorithm;

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

  bool operator==(llvm::compression::CompressionKind other) const;
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
// This is the base class of all compression algorithms that llvm support
// handles.
class CompressionAlgorithm {
public:
  virtual CompressionKind getAlgorithmId() = 0;

  virtual StringRef getName() = 0;

  virtual bool supported() = 0;

  virtual int getBestSpeedLevel() = 0;
  virtual int getDefaultLevel() = 0;
  virtual int getBestSizeLevel() = 0;

  virtual void compress(ArrayRef<uint8_t> Input,
                        SmallVectorImpl<uint8_t> &CompressedBuffer,
                        int Level) = 0;
  virtual void compress(ArrayRef<uint8_t> Input,
                        SmallVectorImpl<uint8_t> &CompressedBuffer) = 0;

  virtual Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                           size_t &UncompressedSize) = 0;
  virtual Error decompress(ArrayRef<uint8_t> Input,
                           SmallVectorImpl<uint8_t> &UncompressedBuffer,
                           size_t UncompressedSize) = 0;
};
class UnknownCompressionAlgorithm;
class ZStdCompressionAlgorithm;
class ZlibCompressionAlgorithm;

template <class CompressionAlgorithmType>
class CompressionAlgorithmImpl : public CompressionAlgorithm {
public:
  virtual CompressionKind getAlgorithmId() {
    return CompressionAlgorithmType::AlgorithmId;
  }

  virtual StringRef getName() { return CompressionAlgorithmType::Name; }

  virtual bool supported() { return CompressionAlgorithmType::Supported(); }

  virtual int getBestSpeedLevel() {
    return CompressionAlgorithmType::BestSpeedCompression;
  }
  virtual int getDefaultLevel() {
    return CompressionAlgorithmType::DefaultCompression;
  }
  virtual int getBestSizeLevel() {
    return CompressionAlgorithmType::BestSizeCompression;
  }

  virtual void compress(ArrayRef<uint8_t> Input,
                        SmallVectorImpl<uint8_t> &CompressedBuffer, int Level) {

    return CompressionAlgorithmType::Compress(Input, CompressedBuffer, Level);
  }
  virtual void compress(ArrayRef<uint8_t> Input,
                        SmallVectorImpl<uint8_t> &CompressedBuffer) {
    return CompressionAlgorithmType::Compress(
        Input, CompressedBuffer, CompressionAlgorithmType::DefaultCompression);
  }

  virtual Error decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                           size_t &UncompressedSize) {
    return CompressionAlgorithmType::Decompress(Input, UncompressedBuffer,
                                                UncompressedSize);
  }
  virtual Error decompress(ArrayRef<uint8_t> Input,
                           SmallVectorImpl<uint8_t> &UncompressedBuffer,
                           size_t UncompressedSize) {
    UncompressedBuffer.resize_for_overwrite(UncompressedSize);
    Error E = CompressionAlgorithmType::Decompress(
        Input, UncompressedBuffer.data(), UncompressedSize);
    if (UncompressedSize < UncompressedBuffer.size())
      UncompressedBuffer.truncate(UncompressedSize);
    return E;
  }
};

class ZStdCompressionAlgorithm
    : public CompressionAlgorithmImpl<ZStdCompressionAlgorithm> {
public:
  constexpr static CompressionKind AlgorithmId = CompressionKind::ZStd;
  constexpr static StringRef Name = "zstd";
  constexpr static int BestSpeedCompression = 1;
  constexpr static int DefaultCompression = 5;
  constexpr static int BestSizeCompression = 12;
  static void Compress(ArrayRef<uint8_t> Input,
                       SmallVectorImpl<uint8_t> &CompressedBuffer, int Level);
  static Error Decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                          size_t &UncompressedSize);
  static bool Supported();

protected:
  friend CompressionAlgorithm *CompressionKind::operator->() const;
  constexpr ZStdCompressionAlgorithm(){};
};

class ZlibCompressionAlgorithm
    : public CompressionAlgorithmImpl<ZlibCompressionAlgorithm> {
public:
  constexpr static CompressionKind AlgorithmId = CompressionKind::Zlib;
  constexpr static StringRef Name = "zlib";
  constexpr static int BestSpeedCompression = 1;
  constexpr static int DefaultCompression = 6;
  constexpr static int BestSizeCompression = 9;
  static void Compress(ArrayRef<uint8_t> Input,
                       SmallVectorImpl<uint8_t> &CompressedBuffer, int Level);
  static Error Decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                          size_t &UncompressedSize);
  static bool Supported();

protected:
  friend CompressionAlgorithm *CompressionKind::operator->() const;
  constexpr ZlibCompressionAlgorithm(){};
};

class UnknownCompressionAlgorithm
    : public CompressionAlgorithmImpl<UnknownCompressionAlgorithm> {
public:
  constexpr static CompressionKind AlgorithmId = CompressionKind::Unknown;
  constexpr static StringRef Name = "unknown";
  constexpr static int BestSpeedCompression = -999;
  constexpr static int DefaultCompression = -999;
  constexpr static int BestSizeCompression = -999;
  static void Compress(ArrayRef<uint8_t> Input,
                       SmallVectorImpl<uint8_t> &CompressedBuffer, int Level);
  static Error Decompress(ArrayRef<uint8_t> Input, uint8_t *UncompressedBuffer,
                          size_t &UncompressedSize);
  static bool Supported();

protected:
  friend CompressionAlgorithm *CompressionKind::operator->() const;
  constexpr UnknownCompressionAlgorithm(){};
};

} // End of namespace compression

} // End of namespace llvm

#endif
