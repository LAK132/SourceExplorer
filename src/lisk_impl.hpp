#ifndef LISK_IMPL_HPP
#define LISK_IMPL_HPP

#include "imgui_utils.hpp"

#include <lak/lisk/lisk.hpp>

lak::lisk::expression LiskAbort(lak::lisk::environment &env,
                                bool allow_tail_eval);

lak::lisk::expression LiskButton(lak::lisk::environment &env,
                                 bool allow_tail_eval,
                                 lak::lisk::string label,
                                 lak::lisk::uneval_expr expr);

lak::lisk::expression LiskTreeNode(lak::lisk::environment &env,
                                   bool allow_tail_eval,
                                   lak::lisk::string label,
                                   lak::lisk::uneval_expr expr);

lak::lisk::expression LiskTextEdit(lak::lisk::environment &env,
                                   bool allow_tail_eval,
                                   lak::lisk::string id,
                                   lak::shared_ptr<lak::lisk::string> str);

lak::lisk::expression LiskMultiTextEdit(
  lak::lisk::environment &env,
  bool allow_tail_eval,
  lak::lisk::string id,
  lak::shared_ptr<lak::lisk::string> str);

lak::lisk::expression LiskNew(lak::lisk::environment &env,
                              bool allow_tail_eval,
                              lak::lisk::symbol sym);

lak::lisk::expression LiskValue(lak::lisk::environment &env,
                                bool allow_tail_eval,
                                lak::lisk::pointer ptr);

lak::lisk::expression LiskSet(lak::lisk::environment &env,
                              bool allow_tail_eval,
                              lak::lisk::pointer ptr,
                              lak::lisk::expression value);

lak::lisk::environment DefaultEnvironment();

#endif