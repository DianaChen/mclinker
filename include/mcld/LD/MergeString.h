//===- MergeString.h ------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_LD_MERGESTRING_H_
#define MCLD_LD_MERGESTRING_H_

#include "mcld/Config/Config.h"
#include "mcld/Fragment/RegionFragment.h"
#include "mcld/Support/Allocators.h"

#include <llvm/ADT/StringRef.h>

#include <set>
#include <map>

namespace mcld {

class LDSection;
class SectionData;

/** \class MergeString
 *  \brief MergeString is the base class to represent the mergeable string
 *  sections those have MS section flag.
 */
class MergeString {
 protected:
  MergeString()
      : m_pSection(NULL), m_pSectionData(NULL) {}
  MergeString(LDSection& pSection);

  virtual ~MergeString() {};

 public:
  const LDSection& getSection() const { return *m_pSection; }
  LDSection& getSection() { return *m_pSection; }

  const SectionData& getSectionData() const { return *m_pSectionData; }
  SectionData& getSectionData() { return *m_pSectionData; }

  /// getOutputOffset - get the output offset of the given RegionFragment
  uint64_t getOutputOffset(const Fragment& pFrag) const;

  /// getOutputFragment - get the output fragment of the given merge string
  /// fragment
  Fragment& getOutputFragment(Fragment& pFrag);

  const Fragment& getOutputFragment(const Fragment& pFrag) const;

  /// getOutputOffset - get the output offset from the specfied input offset
  /// pInputOffset
  virtual uint64_t getOutputOffset(uint64_t pInputOffset) const {
    assert(false);
    return 0;
  }

  virtual void addString(llvm::StringRef pString, uint64_t pInputOffset) {
    assert(false);
  }

  /// merge - merge string from pOther to this section
  virtual MergeString& merge(MergeString& pOther) { return *this; }

  virtual bool isOutput() const = 0;

 protected:
  /** \class Entry
   *  \brief Entry represents a string in the merge string section.
   */
  class Entry : public RegionFragment {
   public:
    Entry(llvm::StringRef pRegion, SectionData* pSD)
        : RegionFragment(pRegion, pSD), m_pOutEntry(NULL) {}

    /// getOutputEntry - after string merge, get the output one which this
    /// entry has been merged to. If this entry itself is the output entry,
    /// than return itself.
    Entry& getOutputEntry()
    { assert(m_pOutEntry != NULL); return *m_pOutEntry; }

    const Entry& getOutputEntry() const
    { assert(m_pOutEntry != NULL); return *m_pOutEntry; }

    // setOutputEntry - when this entry is merged, set the target output entry
    void setOutputEntry(Entry& pEntry)
    { assert(m_pOutEntry == NULL); m_pOutEntry = &pEntry; }

   private:
    Entry* m_pOutEntry;
  };

 protected:
  LDSection* m_pSection;
  SectionData* m_pSectionData;

 private:
  DISALLOW_COPY_AND_ASSIGN(MergeString);
};

/** \class MergeStrigOutput
 *  \brief MergeStringOutput represents the output merge string section
 */
class MergeStringOutput : public MergeString {
 private:
  friend class Chunk<MergeStringOutput, 1>;

  MergeStringOutput() {}
  explicit MergeStringOutput(LDSection& pSection)
      : MergeString(pSection) {}

  ~MergeStringOutput() {};

 public:
  static MergeStringOutput* Create(LDSection& pSection);
  static void Destroy(MergeStringOutput*& pMS);
  static void Clear();

  /// clearStringPool - after merging all the strings, clear the string pool to
  /// save memory
  void clearStringPool();

  /// getOutputOffset - get the output offset from the given input offset. The
  /// given input offset should associate to the input merge string section.
  /// This function should not be called for an output merge string section.
  uint64_t getOutputOffset(uint64_t pInputOffset) const {
      assert(false);
      return 0;
  }

  void addString(llvm::StringRef pString, uint64_t pInputOffset) {
    assert(false);
  }

  /// merge - merge string from pOther to this section
  virtual MergeString& merge(MergeString& pOther);

  bool isOutput() const { return true; }

 private:
  struct EntryEqual {
    bool operator() (const Entry* pX, const Entry* pY) const
    { return pX->getRegion().equals(pY->getRegion()); }
  };

  typedef std::set<Entry*, EntryEqual> StringPoolTy;

 private:
  // string_pool - unique string pool to store all strings
  StringPoolTy m_StringPool;
};

/** \class MergeStrigInput
 *  \brief MergeStringInput represents the input merge string section
 */
class MergeStringInput : public MergeString {
 private:
  friend class Chunk<MergeStringInput, MCLD_SECTIONS_PER_INPUT>;

  MergeStringInput() {}
  explicit MergeStringInput(LDSection& pSection)
    : MergeString(pSection) {}

  ~MergeStringInput() {}

 public:
  static MergeStringInput* Create(LDSection& pSection);
  static void Destroy(MergeStringInput*& pMS);
  static void Clear();

  // addString - create an Entry for pString and add it to this section
  void addString(llvm::StringRef pString, uint64_t pInputOffset);

  /// getOutputOffset - get the output offset frome the given input offset. The
  /// given input offset should associate to this input merge string section.
  uint64_t getOutputOffset(uint64_t pInputOffset) const;

  bool isOutput() const { return false; }

 private:
  typedef std::map<uint64_t, Entry*> OffsetMapTy;

 private:
  // m_InOffsetMap - Map the input offset and the Entry. This is used to record
  // the input offset of each string and help to apply the relocation against
  // merge string section.
  OffsetMapTy m_InOffsetMap;
};

}  // namespace mcld

#endif  // MCLD_LD_MERGESTRING_H_
