#include <iostream>
#include <string>
#include <locale>
namespace tig::magimocha {
	/*std::u32string to_u32string(const std::string& external)
	{
		std::locale::global(std::locale("en_US.utf8"));
		auto& f = std::use_facet<std::codecvt<char32_t, char, std::mbstate_t>>(std::locale());

		std::mbstate_t mb = std::mbstate_t();
		std::u32string internal(external.size(), '\0');
		const char* from_next;
		char32_t* to_next;
		f.in(mb, &external[0], &external[external.size()], from_next,
			&internal[0], &internal[internal.size()], to_next);
		internal.resize(to_next - &internal[0]);
		return internal;
	}*/
	std::string to_string(const std::u32string& external)
	{
		std::locale::global(std::locale("en_US.utf8"));
		auto& f = std::use_facet<std::codecvt<char, char32_t, std::mbstate_t>>(std::locale());

		std::mbstate_t mb = std::mbstate_t();
		std::string internal(external.size(), '\0');
		const char32_t* from_next;
		char* to_next;
		f.in(mb, &external[0], &external[external.size()], from_next,
			&internal[0], &internal[internal.size()], to_next);
		internal.resize(to_next - &internal[0]);
		return internal;
	}
}