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

class FragmentRef;
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
  explicit MergeString(LDSection& pSection);

  virtual ~MergeString() {};

 public:
  const LDSection& getSection() const { return *m_pSection; }
  LDSection& getSection() { return *m_pSection; }

  const SectionData& getSectionData() const { return *m_pSectionData; }
  SectionData& getSectionData() { return *m_pSectionData; }

  /// getOutputSection - get the output section of the given fragment.
  virtual const LDSection& getOutputSection(const Fragment& pFrag) const = 0;
  virtual LDSection& getOutputSection(Fragment& pFrag) = 0;

  /// getOutputOffset - get the output offset of the given FragmentRef
  uint64_t getOutputOffset(const FragmentRef& pFragRef) const;

  /// getOutputOffset - get the output offset from the specfied input offset
  /// pInputOffset of pFrag. Here pFrag should be the first fragment when it's
  /// in the input section
  uint64_t getOutputOffset(uint64_t pInputOffset,
                           const FragmentRef& pFragRef) const;

  /// updateFragmentRef - update the pRragRef to the corresponding output
  /// fragment and offset
  void updateFragmentRef(FragmentRef& pFragRef);

  virtual void addString(llvm::StringRef pString, uint64_t pInputOffset) {
    assert(false);
  }

  /// merge - merge string from pOther to this section
  /// @param pForce - force to add all the strings in pOther into this section
  /// without checking if the string is existed. An output merge string section
  /// may contain sections from mergeable and non-mergeable sections.
  /// Non-mergeable input sections should be forced to add directly
  virtual MergeString& merge(MergeString& pOther, bool pForce) { return *this; }

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
  virtual uint64_t doGetOutputOffset(uint64_t pInputOffset,
                                     const FragmentRef& pFragRef) const = 0;

  /// doUpdateFragmentRef - update the pRragRef to the corresponding output
  /// fragment and offset
  virtual void doUpdateFragmentRef(FragmentRef& pFragRef) = 0;

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

  /// getOutputSection - get the output section of the given fragment.
  virtual const LDSection& getOutputSection(const Fragment& pFrag) const
  { return *m_pSection; }

  virtual LDSection& getOutputSection(Fragment& pFrag)
  { return *m_pSection; }

  /// clearStringPool - after merging all the strings, clear the string pool to
  /// save memory
  void clearStringPool();

  void addString(llvm::StringRef pString, uint64_t pInputOffset) {
    assert(false);
  }

  /// merge - merge string from pOther to this section
  /// @param pForce - force to add all the strings in pOther into this section
  /// without checking if the string is existed. An output merge string section
  /// may contain sections from mergeable and non-mergeable sections.
  /// Non-mergeable input sections should be forced to add directly
  virtual MergeString& merge(MergeString& pOther, bool pForce);

  bool isOutput() const { return true; }

 protected:
  /// getOutputOffset - get the output offset from the given input offset
  uint64_t doGetOutputOffset(uint64_t pInputOffset,
                             const FragmentRef& pFragRef) const;

  /// doUpdateFragmentRef - update the pRragRef to the corresponding output
  /// fragment and offset
  virtual void doUpdateFragmentRef(FragmentRef& pFragRef);

 private:
  struct EntryCompare {
    bool operator() (const Entry* pX, const Entry* pY) const
    { return (pX->getRegion().compare(pY->getRegion()) < 0); }
  };

  typedef std::set<Entry*, EntryCompare> StringPoolTy;
  typedef std::map<const Fragment*, const MergeString*> FragSectionMapTy;

 private:
  // m_StringPool - unique string pool to store all strings
  StringPoolTy m_StringPool;

  // m_FragSectMap - map the first fragment of an input section to its intput
  // MergeString. In case that the fragment has been moved to output and we're
  // not able to get its output offset according to the input offset
  FragSectionMapTy m_FragSectMap;
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

  virtual const LDSection& getOutputSection(const Fragment& pFrag) const;
  virtual LDSection& getOutputSection(Fragment& pFrag);

  // addString - create an Entry for pString and add it to this section
  void addString(llvm::StringRef pString, uint64_t pInputOffset);

  bool isOutput() const { return false; }

 protected:
  uint64_t doGetOutputOffset(uint64_t pInputOffset,
                             const FragmentRef& pFragRef) const;

  /// doUpdateFragmentRef - update the pRragRef to the corresponding output
  /// fragment and offset
  virtual void doUpdateFragmentRef(FragmentRef& pFragRef);

 private:
  typedef std::map<uint64_t, Entry*> OffsetMapTy;
  typedef OffsetMapTy::iterator offset_map_iterator;
  typedef OffsetMapTy::const_iterator const_offset_map_iterator;

 private:
  // m_InOffsetMap - Map the input offset and the Entry. This is used to record
  // the input offset of each string and help to apply the relocation against
  // merge string section.
  OffsetMapTy m_InOffsetMap;
};

}  // namespace mcld

#endif  // MCLD_LD_MERGESTRING_H_
