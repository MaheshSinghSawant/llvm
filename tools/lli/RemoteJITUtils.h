//===-- RemoteJITUtils.h - Utilities for remote-JITing with LLI -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Utilities for remote-JITing with LLI.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLI_REMOTEJITUTILS_H
#define LLVM_TOOLS_LLI_REMOTEJITUTILS_H

#include "llvm/ExecutionEngine/Orc/RPCChannel.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include <mutex>

#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <unistd.h>
#else
#include <io.h>
#endif

/// RPC channel that reads from and writes from file descriptors.
class FDRPCChannel final : public llvm::orc::remote::RPCChannel {
public:
  FDRPCChannel(int InFD, int OutFD) : InFD(InFD), OutFD(OutFD) {}

  std::error_code readBytes(char *Dst, unsigned Size) override {
    assert(Dst && "Attempt to read into null.");
    ssize_t ReadResult = ::read(InFD, Dst, Size);
    if (ReadResult != (ssize_t)Size)
      return std::error_code(errno, std::generic_category());
    return std::error_code();
  }

  std::error_code appendBytes(const char *Src, unsigned Size) override {
    assert(Src && "Attempt to append from null.");
    ssize_t WriteResult = ::write(OutFD, Src, Size);
    if (WriteResult != (ssize_t)Size)
      std::error_code(errno, std::generic_category());
    return std::error_code();
  }

  std::error_code send() override { return std::error_code(); }

private:
  int InFD, OutFD;
};

// launch the remote process (see lli.cpp) and return a channel to it.
std::unique_ptr<FDRPCChannel> launchRemote();

namespace llvm {

// ForwardingMM - Adapter to connect MCJIT to Orc's Remote memory manager.
class ForwardingMemoryManager : public llvm::RTDyldMemoryManager {
public:
  void setMemMgr(std::unique_ptr<RuntimeDyld::MemoryManager> MemMgr) {
    this->MemMgr = std::move(MemMgr);
  }

  void setResolver(std::unique_ptr<RuntimeDyld::SymbolResolver> Resolver) {
    this->Resolver = std::move(Resolver);
  }

  uint8_t *allocateCodeSection(uintptr_t Size, unsigned Alignment,
                               unsigned SectionID,
                               StringRef SectionName) override {
    return MemMgr->allocateCodeSection(Size, Alignment, SectionID, SectionName);
  }

  uint8_t *allocateDataSection(uintptr_t Size, unsigned Alignment,
                               unsigned SectionID, StringRef SectionName,
                               bool IsReadOnly) override {
    return MemMgr->allocateDataSection(Size, Alignment, SectionID, SectionName,
                                       IsReadOnly);
  }

  void reserveAllocationSpace(uintptr_t CodeSize, uint32_t CodeAlign,
                              uintptr_t RODataSize, uint32_t RODataAlign,
                              uintptr_t RWDataSize,
                              uint32_t RWDataAlign) override {
    MemMgr->reserveAllocationSpace(CodeSize, CodeAlign, RODataSize, RODataAlign,
                                   RWDataSize, RWDataAlign);
  }

  bool needsToReserveAllocationSpace() override {
    return MemMgr->needsToReserveAllocationSpace();
  }

  void registerEHFrames(uint8_t *Addr, uint64_t LoadAddr,
                        size_t Size) override {
    MemMgr->registerEHFrames(Addr, LoadAddr, Size);
  }

  void deregisterEHFrames(uint8_t *Addr, uint64_t LoadAddr,
                          size_t Size) override {
    MemMgr->deregisterEHFrames(Addr, LoadAddr, Size);
  }

  bool finalizeMemory(std::string *ErrMsg = nullptr) override {
    return MemMgr->finalizeMemory(ErrMsg);
  }

  void notifyObjectLoaded(RuntimeDyld &RTDyld,
                          const object::ObjectFile &Obj) override {
    MemMgr->notifyObjectLoaded(RTDyld, Obj);
  }

  // Don't hide the sibling notifyObjectLoaded from RTDyldMemoryManager.
  using RTDyldMemoryManager::notifyObjectLoaded;

  RuntimeDyld::SymbolInfo findSymbol(const std::string &Name) override {
    return Resolver->findSymbol(Name);
  }

  RuntimeDyld::SymbolInfo
  findSymbolInLogicalDylib(const std::string &Name) override {
    return Resolver->findSymbolInLogicalDylib(Name);
  }

private:
  std::unique_ptr<RuntimeDyld::MemoryManager> MemMgr;
  std::unique_ptr<RuntimeDyld::SymbolResolver> Resolver;
};
}

#endif
