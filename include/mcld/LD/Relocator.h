//===- Relocator.h --------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_LD_RELOCATOR_H_
#define MCLD_LD_RELOCATOR_H_

#include "mcld/Fragment/Relocation.h"

namespace mcld {

class Input;
class IRBuilder;
class MergeString;
class Module;
class TargetLDBackend;

/** \class Relocator
 *  \brief Relocator provides the interface of performing relocations
 */
class Relocator {
 public:
  typedef Relocation::Type Type;
  typedef Relocation::Address Address;
  typedef Relocation::DWord DWord;
  typedef Relocation::SWord SWord;
  typedef Relocation::Size Size;

 public:
  enum Result { OK, BadReloc, Overflow, Unsupported, Unknown };

 public:
  explicit Relocator(const LinkerConfig& pConfig) : m_Config(pConfig) {}

  virtual ~Relocator() = 0;

  /// applyRelocation - general apply function
  Result applyRelocation(Relocation& pRelocation);

  /// scanRelocation - When read in relocations, backend can do any modification
  /// to relocation and generate empty entries, such as GOT, dynamic relocation
  /// entries and other target dependent entries. These entries are generated
  /// for layout to adjust the ouput offset.
  /// @param pReloc - a read in relocation entry
  /// @param pInputSym - the input LDSymbol of relocation target symbol
  /// @param pSection - the section of relocation applying target
  /// @param pInput - the input file of relocation
  virtual void scanRelocation(Relocation& pReloc,
                              IRBuilder& pBuilder,
                              Module& pModule,
                              LDSection& pSection,
                              Input& pInput) = 0;

  /// issueUndefRefError - Provides a basic version for undefined reference
  /// dump.
  /// It will handle the filename and function name automatically.
  /// @param pReloc - a read in relocation entry
  /// @param pSection - the section of relocation applying target
  /// @ param pInput - the input file of relocation
  virtual void issueUndefRef(Relocation& pReloc,
                             LDSection& pSection,
                             Input& pInput);

  /// initializeScan - do initialization before scan relocations in pInput
  /// @return - return true for initialization success
  virtual bool initializeScan(Input& pInput) { return true; }

  /// finalizeScan - do finalization after scan relocations in pInput
  /// @return - return true for finalization success
  virtual bool finalizeScan(Input& pInput) { return true; }

  /// initializeApply - do initialization before apply relocations in pInput
  /// @return - return true for initialization success
  virtual bool initializeApply(Input& pInput) { return true; }

  /// finalizeApply - do finalization after apply relocations in pInput
  /// @return - return true for finalization success
  virtual bool finalizeApply(Input& pInput) { return true; }

  /// partialScanRelocation - When doing partial linking, backend can do any
  /// modification to relocation to fix the relocation offset after section
  /// merge
  /// @param pReloc - a read in relocation entry
  /// @param pInputSym - the input LDSymbol of relocation target symbol
  /// @param pSection - the section of relocation applying target
  virtual void partialScanRelocation(Relocation& pReloc,
                                     Module& pModule);

  // ------ observers -----//
  virtual TargetLDBackend& getTarget() = 0;

  virtual const TargetLDBackend& getTarget() const = 0;

  /// getName - get the name of a relocation
  virtual const char* getName(Type pType) const = 0;

  /// getSize - get the size of a relocation in bit
  virtual Size getSize(Type pType) const = 0;

  /// mayHaveFunctionPointerAccess - check if the given reloc would possibly
  /// access a function pointer.
  /// Note: Each target relocator should override this function, or be
  /// conservative and return true to avoid getting folded.
  virtual bool mayHaveFunctionPointerAccess(const Relocation& pReloc) const {
    return true;
  }

  /// getMergeStringOffset - get the offset from the relocation target. This is
  /// used to get the relocation target symbol offset.
  virtual uint32_t getMergeStringOffset(Relocation& pReloc) const = 0;

  /// applyMergeStringOffset - apply the relocation target to specific offset.
  /// This is used to set the relocation against merge string sections.
  virtual void applyMergeStringOffset(Relocation& pReloc, uint32_t pOffset) = 0;

 protected:
  const LinkerConfig& config() const { return m_Config; }

  /// doApplyRelocation - applyRelocation hook
  virtual Result doApplyRelocation(Relocation& pRelocation) = 0;

  /// applyRelocationForMergeString - apply the relocation against the symbol
  /// define in merge string sections
  virtual Result applyRelocationForMergeString(Relocation& pRelocation,
                                               MergeString& pTargetSection);

 private:
  const LinkerConfig& m_Config;
};

}  // namespace mcld

#endif  // MCLD_LD_RELOCATOR_H_
