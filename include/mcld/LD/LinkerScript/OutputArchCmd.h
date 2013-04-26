//===- OutputArchCmd.h ----------------------------------------------------===//
//
//                     The MCLinker Project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef MCLD_OUTPUTARCH_COMMAND_INTERFACE_H
#define MCLD_OUTPUTARCH_COMMAND_INTERFACE_H
#ifdef ENABLE_UNITTEST
#include <gtest.h>
#endif

#include <mcld/LD/LinkerScript/ScriptCommand.h>
#include <string>

namespace mcld
{

/** \class OutputArchCmd
 *  \brief This class defines the interfaces to OutputArch command.
 */

class OutputArchCmd : public ScriptCommand
{
public:
  OutputArchCmd(const std::string& pArch);
  ~OutputArchCmd();

  void dump() const;

  static bool classof(const ScriptCommand* pCmd)
  {
    return pCmd->getKind() == ScriptCommand::OutputArch;
  }

  void activate();

private:
  std::string m_Arch;
};

} // namespace of mcld

#endif

