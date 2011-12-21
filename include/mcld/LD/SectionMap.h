//===- SectionMap.h -------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_SECTION_MAP_H
#define MCLD_SECTION_MAP_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

#include <llvm/Support/DataTypes.h>
#include <vector>
#include <string>

namespace mcld
{

/** \class SectionMap
 *  \brief descirbe the mappings of input section's name (or prefix) to
 *         its associated output section's name and offset
 */
class SectionMap
{
public:
  // a mapping in SectionMap is the triple of
  // {input substr, output section's name, output section's offset}
  struct Mapping {
    std::string inputSubStr;
    std::string outputStr;
    uint64_t offset;
  };

  typedef std::vector<struct Mapping> SectionMappingTy;

  typedef SectionMappingTy::iterator iterator;
  typedef SectionMappingTy::const_iterator const_iterator;

public:
  SectionMap();
  ~SectionMap();

  // add a mapping from input substr to output name and offset.
  bool push_back(const std::string& pInput,
                 const std::string& pOutput,
                 const uint64_t pOffset);

  // find - return the iterator to the mapping
  iterator find(const std::string& pInput);

  // at - return the pointer to the mapping
  Mapping* at(const std::string& pInput);

  // -----  observers  ----- //
  bool empty() const
  { return m_SectMap.empty(); }

  size_t size() const
  { return m_SectMap.size(); }

  size_t capacity () const
  { return m_SectMap.capacity(); }

  // -----  iterators  ----- //
  iterator begin()
  { return m_SectMap.begin(); }

  iterator end()
  { return m_SectMap.end(); }

  const_iterator begin() const
  { return m_SectMap.begin(); }

  const_iterator end() const
  { return m_SectMap.end(); }

  // addStdElFMap - add elf mappings to SectionMap
  void addStdELFMap();

private:
  struct SectionNameMapping {
    const char* from;
    const char* to;
  };

  // ELF mappings from gold
  static const SectionNameMapping m_StdELFMap[];

  static const int m_StdELFMapSize;

  SectionMappingTy m_SectMap;
};

} // namespace of mcld

#endif

