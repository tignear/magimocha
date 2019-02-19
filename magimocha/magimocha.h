#pragma once
#include <array>
#include <string>
#include <stack>
#include <cppcp.h>
namespace tig::magimocha {
	static_assert(-std::numeric_limits<int>::max()!=std::numeric_limits<int>::min(),"Not the two's complement representation.");
	class magimocha_parser_exception :public cppcp::parser_exception {

	};
	class unexpected_token_exception :public magimocha_parser_exception {

	};
	namespace rawast{
		using cppcp::either;
		using cppcp::left;
		using cppcp::right;



		namespace impl {

			template<class P>
			constexpr auto unicodeBlock(P p, typename cppcp::result_type_t<P> s, typename cppcp::result_type_t<P> e) {
				return cppcp::map(p, 
					[s,e](auto c_) {
						if (s<=c_&&c_<=e) {
							return typename cppcp::result_type_t<P>(c_);
						}
						throw unexpected_token_exception();
					}
				);
			}
			template<class P>
			constexpr auto unicodeChar(P p,typename cppcp::result_type_t<P> c) {
				return cppcp::map(p, 
					[c](auto c_) {
						if (c_ == c) {
							return typename cppcp::result_type_t<P>(c);
						}
						throw unexpected_token_exception();
					}
				);
			}
			template<class Src>
			struct any:public cppcp::parser<Src,  typename Src::value_type,any<Src>> {
				constexpr any() {}
				constexpr auto parse(Src&& src)const{
					if (src.isEnd()) {
						throw cppcp::eof_exception();
					}
					else {
						return cppcp::ret<Src, typename Src::value_type>(std::next(src,1), *src);
					}
				}
			};
		


			template<class V>
			static constexpr auto bs2ull(V v,size_t* idx=nullptr,int base=10) {
				std::string s;
				s.reserve(v.length());
				for (auto x : v)
				{
					if (
						static_cast<std::common_type_t<typename V::value_type,char>>(std::numeric_limits<char>::min()) <= static_cast<std::common_type_t<typename V::value_type, char>>(x) &&
						static_cast<std::common_type_t<typename V::value_type, char>>(x) <= static_cast<std::common_type_t<typename V::value_type, char>>(std::numeric_limits<char>::max())) {
						throw std::invalid_argument("");
					}
					s+=static_cast<char>(x);
				}
				return std::stoull(s, idx, base);
			}
			template<class V>
			static constexpr auto bs2d(V v, size_t* idx = nullptr) {
				std::string s;
				s.reserve(v.length());
				for (auto x : v)
				{
					if (
						static_cast<std::common_type_t<typename V::value_type, char>>(std::numeric_limits<char>::min()) <= static_cast<std::common_type_t<typename V::value_type, char>>(x) &&
						static_cast<std::common_type_t<typename V::value_type, char>>(x) <= static_cast<std::common_type_t<typename V::value_type, char>>(std::numeric_limits<char>::max())) {
						throw std::invalid_argument("");
					}
					s += static_cast<char>(x);
				}
				return std::stod(s,idx);
			}
		}
		template<class char_type>
		struct ast_ns {
			using string_type = std::basic_string<char_type>;
			constexpr ast_ns() {

			}
			struct ast_base {
				virtual ~ast_base(){}
			};
			struct expression :public ast_base {

			};
			class declaration_name :public ast_base {
				string_type name_;
			public:
				constexpr declaration_name(const string_type& name) :name_(name) {

				}

				constexpr const string_type& name()const {
					return name_;
				}

			};
			class declaration_parameter :public ast_base {
				std::optional<string_type> name_;
			public:
				constexpr declaration_parameter(const string_type& name) :name_(name) {

				}
				constexpr declaration_parameter() : name_(std::nullopt) {

				}
				constexpr declaration_parameter(std::optional<string_type> s) : name_(s) {

				}
				constexpr const std::optional<string_type> name()const {
					return name_;
				}
				constexpr bool is_ignore_parameter() {
					return !name_;
				}
			};
			
			class declaration_function :public expression {
				std::vector<std::shared_ptr<declaration_parameter>> params_;
				std::shared_ptr<expression> body_;
			public:
				constexpr declaration_function(
					std::vector<std::shared_ptr<declaration_parameter>> params,
					std::shared_ptr<expression> body
				):params_(params),body_(body) {

				}
				const std::vector<std::shared_ptr<declaration_parameter>>& params() {
					return params_;
				}
				const std::shared_ptr<expression>& body() {
					return body_;
				}
			};
			struct literal_ :public expression {

			};
			struct numeric_literal :public literal_ {
			};
			struct integer_literal :public numeric_literal {

			};

			class signed_number_literal :public integer_literal {
				std::int64_t value_;
			public:
				constexpr signed_number_literal(const  std::int64_t value) :value_(value) {

				}
				constexpr const std::int64_t value()const {
					return value_;
				}
			};
			class unsigned_number_literal :public integer_literal {
				std::uint64_t value_;
			public:
				constexpr unsigned_number_literal(const  std::uint64_t value) :value_(value) {

				}
				constexpr const std::uint64_t value()const {
					return value_;
				}
				constexpr const std::int64_t signed_p()const {
					if (static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) >= value_) {
						return value_;
					}
					throw std::invalid_argument("");

				}
				constexpr const std::int64_t signed_n()const {
					if (static_cast<uint64_t>(std::numeric_limits<int64_t>::max())+1>= value_) {
						return -static_cast<int64_t>(value_);
					}
					throw std::invalid_argument("");
				}
			};
			class floating_literal :public numeric_literal {
				double value_;
			public:
				constexpr floating_literal(double value) :value_(value) {

				}
				constexpr const double value()const {
					return value_;
				}
				
			};
			class string_literal :public literal_ {
				std::u32string value_;
			public:
				constexpr string_literal(const std::u32string& value) :value_(value) {

				}
				constexpr const std::u32string& value()const {
					return value_;
				}
			};
			class apply_function:public expression {
				std::shared_ptr<expression> target_;
				std::vector< std::shared_ptr<expression>> args_;
			public:
				constexpr apply_function(std::shared_ptr<expression> target, std::vector<std::shared_ptr<expression>> args):target_(target),args_(args) {

				}
				constexpr const auto& target() {
					return target_;
				}
				constexpr const auto& args() {
					return args_;
				}
			};
			struct call_name:public expression {
				std::u32string value_;
				constexpr call_name(const std::u32string& value) :value_(value) {

				}
				constexpr const std::u32string& value()const {
					return value_;
				}
			};
			/*
			*
			*/

		};
		template<class Itr>
		struct p {

			using char_type = typename Itr::value_type;
			using string_type = std::basic_string<char_type>;
			using ast = ast_ns<char_type>;



			static constexpr impl::any<Itr> anyc() {
				static const auto c = impl::any<Itr>();
				return c;
			}

			static constexpr auto any() {
				static const auto c = cppcp::map(anyc(), [](auto e) {
					return std::basic_string<typename cppcp::result_type_t<decltype(anyc())>>{e};
				});
				return c;

			}
			static constexpr auto nullc() {
				static const auto c = impl::unicodeChar(anyc(), 0x00);
				return c;
			}
			static constexpr auto CR() {
				static const auto c = impl::unicodeChar(anyc(), 0x0D);
				return c;
			}
			static constexpr auto LF() {
				static const auto c = impl::unicodeChar(anyc(), 0x0A);
				return c;
			}
			static constexpr auto slash() {
				static const auto c = impl::unicodeChar(anyc(), U'/');
				return c;
			}
			static constexpr auto backslash() {
				static const auto c = impl::unicodeChar(anyc(), U'\\');
				return c;
			}
			static constexpr auto double_quote() {
				static const auto c = impl::unicodeChar(anyc(), U'\"');
				return c;
			}
			static constexpr auto sharp() {
				return impl::unicodeChar(anyc(), U'#');
			}
			static constexpr auto plus() {
				return impl::unicodeChar(anyc(), U'+');
			}
			static constexpr auto minus() {
				return impl::unicodeChar(anyc(), U'-');
			}

			static constexpr auto exclamation() {
				return impl::unicodeChar(anyc(), U'!');
			}
			static constexpr auto percent() {
				return impl::unicodeChar(anyc(), U'%');
			}
			static constexpr auto angle_open() {
				return impl::unicodeChar(anyc(), U'<');
			}
			static constexpr auto angle_close() {
				return impl::unicodeChar(anyc(), U'>');
			}
			static constexpr auto asterisk() {
				return impl::unicodeChar(anyc(), U'*');
			}
			static constexpr auto underbar() {
				return impl::unicodeChar(anyc(), U'_');
			}
			static constexpr auto and () {
				return impl::unicodeChar(anyc(), U'&');
			}
			static constexpr auto or () {
				return impl::unicodeChar(anyc(), U'|');
			}
			static constexpr auto caret() {
				return impl::unicodeChar(anyc(), U'^');
			}
			static constexpr auto tilde() {
				return impl::unicodeChar(anyc(), U'~');
			}
			static constexpr auto question() {
				return impl::unicodeChar(anyc(), U'?');
			}
			static constexpr auto parenthesis_open() {
				return impl::unicodeChar(anyc(), U'(');
			}
			static constexpr auto parenthesis_close() {
				return impl::unicodeChar(anyc(), U')');
			}
			static constexpr auto comma() {
				return impl::unicodeChar(anyc(), U',');
			}
			static constexpr auto equal() {
				return impl::unicodeChar(anyc(), U'=');
			}
			static constexpr auto dot() {
				return impl::unicodeChar(anyc(), U'.');
			}
			static constexpr auto  braces_open() {
				return impl::unicodeChar(anyc(), U'{');
			}
			static constexpr auto  braces_close() {
				return impl::unicodeChar(anyc(), U'}');
			}
			template<class P>
			static constexpr auto c2s(P p) {
				return cppcp::map(p, [](const auto& v) {
					return std::basic_string<cppcp::result_type_t<P>>({ v });
				});
			}

			template<class P>
			static constexpr auto s2ull(P p, int base = 10) {
				return cppcp::map(p, [](const string_type& v) {
					return impl::bs2ull(v);
				});
			}
			static constexpr auto CRLF() {
				return cppcp::map(
					cppcp::join(c2s(CR()), c2s(LF())), [](const auto& e)->string_type {
					return string_type{ 0x0D ,0x0A };
				}
				);
			}
			static constexpr auto line_break() {
				return cppcp::make_catching<cppcp::all_of_parser_failed_exception>(
					cppcp::trys(
						CRLF(), c2s(CR()), c2s(LF())
					),
					[](const auto& e)->string_type {throw tig::magimocha::unexpected_token_exception(); }
				);
			}
			static constexpr auto single_line_comment() {
				using namespace std::string_literals;
				static const auto cache = cppcp::map(
					cppcp::join(
						slash(),
						slash(),
						cppcp::fold(
							cppcp::sup<Itr, string_type>(string_type()),
							cppcp::trys(
								cppcp::map(line_break(), [](const auto& e) {return cppcp::accm::terminate(e); }),
								cppcp::map(any(), [](auto&& e) {return cppcp::accm::contd(e); })
							),
							[](auto&& a, auto&& e) {
					return  std::make_pair(e.first, a + e.second);
				}
						)
					),
					[](const auto&e) {
					return string_type{ {std::get<0>(e) ,std::get<1>(e) } }+std::get<2>(e);
				}
				);
				return cache;
			}
			constexpr static cppcp::type_eraser<Itr, string_type> multi_line_comment() {
				static const auto cache = cppcp::map(
					cppcp::join(
						cppcp::map(cppcp::join(slash(), asterisk()), [](const auto&e) {
							return string_type{ {std::get<0>(e) , std::get<1>(e)} };
						}),
						cppcp::fold(
							cppcp::sup<Itr, std::u32string>(std::u32string()),
							cppcp::trys(
								cppcp::map(cppcp::lazy([]() {return multi_line_comment(); }), [](const auto&e) {
									return cppcp::accm::terminate(e);
								}),
								cppcp::map(cppcp::join(asterisk(), slash()), [](const auto& e) {
									return cppcp::accm::terminate(string_type{ {std::get<0>(e) , std::get<1>(e)} });
								}),
								cppcp::map(any(), [](const auto& e) {return cppcp::accm::contd(e); })
							),
							[](auto&& a, auto&& e) {
								return std::make_pair(e.first, a + e.second);
							}
						)
					),
					[](const auto&e) {
						return std::get<0>(e) + std::get<1>(e);
					}
				);
				return cache;
			}
			constexpr static auto whitespace_item() {
				static const constexpr auto single = impl::unicodeChar<decltype(anyc())>;
				static const auto cache = cppcp::trys(
					c2s(single(anyc(), 0x0000)),
					c2s(single(anyc(), 0x0009)),
					c2s(single(anyc(), 0x000B)),
					c2s(single(anyc(), 0x000C)),
					c2s(single(anyc(), 0x0020)),
					line_break(),
					single_line_comment(),
					multi_line_comment()
				);
				return cache;
			}
			constexpr static auto whitespace() {
				static const auto cache = cppcp::manyN(
					cppcp::sup<Itr, string_type>(string_type()),
					whitespace_item(),
					1,
					[](auto&& a, const auto& e) {
					a += e;
					return cppcp::accm::contd(a);
				}
				);
				return cache;
			}

			/*
			*integer literals
			*/

			constexpr static auto binary_digit() {
				return impl::unicodeBlock(anyc(), U'0', U'1');
			}

			constexpr static auto binary_literal_character() {
				return cppcp::trys(binary_digit(), underbar());
			}
			constexpr static auto binary_literal_characters() {
				return cppcp::manyN(
					cppcp::sup<Itr>(string_type()),
					binary_literal_character(),
					1,
					[](auto&& a, const auto& e) {
					if (e == U'_') {
						return cppcp::accm::contd(a);
					}
					a.push_back(e);
					return cppcp::accm::contd(a);
				}
				);
			}
			constexpr static auto binary_literal() {
				return cppcp::map(
					cppcp::join(
						cppcp::skip(impl::unicodeChar(anyc(), U'0')),
						cppcp::skip(impl::unicodeChar(anyc(), U'b')),
						binary_literal_characters()
					),
					[](const auto& e) {
					return std::make_shared<ast::unsigned_number_literal>(static_cast<std::uint64_t>(impl::bs2ull(std::get<0>(e), nullptr, 2)));
				}
				);
			}

			constexpr static auto octal_digit() {
				return impl::unicodeBlock(anyc(), U'0', U'7');
			}

			constexpr static auto octal_literal_character() {
				return cppcp::trys(octal_digit(), underbar());
			}
			constexpr static auto octal_literal_characters() {
				return cppcp::manyN(
					cppcp::sup<Itr>(string_type()),
					octal_literal_character(),
					1,
					[](auto&& a, const auto& e) {
					if (e == U'_') {
						return cppcp::accm::contd(a);
					}
					a.push_back(e);
					return cppcp::accm::contd(a);
				}
				);
			}
			constexpr static auto octal_literal() {
				return cppcp::map(
					cppcp::join(
						cppcp::skip(impl::unicodeChar(anyc(), U'0')),
						cppcp::skip(impl::unicodeChar(anyc(), U'o')),
						octal_literal_characters()
					),
					[](const auto& e) {
					return std::make_shared<ast::unsigned_number_literal>(
						static_cast<std::uint64_t>(impl::bs2ull(std::get<0>(e), nullptr, 8))
						);
				}
				);
			}
			constexpr static auto decimal_digit() {
				return impl::unicodeBlock(anyc(), U'0', U'9');
			}
			constexpr static auto  decimal_digits() {
				return cppcp::many(
					cppcp::sup<Itr>(string_type()),
					decimal_digit(), [](auto&& a, const auto& e) {
					if (e == U'_') {
						return cppcp::accm::contd(a);
					}
					a.push_back(e);
					return cppcp::accm::contd(a);
				}
				);
			}
			constexpr static auto decimal_literal_character() {
				return cppcp::trys(decimal_digit(), underbar());
			}
			constexpr static auto decimal_literal_characters() {
				return cppcp::manyN(
					cppcp::sup<Itr>(string_type()),
					decimal_literal_character(),
					1,
					[](auto&& a, const auto& e) {
					if (e == U'_') {
						return cppcp::accm::contd(a);
					}
					a.push_back(e);
					return cppcp::accm::contd(a);
				}
				);
			}
			constexpr static auto decimal_literal_s() {
				return decimal_literal_characters();
			}
			constexpr static auto decimal_literal() {
				return cppcp::map(
					decimal_literal_characters(),
					[](const auto& e) {
					return std::make_shared<ast::unsigned_number_literal>(static_cast<std::uint64_t>(impl::bs2ull(e, nullptr, 10)));
				}
				);
			}

			constexpr static auto hexadecimal_digit() {
				return cppcp::trys(
					impl::unicodeBlock(anyc(), U'0', U'9'),
					impl::unicodeBlock(anyc(), U'a', U'f'),
					impl::unicodeBlock(anyc(), U'A', U'F')
				);
			}

			constexpr static auto hexadecimal_literal_character() {
				return cppcp::trys(hexadecimal_digit(), underbar());
			}
			constexpr static auto hexadecimal_literal_characters() {
				return cppcp::manyN(
					cppcp::sup<Itr>(string_type()),
					hexadecimal_literal_character(),
					1,
					[](auto&& a, const auto& e) {
					if (e == U'_') {
						return cppcp::accm::contd(a);
					}
					a.push_back(e);
					return cppcp::accm::contd(a);
				}
				);
			}
			constexpr static auto hexadecimal_literal_s() {
				return cppcp::map(
					cppcp::join(
						impl::unicodeChar(anyc(), U'0'),
						impl::unicodeChar(anyc(), U'x'),
						hexadecimal_literal_characters()
					),
					[](const auto&e) {
					return string_type{ {std::get<0>(e),std::get<1>(e)} }+std::get<2>(e);
				}
				);
			}
			constexpr static auto hexadecimal_literal() {
				return cppcp::map(
					cppcp::join(
						cppcp::skip(impl::unicodeChar(anyc(), U'0')),
						cppcp::skip(impl::unicodeChar(anyc(), U'x')),
						hexadecimal_literal_characters()
					),
					[](const auto& e) {
					return std::make_shared<ast::unsigned_number_literal>(
						static_cast<std::uint64_t>(impl::bs2ull(std::get<0>(e), nullptr, 16))
						);
				}
				);
			}

			constexpr static auto integer_literal() {
				return cppcp::trys(
					binary_literal(),
					octal_literal(),
					hexadecimal_literal(),
					decimal_literal()
				);
			}
			/*
			* float literals
			*/
			constexpr static auto sign() {
				return cppcp::trys(
					plus(),
					minus()
				);
			}
			constexpr static auto floating_point_p() {
				return cppcp::trys(
					impl::unicodeChar(anyc(), U'p'),
					impl::unicodeChar(anyc(), U'P')
				);
			}
			constexpr static auto floating_point_e() {
				return cppcp::trys(
					impl::unicodeChar(anyc(), U'e'),
					impl::unicodeChar(anyc(), U'E')
				);
			}
			constexpr static auto hexadecimal_exponent() {
				return cppcp::map(
					cppcp::join(
						floating_point_p(),
						cppcp::option_or(sign(), U'+'),
						decimal_literal_s()
					),
					[](const auto& e) {
					return string_type{ {std::get<0>(e),std::get<1>(e)} }+std::get<2>(e);
				}
				);
			}
			constexpr static auto hexadecimal_fraction() {
				return cppcp::map(
					cppcp::join(
						dot(),
						hexadecimal_digit(),
						hexadecimal_literal_characters()
					),
					[](const auto& e) {
					return string_type{ {std::get<0>(e),std::get<1>(e)} }+std::get<2>(e);
				}
				);
			}
			constexpr static auto decimal_exponent() {
				return cppcp::map(
					cppcp::join(
						floating_point_e(),
						cppcp::option_or(sign(), U'+'),
						decimal_literal_s()
					),
					[](const auto& e) {
					return string_type{ {std::get<0>(e),std::get<1>(e)} }+std::get<2>(e);
				}
				);
			}
			constexpr static auto decimal_fraction() {
				return  cppcp::map(
					cppcp::join(
						dot(),
						decimal_literal_s()
					),
					[](const auto& e) {
					return string_type{ {std::get<0>(e) } }+std::get<1>(e);
				}
				);
			}
			constexpr static auto floating_point_literal() {
				return cppcp::map(cppcp::trys(
					cppcp::join(cppcp::option(c2s(minus())), hexadecimal_literal_s(), hexadecimal_fraction(), hexadecimal_exponent()),
					cppcp::join(cppcp::option(c2s(minus())), decimal_literal_s(), decimal_fraction(), cppcp::option_or(decimal_exponent(), U""))
				),
					[](const auto& e) {
					return std::make_shared<ast::floating_literal>(impl::bs2d(std::get<0>(e).value_or(U"") + std::get<1>(e) + std::get<2>(e) + std::get<3>(e)));
				}
				);
			}
			/*
			numeric literals
			*/
			constexpr static auto numeric_literal() {

				return cppcp::trys(
					cppcp::parser_cast<std::shared_ptr<ast::numeric_literal>>(floating_point_literal()),
					cppcp::parser_cast<std::shared_ptr<ast::numeric_literal>>(integer_literal()),
					cppcp::parser_cast<std::shared_ptr<ast::numeric_literal>>(cppcp::map(
						cppcp::join(cppcp::skip(minus()), integer_literal()),
						[](const auto& e) {
					return std::make_shared<ast::signed_number_literal>(std::get<0>(e)->signed_n());
				}
					))
				);
			}
			/*
			* string literals
			*/

			constexpr static auto escaped_newline() {
				return cppcp::map(
					cppcp::join(
						cppcp::many(
							cppcp::sup < Itr, std::nullopt_t >(std::nullopt),
							whitespace(),
							[](auto, auto) {
					return cppcp::accm::contd(std::nullopt);
				}
						),
						cppcp::skip(line_break())
					),
					[](const auto&) {
					return string_type();
				}
				);
			}
			constexpr static auto unicode_scalar_digits() {
				return cppcp::manyNM(
					cppcp::sup<Itr, string_type>(string_type()),
					hexadecimal_digit(),
					1,
					8,
					[](auto&& a, const auto& e) {
					a.push_back(e);
					return cppcp::accm::contd(a);
				}
				);
			}
			constexpr static auto unicode_scalar() {
				return cppcp::map(
					cppcp::join(
						cppcp::skip(impl::unicodeChar(anyc(), U'u')),
						cppcp::skip(impl::unicodeChar(anyc(), U'{')),
						cppcp::map(
							unicode_scalar_digits(),
							[](const auto& e) {
					return static_cast<char32_t>(impl::bs2ull(e));
				}
						),
						cppcp::skip(impl::unicodeChar(anyc(), U'}'))
					),
					[](const auto& e) {
					return std::get<0>(e);
				}
				);
			}
			constexpr static auto escaped_character() {
				return cppcp::map(cppcp::join(
					cppcp::skip(backslash()),
					cppcp::trys(
						nullc(),
						backslash(),
						cppcp::map(
							impl::unicodeChar(anyc(), 't'),
							[](const auto&) {
					return U'\t';
				}
						),
						cppcp::map(
							impl::unicodeChar(anyc(), 'n'),
							[](const auto&) {
					return U'\n';
				}
						),
					cppcp::map(
						impl::unicodeChar(anyc(), 'r'),
						[](const auto&) {
					return U'\r';
				}
					),
					cppcp::map(
						impl::unicodeChar(anyc(), '\"'),
						[](const auto&) {
					return U'\"';
				}
					),
					cppcp::map(
						impl::unicodeChar(anyc(), '\''),
						[](const auto&) {
					return U'\'';
				}
					),
					unicode_scalar()
					)
				),
					[](const auto& e) {
					return std::get<0>(e);
				});
			}
			constexpr static auto multiline_quoted_text_item() {
				return cppcp::trys(
					c2s(escaped_character()),
					escaped_newline(),
					cppcp::map(
						anyc(),
						[](const auto& e)->string_type {
					if (U'\\' == e || '\"' == e) {
						throw unexpected_token_exception();
					}
					return string_type{ { e} };
				}
					)
				);
			}
			constexpr static auto multiline_quoted_text() {
				return cppcp::many(
					cppcp::sup<Itr, string_type>(string_type()),
					multiline_quoted_text_item(),
					[](auto&& a, const auto& e) {
					a += e;
					return cppcp::accm::contd(a);
				}
				);
			}
			constexpr static auto quoted_text_item() {
				return cppcp::trys(
					escaped_character(),
					cppcp::map(
						anyc(),
						[](const auto& e) {
					if (U'\\' == e || '\"' == e) {
						throw unexpected_token_exception();
					}
					return e;
				}
					)
				);
			}
			constexpr static auto quoted_text() {
				return cppcp::many(
					cppcp::sup<Itr, string_type>(string_type()),
					quoted_text_item(),
					[](auto&& a, const auto& e) {
					a.push_back(e);
					return cppcp::accm::contd(a);
				}
				);
			}
			/*constexpr static auto  multiline_interpolated_text_item() {
				return cppcp::trys(
					cppcp::join(
						cppcp::skip(backslash()), cppcp::skip(parenthesis_open()), expression(), cppcp::skip(parenthesis_close())
					),
					multiline_quoted_text_item()
				);
			}
			constexpr static auto  multiline_interpolated_text() {
				return cppcp::manyN(
					multiline_interpolated_text_item(),
					1,
					cppcp::sup<Itr, string_type>(string_type{}),
					[](auto&& a, const auto& e) {
						a += e;
						return a;
					}
				);
			}*/
			constexpr static auto static_string_literal() {
				return cppcp::map(
					cppcp::trys(
						cppcp::join(
							cppcp::skip(double_quote()),
							quoted_text(),
							cppcp::skip(double_quote())
						),
						cppcp::join(
							cppcp::skip(double_quote()),
							cppcp::skip(double_quote()),
							cppcp::skip(double_quote()),
							multiline_quoted_text(),
							cppcp::skip(double_quote()),
							cppcp::skip(double_quote()),
							cppcp::skip(double_quote())
						)
					),
					[](const auto& e) {return std::get<0>(e); }
				);
			}
			constexpr static auto string_literal() {
				return cppcp::map(static_string_literal(), [](auto&& e) {
					return std::make_shared<ast::string_literal>(e);
				});
			}
			constexpr static auto literal_() {
				return cppcp::trys(
					cppcp::parser_cast<std::shared_ptr<ast::literal_>>(numeric_literal()),
					cppcp::parser_cast<std::shared_ptr<ast::literal_>>(string_literal())
				);
			}
			/*
			* identifiers
			*/
			constexpr static auto identifier_head() {
				static const constexpr auto block = impl::unicodeBlock<decltype(anyc())>;
				static const constexpr auto single = impl::unicodeChar<decltype(anyc())>;
				return cppcp::trys(
					block(anyc(), U'A', U'Z'),
					block(anyc(), U'a', U'z'),
					single(anyc(), 0x00A8),
					single(anyc(), 0x00AA),
					single(anyc(), 0x00AD),
					single(anyc(), 0x00AF),
					block(anyc(), 0x00B2, 0x00B5),
					block(anyc(), 0x00B7, 0x00BA),
					block(anyc(), 0x00BC, 0x00BE),
					block(anyc(), 0x00C0, 0x00D6),
					block(anyc(), 0x00D8, 0x00F6),
					block(anyc(), 0x00F8, 0x00FF),
					block(anyc(), 0x0100, 0x02FF),
					block(anyc(), 0x0370, 0x167F),
					block(anyc(), 0x1681, 0x180D),
					block(anyc(), 0x180F, 0x1DBF),
					block(anyc(), 0x1E00, 0x1FFF),
					block(anyc(), 0x200B, 0x200D),
					block(anyc(), 0x202A, 0x202E),
					block(anyc(), 0x203F, 0x2040),
					single(anyc(), 0x2054),
					block(anyc(), 0x2060, 0x206F),
					block(anyc(), 0x2070, 0x20CF),
					block(anyc(), 0x2100, 0x218F),
					block(anyc(), 0x2460, 0x24FF),
					block(anyc(), 0x2776, 0x2793),
					block(anyc(), 0x2C00, 0x2DFF),
					block(anyc(), 0x2E80, 0x2FFF),
					block(anyc(), 0x3004, 0x3007),
					block(anyc(), 0x3021, 0x302F),
					block(anyc(), 0x3031, 0x303F),
					block(anyc(), 0x3040, 0xD7FF),
					block(anyc(), 0xF900, 0xFD3D),
					block(anyc(), 0xFD40, 0xFDCF),
					block(anyc(), 0xFDF0, 0xFE1F),
					block(anyc(), 0xFE30, 0xFE44),
					block(anyc(), 0xFE47, 0xFFFD),
					block(anyc(), 0x10000, 0x1FFFD),
					block(anyc(), 0x20000, 0x2FFFD),
					block(anyc(), 0x30000, 0x3FFFD),
					block(anyc(), 0x40000, 0x4FFFD),
					block(anyc(), 0x50000, 0x5FFFD),
					block(anyc(), 0x60000, 0x6FFFD),
					block(anyc(), 0x70000, 0x7FFFD),
					block(anyc(), 0x80000, 0x8FFFD),
					block(anyc(), 0x90000, 0x9FFFD),
					block(anyc(), 0xA0000, 0xAFFFD),
					block(anyc(), 0xB0000, 0xBFFFD),
					block(anyc(), 0xC0000, 0xCFFFD),
					block(anyc(), 0xD0000, 0xDFFFD),
					block(anyc(), 0xE0000, 0xEFFFD)
				);
			}
			constexpr static auto identifier_character() {
				static const constexpr auto block = impl::unicodeBlock<decltype(anyc())>;
				static const constexpr auto single = impl::unicodeChar<decltype(anyc())>;
				return cppcp::trys(
					block(anyc(), U'0', U'9'),
					block(anyc(), 0x0300, 0x036F),
					block(anyc(), 0x1DC0, 0x1DFF),
					block(anyc(), 0x20D0, 0x20FF),
					block(anyc(), 0xFE20, 0xFE2F),
					identifier_head()
				);
			}
			constexpr static auto identifier_characters() {
				return cppcp::many(
					cppcp::sup<Itr, string_type>(string_type()),
					identifier_character(),
					[](auto&& a, const auto& e) {
					a.push_back(e);
					return cppcp::accm::contd(a);
				}
				);
			}
			constexpr static auto identifier() {
				return cppcp::trys(
					cppcp::map(cppcp::join(identifier_head(), identifier_characters()), [](const auto& e) {
					return string_type{ {std::get<0>(e)} }+std::get<1>(e);
				}),
					cppcp::map(cppcp::join(underbar(), identifier_character(), identifier_characters()), [](const auto& e) {
					return string_type{ {std::get<0>(e),std::get<1>(e)} }+std::get<2>(e);
				})
					);
			}
			constexpr static auto implicit_parameter_name() {
				static const constexpr auto block = impl::unicodeBlock<decltype(anyc())>;
				static const constexpr auto single = impl::unicodeChar<decltype(anyc())>;
				return cppcp::map(
					cppcp::join(
						single(anyc(), '$'),
						decimal_digits()
					),
					[](const auto& e) {
					return string_type{ {std::get<0>(e)} }+std::get<1>(e);
				}
				);
			}
			/*
			* operators
			*/
			constexpr static auto op_head() {
				static const constexpr auto block = impl::unicodeBlock<decltype(anyc())>;
				static const constexpr auto single = impl::unicodeChar<decltype(anyc())>;
				static constexpr auto cache = cppcp::trys(
					slash(),
					equal(),
					minus(),
					plus(),
					exclamation(),
					asterisk(),
					percent(),
					angle_open(),
					angle_close(),
					and (),
					or (),
					caret(),
					tilde(),
					question(),
					block(anyc(), 0x00A1, 0x00A7),
					block(anyc(), 0x00A9, 0x00AB),
					block(anyc(), 0x00AC, 0x00AE),
					block(anyc(), 0x00B0, 0x00B1),
					single(anyc(), 0x00B6),
					single(anyc(), 0x00BB),
					single(anyc(), 0x00BF),
					single(anyc(), 0x00D7),
					single(anyc(), 0x00F7),
					block(anyc(), 0x2016, 0x2017),
					block(anyc(), 0x2020, 0x2027),
					block(anyc(), 0x2030, 0x203E),
					block(anyc(), 0x2041, 0x2053),
					block(anyc(), 0x2055, 0x205E),
					block(anyc(), 0x2190, 0x23FF),
					block(anyc(), 0x2500, 0x2775),
					block(anyc(), 0x2794, 0x2BFF),
					block(anyc(), 0x2E00, 0x2E7F),
					block(anyc(), 0x3001, 0x3003),
					block(anyc(), 0x3008, 0x3020),
					single(anyc(), 0x3030)
				);
				return cache;
			}
			constexpr static auto op_character() {
				static const constexpr auto block = impl::unicodeBlock<decltype(anyc())>;
				static const constexpr auto single = impl::unicodeChar<decltype(anyc())>;
				return cppcp::trys(
					block(anyc(), 0x0300, 0x036F),
					block(anyc(), 0x1DC0, 0x1DFF),
					block(anyc(), 0x20D0, 0x20FF),
					block(anyc(), 0xFE00, 0xFE0F),
					block(anyc(), 0xFE20, 0xFE2F),
					block(anyc(), 0xE0100, 0xE01EF),
					op_head()
				);
			}
			constexpr static auto op() {
				return cppcp::map(
					cppcp::join(
						op_head(),
						cppcp::many(
							cppcp::sup<Itr, string_type>(string_type({ 0 })),
							op_character(),
							[](auto&& a, const auto& e) {
								a += e;
								return cppcp::accm::contd(a);
							}
						)
					),
					[](auto&& e) {
					auto r = std::get<1>(e);
					r[0] = std::get<0>(e);
					return std::make_shared<ast::call_name>(r);
				}
				);
			}




			/*
			 * declarations
			*/

			constexpr static auto declaration_name() {
				return  cppcp::map(
					identifier(),
					[](auto&& e) {
					return	std::make_shared<ast::declaration_name>(e);
				}
				);
			}

			constexpr static auto declaration_parameter() {
				return cppcp::trys(
					cppcp::map(
						underbar(),
						[](const auto& ig) {
					return std::make_shared<ast::declaration_parameter>();
				}
					),
					cppcp::map(
						identifier(),
						[](const auto& e) {
					return std::make_shared<ast::declaration_parameter>(e);
				}
					)
					);
			}
			constexpr static auto declaration_lambda_parameter_item() {
				return cppcp::map(cppcp::join(
					declaration_parameter(),
					cppcp::skip(comma())
				), [](const auto& e) {
					return std::get<0>(e);
				});
			}
			constexpr static auto declaration_lambda_parameter() {
				return cppcp::trys(
					cppcp::map(
						cppcp::join(
							cppcp::many(
								cppcp::sup<Itr, std::vector<std::shared_ptr<ast::declaration_parameter>>>(std::vector<std::shared_ptr<ast::declaration_parameter>>()),
								declaration_lambda_parameter_item(),
								[](auto&& a, auto&& e) {
					a.push_back(e);
					return cppcp::accm::contd(a);
				}
							),
							declaration_parameter()
					),
						[](auto&& e) {
					auto&& r = std::get<0>(e);
					r.push_back(std::get<1>(e));
					return r;
				}
					),
					cppcp::sup<Itr, std::vector<std::shared_ptr<ast::declaration_parameter>>>(std::vector<std::shared_ptr<ast::declaration_parameter>>())
					);
			}
			
			constexpr static auto declaration_lambda() {
				return cppcp::map(
					cppcp::join(
						cppcp::skip(and ()),
						cppcp::skip(parenthesis_open()),
						declaration_lambda_parameter(),
						cppcp::skip(parenthesis_close()),
						expression()
					),
					[](auto&& e) {
						return std::make_shared<ast::declaration_function>(std::get<0>(e), std::get<1>(e));
					}
				);
			}

			/*
			useable_name
			*/
			constexpr static auto call_name() {
				return cppcp::map(
					cppcp::trys(
						identifier()
					),
					[](const auto& e) {
					return std::make_shared<ast::call_name>(e);
				}
				);
			}
			struct expression_token_object
			{
				
			};
			using expression_token = std::variant<std::monostate>;
			static constexpr cppcp::type_eraser<Itr, expression_token> expression_end() {
				return cppcp::sup<Itr, expression_builder>(expression_builder{});
			}
			static constexpr cppcp::type_eraser<Itr,expression_token> expression_declaration_lambda() {
				return cppcp::join(
					declaration_lambda(),
					cppcp::trys_variant(
						expression_apply_function(),
						expression_double_op(),
						expression_end()
					)
				);
			}
			static constexpr auto expression_literal() {
				return cppcp::join(
					literal_(),
					cppcp::trys_variant(
						expression_double_op(),
						expression_end()
					)
				);
			}
			static constexpr auto expression_single_op() {
				return cppcp::join(
					op(),
					cppcp::trys_variant(
						expression_call_name(),
						expression_literal(),
						expression_declaration_lambda()
					)
				);
			}
			static constexpr auto expression_double_op() {
				return cppcp::join(
					op(),
					cppcp::trys_variant(
						expression_call_name(),
						expression_literal(),
						expression_declaration_lambda()
					)
				);
			}
			static constexpr auto expression_apply_function() {
				return cppcp::join(
					apply_function(),
					expression_double_op(),
					expression_end()
				)
			}
			static constexpr auto expression_start() {
				return cppcp::trys(
					declaration_lambda(),
					op()
				);
			}
			/*enum class op_token_type {
				one,two
			};*/
			class op_token_double {
				
				std::shared_ptr<typename ast::call_name> s_;
			public:
				constexpr op_token_double(std::shared_ptr<typename ast::call_name> s):s_(s) {

				}
				
				constexpr unsigned int priority()const {
					return 0;
				}
			};
			class op_token_single {
				std::shared_ptr<typename ast::call_name> s_;
			public:
				constexpr op_token_single(std::shared_ptr<typename ast::call_name> s) :s_(s) {

				}
			};
			enum class operator_tokenizer_sw:char {
				start, single_operator, double_operator, declation_lambda, literal_, apply_function, call_name, end
				
			};
			static constexpr auto operator_tokenizer_s_op() {

			}
			static constexpr auto operator_tokenizer() {
				using s = operator_tokenizer_sw;
				using val_type = std::variant<op_token_single,op_token_double, std::shared_ptr<ast::expression>>;
				//std::array<operator_tokenizer_sw, 4>ar=;
				return cppcp::state_machine_parser(
					cppcp::sup<Itr, std::pair<std::array<operator_tokenizer_sw, 4>, std::vector<val_type>>>(
						//start
						std::pair<std::array<operator_tokenizer_sw, 4>, std::vector<val_type>>{
							std::array<operator_tokenizer_sw, 4>{

								s::single_operator,
								s::declation_lambda,
								s::call_name,
								s::literal_

							},
							std::vector<val_type>()
						}
					),
					[](auto&& a,auto&&, auto&&e) {
						a.push_back(e);
						return cppcp::accm::contd(a);
					},
					cppcp::branch::value_with(s::declation_lambda,
						cppcp::map(
							declaration_lambda(),
							[](auto&& e) {
								return std::pair<std::array<operator_tokenizer_sw, 3>, val_type> {
									std::array<operator_tokenizer_sw, 3>{
										s::apply_function,
										s::double_operator,
										s::end
									},
									val_type(std::static_pointer_cast<ast::expression>(e))
								};
							}
						)
					),
					cppcp::branch::value_with(s::single_operator,
						cppcp::map(
							op(),
							[](auto&& e) {
								return std::pair<std::array<operator_tokenizer_sw, 3>, val_type> {
									std::array<operator_tokenizer_sw, 3>{
										s::literal_,
										s::declation_lambda,
										s::call_name
									},
									val_type(op_token_single{ e })
								};
							}
						)

					),
					cppcp::branch::value_with(s::apply_function,
						cppcp::map(
							apply_function(),
							[](auto&& e) {
								return std::pair<std::array<operator_tokenizer_sw, 3>, val_type> {
									std::array<operator_tokenizer_sw, 3>{
										s::double_operator,
										s::apply_function,
										s::end
									},
									val_type(std::static_pointer_cast<ast::expression>(e) )
								};
							}
						)
					),
					cppcp::branch::end_with(s::end)
				);
			}
			static constexpr auto expression() {
				return cppcp::map(
					cppcp::expression_right(
						literal_(),op()
					),
					[](auto&& e) {
						return processing_expression_tree(std::move(e));
					}
				);
			}
			static constexpr std::shared_ptr<typename ast::expression> processing_expression_tree(
				cppcp::node<
					std::shared_ptr<typename ast::call_name>,
					std::shared_ptr<typename ast::literal_>
				> n
			) {
				switch (n.type()) {
				case cppcp::node_type::leaf:
					return n.term();
				case cppcp::node_type::node:
					return std::make_shared<ast::apply_function>(
						n.op(), std::vector<std::shared_ptr<typename ast::expression>>{ processing_expression_tree(n.left()), processing_expression_tree(n.right()) }
					);
				default:
					throw std::invalid_argument("unexpected type value");
				}
			}
		};
	}
}