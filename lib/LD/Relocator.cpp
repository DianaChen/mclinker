//===- Relocator.cpp ------------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "mcld/LD/Relocator.h"

#include "mcld/Module.h"
#include "mcld/Fragment/Fragment.h"
#include "mcld/LD/LDContext.h"
#include "mcld/LD/LDSection.h"
#include "mcld/LD/LDSymbol.h"
#include "mcld/LD/MergeString.h"
#include "mcld/LD/ResolveInfo.h"
#include "mcld/LD/SectionData.h"
#include "mcld/Support/Demangle.h"
#include "mcld/Support/MsgHandling.h"
#include "mcld/Target/TargetLDBackend.h"

#include <sstream>

namespace mcld {

//===----------------------------------------------------------------------===//
// Relocator
//===----------------------------------------------------------------------===//
Relocator::~Relocator() {
}

void Relocator::partialScanRelocation(Relocation& pReloc,
                                      Module& pModule) {
  ResolveInfo* sym_info = pReloc.symInfo();
  if (!sym_info->outSymbol()->hasFragRef())
    return;

  LDSection& target_sect = sym_info
                               ->outSymbol()
                               ->fragRef()
                               ->frag()
                               ->getParent()
                               ->getSection();

  if (getTarget().isMergeStringSection(target_sect)) {
    // update the relocation target offset
    assert(target_sect.hasMergeString());
    MergeString* merge_string = target_sect.getMergeString();
    uint64_t off = 0x0u;
    assert(sym_info->outSymbol()->fragRef()->offset() == 0x0u);
    if (sym_info->type() == ResolveInfo::Section) {
      // offset of the relocation against section symbol should be acquired
      // accordings to input offset
      off = merge_string->getOutputOffset(getMergeStringOffset(pReloc),
                *sym_info->outSymbol()->fragRef());
    } else {
      off = merge_string->getOutputOffset(*sym_info->outSymbol()->fragRef());
    }
    pReloc.target() = off;

    // update the relocation target symbol
    Fragment* sym_frag = sym_info->outSymbol()->fragRef()->frag();
    LDSection& output_sect = merge_string->getOutputSection(*sym_frag);
    pReloc.setSymInfo(pModule
                          .getSectionSymbolSet()
                          .get(output_sect)
                          ->resolveInfo());
  } else if (pReloc.symInfo()->type() == ResolveInfo::Section) {
    // update the relocation target offset
    pReloc.target() += pReloc
                           .symInfo()
                           ->outSymbol()
                           ->fragRef()
                           ->getOutputOffset();
    // update relocation target symbol
    ResolveInfo* sym_info =
        pModule.getSectionSymbolSet().get(target_sect)->resolveInfo();
    // set relocation target symbol to the output section symbol's resolveInfo
    pReloc.setSymInfo(sym_info);
  }
}

void Relocator::issueUndefRef(Relocation& pReloc,
                              LDSection& pSection,
                              Input& pInput) {
  FragmentRef::Offset undef_sym_pos = pReloc.targetRef().offset();
  std::string sect_name(pSection.name());
  // Drop .rel(a) prefix
  sect_name = sect_name.substr(sect_name.find('.', /*pos=*/1));

  std::string reloc_sym(pReloc.symInfo()->name());
  reloc_sym = demangleName(reloc_sym);

  std::stringstream ss;
  ss << "0x" << std::hex << undef_sym_pos;
  std::string undef_sym_pos_hex(ss.str());

  if (sect_name.substr(0, 5) != ".text") {
    // Function name is only valid for text section
    fatal(diag::undefined_reference) << reloc_sym << pInput.path() << sect_name
                                     << undef_sym_pos_hex;
    return;
  }

  std::string caller_file_name;
  std::string caller_func_name;
  for (LDContext::sym_iterator i = pInput.context()->symTabBegin(),
                               e = pInput.context()->symTabEnd();
       i != e;
       ++i) {
    LDSymbol& sym = **i;
    if (sym.resolveInfo()->type() == ResolveInfo::File)
      caller_file_name = sym.resolveInfo()->name();

    if (sym.resolveInfo()->type() == ResolveInfo::Function &&
        sym.value() <= undef_sym_pos &&
        sym.value() + sym.size() > undef_sym_pos) {
      caller_func_name = sym.name();
      break;
    }
  }

  caller_func_name = demangleName(caller_func_name);

  fatal(diag::undefined_reference_text) << reloc_sym << pInput.path()
                                        << caller_file_name << caller_func_name;
}

Relocator::Result Relocator::applyRelocation(Relocation& pRelocation) {
  // update the targetRef of the relocation if the target in MergeString
  if (getTarget().isMergeStringSection(pRelocation
                                           .targetRef()
                                           .frag()
                                           ->getParent()
                                           ->getSection())) {
    MergeString* target_ms = pRelocation
                                 .targetRef()
                                 .frag()
                                 ->getParent()
                                 ->getSection()
                                 .getMergeString();
    assert(!target_ms->isOutput());
    target_ms->updateFragmentRef(pRelocation.targetRef());
  }
  return doApplyRelocation(pRelocation);
}

}  // namespace mcld
