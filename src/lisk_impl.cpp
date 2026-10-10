#include "lisk_impl.hpp"

lak::lisk::expression LiskAbort(lak::lisk::environment &, bool)
{
	std::abort();
}

lak::lisk::expression LiskButton(lak::lisk::environment &env,
                                 bool allow_tail_eval,
                                 lak::lisk::string label,
                                 lak::lisk::uneval_expr expr)
{
	if (ImGui::Button(reinterpret_cast<const char *>(label.c_str())))
		return lak::lisk::eval(expr.expr, env, allow_tail_eval);
	return lak::lisk::atom::nil{};
}

lak::lisk::expression LiskTreeNode(lak::lisk::environment &env,
                                   bool allow_tail_eval,
                                   lak::lisk::string label,
                                   lak::lisk::uneval_expr expr)
{
	if (ImGui::TreeNode(reinterpret_cast<const char *>(label.c_str())))
	{
		auto result = lak::lisk::eval(expr.expr, env, allow_tail_eval);
		ImGui::TreePop();
		return result;
	}
	return lak::lisk::atom::nil{};
}

lak::lisk::expression LiskTextEdit(lak::lisk::environment &,
                                   bool,
                                   lak::lisk::string id,
                                   lak::shared_ptr<lak::lisk::string> str)
{
	return lak::lisk::atom(
	  lak::input_text((char *)id.c_str(),
	                  static_cast<lak::u8string *>(str.get()),
	                  ImGuiInputTextFlags_EnterReturnsTrue));
}

lak::lisk::expression LiskMultiTextEdit(lak::lisk::environment &,
                                        bool,
                                        lak::lisk::string id,
                                        lak::shared_ptr<lak::lisk::string> str)
{
	return lak::lisk::atom(lak::input_text(
	  (char *)id.c_str(),
	  static_cast<lak::u8string *>(str.get()),
	  static_cast<ImGuiInputTextFlags>(ImGuiInputTextFlags_Multiline) |
	    ImGuiInputTextFlags_EnterReturnsTrue));
}

lak::lisk::expression LiskNew(lak::lisk::environment &,
                              bool,
                              lak::lisk::symbol sym)
{
	if (sym == u8"uint"_str)
		return lak::lisk::atom(
		  lak::lisk::pointer(lak::shared_ptr<lak::lisk::uint_t>::make()));
	else if (sym == u8"sint"_str)
		return lak::lisk::atom(
		  lak::lisk::pointer(lak::shared_ptr<lak::lisk::sint_t>::make()));
	else if (sym == u8"string"_str)
		return lak::lisk::atom(
		  lak::lisk::pointer(lak::shared_ptr<lak::lisk::string>::make()));
	else
		return lak::lisk::exception{
		  u8"Symbol '"_str + sym +
		  u8"' is not a known type that can be new-ed"_str};
}

template<typename T>
lak::lisk::expression LiskValueAtomImpl(lak::lisk::pointer ptr)
{
	if (ptr.is_shared_ptr<T>())
		return lak::lisk::atom(*ptr.as_shared_ptr<T>());
	else if (ptr.is_raw_ptr<T>())
		return lak::lisk::atom(*ptr.as_raw_ptr<T>());
	else if (ptr.is_raw_const_ptr<T>())
		return lak::lisk::atom(*ptr.as_raw_const_ptr<T>());
	else
		return lak::lisk::exception{u8"Not a valid pointer type"};
}

lak::lisk::expression LiskValue(lak::lisk::environment &,
                                bool,
                                lak::lisk::pointer ptr)
{
	if (ptr._type == std::type_index(typeid(lak::lisk::uint_t)))
		return LiskValueAtomImpl<lak::lisk::uint_t>(ptr);
	else if (ptr._type == std::type_index(typeid(lak::lisk::sint_t)))
		return LiskValueAtomImpl<lak::lisk::sint_t>(ptr);
	else if (ptr._type == std::type_index(typeid(lak::lisk::string)))
		return LiskValueAtomImpl<lak::lisk::string>(ptr);
	else
		return lak::lisk::exception{u8"Cannot get unknown pointer type"};
}

template<typename T>
lak::lisk::expression LiskSetImpl(lak::lisk::pointer ptr, const T &arg)
{
	if (ptr.is_shared_ptr<T>())
	{
		*ptr.as_shared_ptr<T>() = arg;
		return lak::lisk::atom(ptr);
	}
	else if (ptr.is_raw_ptr<T>())
	{
		*ptr.as_shared_ptr<T>() = arg;
		return lak::lisk::atom(ptr);
	}
	else if (ptr.is_raw_const_ptr<T>())
		return lak::lisk::exception{u8"Cannot modify const value"};
	else
		return lak::lisk::exception{u8"Not a valid pointer type"};
}

lak::lisk::expression LiskSet(lak::lisk::environment &,
                              bool,
                              lak::lisk::pointer ptr,
                              lak::lisk::expression value)
{
	if_let_ok (auto ui, lak::lisk::from_expr<lak::lisk::uint_t>(value))
		return LiskSetImpl<lak::lisk::uint_t>(ptr, ui);
	else if_let_ok (auto si, lak::lisk::from_expr<lak::lisk::sint_t>(value))
		return LiskSetImpl<lak::lisk::sint_t>(ptr, si);
	else if_let_ok (auto str, lak::lisk::from_expr<lak::lisk::string>(value))
		return LiskSetImpl<lak::lisk::string>(ptr, str);
	else
		return lak::lisk::exception{u8"Unknown type of second argument"};
}

lak::lisk::environment DefaultEnvironment()
{
	auto result = lak::lisk::builtin::default_env();

	result.define_function("exit", LISK_FUNCTION_WRAPPER(LiskAbort));
	result.define_function("button", LISK_FUNCTION_WRAPPER(LiskButton));
	result.define_function("tree-node", LISK_FUNCTION_WRAPPER(LiskTreeNode));
	result.define_function("text-edit", LISK_FUNCTION_WRAPPER(LiskTextEdit));
	result.define_function("multiline-text-edit",
	                       LISK_FUNCTION_WRAPPER(LiskMultiTextEdit));
	result.define_function("new", LISK_FUNCTION_WRAPPER(LiskNew));
	result.define_function("value", LISK_FUNCTION_WRAPPER(LiskValue));
	result.define_function("set", LISK_FUNCTION_WRAPPER(LiskSet));

	return result;
}
