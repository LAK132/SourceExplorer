#include "lisk_impl.hpp"

lisk::expression LiskAbort(lisk::environment &, bool)
{
  std::abort();
}

lisk::expression LiskButton(
  lisk::environment &env,
  bool allow_tail_eval,
  lisk::string label,
  lisk::uneval_expr expr)
{
  if (ImGui::Button(label.c_str()))
    return lisk::eval(expr.expr, env, allow_tail_eval);
  return lisk::atom::nil{};
}

lisk::expression LiskTreeNode(
  lisk::environment &env,
  bool allow_tail_eval,
  lisk::string label,
  lisk::uneval_expr expr)
{
  if (ImGui::TreeNode(label.c_str()))
  {
    auto result = lisk::eval(expr.expr, env, allow_tail_eval);
    ImGui::TreePop();
    return result;
  }
  return lisk::atom::nil{};
}

lisk::expression LiskTextEdit(
  lisk::environment &,
  bool,
  lisk::string id,
  std::shared_ptr<lisk::string> str)
{
  return lisk::atom(lak::input_text(
    id.c_str(), str.get(), ImGuiInputTextFlags_EnterReturnsTrue));
}

lisk::expression LiskMultiTextEdit(
  lisk::environment &,
  bool,
  lisk::string id,
  std::shared_ptr<lisk::string> str)
{
  return lisk::atom(lak::input_text(
    id.c_str(),
    str.get(),
    static_cast<ImGuiInputTextFlags>(ImGuiInputTextFlags_Multiline) |
      ImGuiInputTextFlags_EnterReturnsTrue));
}

lisk::expression LiskNew(lisk::environment &, bool, lisk::symbol sym)
{
  if (sym == "uint")
    return lisk::atom(lisk::pointer(std::make_shared<lisk::uint_t>()));
  else if (sym == "sint")
    return lisk::atom(lisk::pointer(std::make_shared<lisk::sint_t>()));
  else if (sym == "string")
    return lisk::atom(lisk::pointer(std::make_shared<lisk::string>()));
  else
    return lisk::exception{"Symbol '" + sym +
                           "' is not a known type that can be new-ed"};
}

template<typename T>
lisk::expression LiskValueAtomImpl(lisk::pointer ptr)
{
  if (ptr.is_shared_ptr<T>())
    return lisk::atom(*ptr.as_shared_ptr<T>());
  else if (ptr.is_raw_ptr<T>())
    return lisk::atom(*ptr.as_raw_ptr<T>());
  else if (ptr.is_raw_const_ptr<T>())
    return lisk::atom(*ptr.as_raw_const_ptr<T>());
  else
    return lisk::exception{"Not a valid pointer type"};
}

lisk::expression LiskValue(lisk::environment &, bool, lisk::pointer ptr)
{
  if (ptr._type == std::type_index(typeid(lisk::uint_t)))
    return LiskValueAtomImpl<lisk::uint_t>(ptr);
  else if (ptr._type == std::type_index(typeid(lisk::sint_t)))
    return LiskValueAtomImpl<lisk::sint_t>(ptr);
  else if (ptr._type == std::type_index(typeid(lisk::string)))
    return LiskValueAtomImpl<lisk::string>(ptr);
  else
    return lisk::exception{"Cannot get unknown pointer type"};
}

template<typename T>
lisk::expression LiskSetImpl(lisk::pointer ptr, const T &arg)
{
  if (ptr.is_shared_ptr<T>())
  {
    *ptr.as_shared_ptr<T>() = arg;
    return lisk::atom(ptr);
  }
  else if (ptr.is_raw_ptr<T>())
  {
    *ptr.as_shared_ptr<T>() = arg;
    return lisk::atom(ptr);
  }
  else if (ptr.is_raw_const_ptr<T>())
    return lisk::exception{"Cannot modify const value"};
  else
    return lisk::exception{"Not a valid pointer type"};
}

lisk::expression LiskSet(
  lisk::environment &,
  bool,
  lisk::pointer ptr,
  lisk::expression value)
{
  if (lisk::uint_t ui; value >> ui)
    return LiskSetImpl<lisk::uint_t>(ptr, ui);
  else if (lisk::sint_t si; value >> si)
    return LiskSetImpl<lisk::sint_t>(ptr, si);
  else if (lisk::string str; value >> str)
    return LiskSetImpl<lisk::string>(ptr, str);
  else
    return lisk::exception{"Unknown type of second argument"};
}

lisk::environment DefaultEnvironment()
{
  auto result = lisk::builtin::default_env();

  result.define_functor("exit", &LiskAbort);
  result.define_functor("button", &LiskButton);
  result.define_functor("tree-node", &LiskTreeNode);
  result.define_functor("text-edit", &LiskTextEdit);
  result.define_functor("multiline-text-edit", &LiskMultiTextEdit);
  result.define_functor("new", &LiskNew);
  result.define_functor("value", &LiskValue);
  result.define_functor("set", &LiskSet);

  return result;
}
