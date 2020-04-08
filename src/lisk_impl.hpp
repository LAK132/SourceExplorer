#ifndef LISK_IMPL_HPP
#define LISK_IMPL_HPP

#include "imgui_utils.hpp"

#include <lisk/lisk.hpp>

lisk::expression LiskAbort(lisk::environment &env, bool allow_tail_eval);

lisk::expression LiskButton(lisk::environment &env,
                            bool allow_tail_eval,
                            lisk::string label,
                            lisk::uneval_expr expr);

lisk::expression LiskTreeNode(lisk::environment &env,
                              bool allow_tail_eval,
                              lisk::string label,
                              lisk::uneval_expr expr);

lisk::expression LiskTextEdit(lisk::environment &env,
                              bool allow_tail_eval,
                              lisk::string id,
                              std::shared_ptr<lisk::string> str);

lisk::expression LiskMultiTextEdit(lisk::environment &env,
                                   bool allow_tail_eval,
                                   lisk::string id,
                                   std::shared_ptr<lisk::string> str);

lisk::expression LiskNew(lisk::environment &env,
                         bool allow_tail_eval,
                         lisk::symbol sym);

lisk::expression LiskValue(lisk::environment &env,
                           bool allow_tail_eval,
                           lisk::pointer ptr);

lisk::expression LiskSet(lisk::environment &env,
                         bool allow_tail_eval,
                         lisk::pointer ptr,
                         lisk::expression value);

lisk::environment DefaultEnvironment();

#endif