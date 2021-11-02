#ifndef CONSTEXPR_LOWERCASE_H__
#define CONSTEXPR_LOWERCASE_H__

#include <utility>

// Code based on https://texus.me/2015/07/28/string-to-lowercase-compile-time/
// => https://gist.github.com/texus/8d867996e7a073e1498e8c18d920086c


// Temporarily move the LowerCaseStringHelper in global namespace as MSVC 2017
// creates an internal compiler error.


//----------------------------------------------------------------------------------------
/// LowerCaseString has a static member value which is const char array consisting of the characters given by STR,
/// but converted to lowercase. For example LowerCaseString<'A', 'b', 'C'>::value is {'a', 'b', 'c'}.
//----------------------------------------------------------------------------------------
template <char... STR>
struct LowerCaseString
{
	static constexpr const char value[sizeof...(STR)] =
	{ ((STR >= 'A' && STR <= 'Z') ? STR + ('a' - 'A') : STR)... };
};

// Definition of member variable value.
//template <char... STR>
//		constexpr const char LowerCaseString <STR...>::value[sizeof...(STR)];



template			< typename			, typename				> struct LowerCaseStringHelper;
template			< typename	STRING_WRAPPER	, size_t... indexes			>
struct LowerCaseStringHelper	<		STRING_WRAPPER	, std::index_sequence< indexes... >	>
{
	using Type = LowerCaseString< STRING_WRAPPER{}.value[ indexes]... >;
};


/*----------------------------------------------------------------------------------------*/
/// CONSTEXPR_TOLOWER returns the given string literal as a C string 
/// where all characters have be converted to lower case. The construction of the converted
/// C string happens at compile time.
/// @param[in]	stringLiteral			A C string literal.
/// @return	Pointer to a const char array which contains the given stringLiteral converted to lower case.
#define CONSTEXPR_TOLOWER( stringLiteral)										\
[] {															\
	struct StringWrapper												\
	{														\
		const char* value = (stringLiteral);									\
	};														\
	return LowerCaseStringHelper< StringWrapper, std::make_index_sequence< sizeof(stringLiteral)> >::Type::value;	\
}()

#endif // CONSTEXPR_LOWERCASE_H__
