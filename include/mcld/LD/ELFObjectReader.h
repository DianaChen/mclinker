//===- ELFObjectReader.h --------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_LD_ELFOBJECTREADER_H_
#define MCLD_LD_ELFOBJECTREADER_H_

#include "mcld/ADT/Flags.h"
#include "mcld/LD/ObjectReader.h"

namespace mcld {

class EhFrameReader;
class ELFReaderIF;
class Input;
class IRBuilder;
class MergeStringReader;
class GNULDBackend;
class LinkerConfig;

/** \lclass ELFObjectReader
 *  \brief ELFObjectReader reads target-independent parts of ELF object file
 */
class ELFObjectReader : public ObjectReader {
 public:
  enum ReadFlagType {
    ParseEhFrame = 0x1,  ///< parse .eh_frame section if the bit is set.
    NumOfReadFlags = 1
  };

  typedef Flags<ReadFlagType> ReadFlag;

 public:
  ELFObjectReader(GNULDBackend& pBackend,
                  IRBuilder& pBuilder,
                  const LinkerConfig& pConfig);

  ~ELFObjectReader();

  // -----  observers  ----- //
  bool isMyFormat(Input& pFile, bool& pContinue) const;

  // -----  readers  ----- //
  bool readHeader(Input& pFile);

  virtual bool readSections(Input& pFile);

  virtual bool readSymbols(Input& pFile);

  /// readMergeStrings - read pStrings as merge strings
  ///
  /// We may need to read the merge strings during MergeSections. In this
  /// function, pStrings is read as several fragments containing only one string
  /// each. Then put these fragments into pSection.
  virtual bool readMergeStrings(llvm::StringRef pStrings,
                                LDSection& pSection);

  /// readRelocations - read relocation sections
  ///
  /// This function should be called after symbol resolution.
  virtual bool readRelocations(Input& pFile);

 private:
  ELFReaderIF* m_pELFReader;
  EhFrameReader* m_pEhFrameReader;
  MergeStringReader* m_pMergeStringReader;
  IRBuilder& m_Builder;
  ReadFlag m_ReadFlag;
  GNULDBackend& m_Backend;
  const LinkerConfig& m_Config;
};

}  // namespace mcld

#endif  // MCLD_LD_ELFOBJECTREADER_H_
