#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <locale.h>
#include <fcntl.h>
#include <assert.h>
//#include <iostream> 
#include "constexpr_lowercase.h"
#include "light.h"

#include <vector>
#include <map>
//template <class Val		> using Vector	= std::vector< Val>;
//template <class Key, class Value> using Map	= std::map< Key, Value>;

template <class Val		> using Array	= light::Array< Val, 128>;
template <class Key, class Value> using Map	= light::Map< Key, Value, 128>;


//----------------------------------------------------------------
#include <windows.h>
// в <windows.h> не определены, но встречаются
#define IMAGE_SCN_TYPE_REG		0x00000000  // Reserved.
#define IMAGE_SCN_TYPE_DSECT		0x00000001  // Reserved.
#define IMAGE_SCN_TYPE_NOLOAD		0x00000002  // Reserved.
#define IMAGE_SCN_TYPE_GROUP		0x00000004  // Reserved.
#define IMAGE_SCN_TYPE_COPY		0x00000010  // Reserved.
#define IMAGE_SCN_TYPE_OVER		0x00000400  // Reserved.
#define IMAGE_SCN_0x00002000		0x00002000  // Reserved.
#define IMAGE_SCN_MEM_PROTECTED		0x00004000  // Obsolete
#define IMAGE_SCN_MEM_SYSHEAP		0x00010000  // Obsolete

//----------------------------------------------------------------
#pragma region // макросы для совместимости между unix и windows

#ifdef __unix__
#	include <unistd.h>
#	include <getopt.h>
#	define O_BINARY 0
#	define DIRECTORY_SEPARATOR '/'
#else
#	include <io.h>
#	include "getopt.h"
#	define STDIN_FILENO 0
#	define STDOUT_FILENO 1
#	define DIRECTORY_SEPARATOR '\\'
#	pragma warning( disable: 4996)
#endif

#ifdef __GNUC__
#define NO_RETURN __attribute__((noreturn)) void
#elif __MINGW32__
#define NO_RETURN __attribute__((noreturn)) void
#elif __clang__
#define NO_RETURN __attribute__((noreturn)) void
#elif _MSC_VER
#define NO_RETURN __declspec(noreturn) void
#endif

#pragma endregion

//----------------------------------------------------------------
#define FATAL(x) do { perror( x); exit( EXIT_FAILURE); } while (0)
#define FATALMSG( file, errmsg) do { fprintf( stderr, "%s: " errmsg "\n", (file) ); exit( EXIT_FAILURE); } while (0)
#define SIZE(x) (sizeof (x) / sizeof( *(x)))
#define STRLEN(x) (sizeof (x) / sizeof( *(x)) - 1)
#define PS(x) (x), SIZE(x)

//----------------------------------------------------------------
NO_RETURN usage( const char* cmd )
{
	// нету в студии basename'а...
	size_t cmdlen = strlen( cmd );
	cmd += cmdlen;
	for( ; cmdlen > 0; --cmdlen )
	{
		if( *--cmd == DIRECTORY_SEPARATOR )
		{
			cmd++;
			break;
		}
	}

	fprintf( stderr,
		"Usage:  %s [-h] [file]\n"
		"\n"
		"Parameters:\n"
		"	file		Print text file\n"
		"	-h		Print %s usage\n"
		"\n"
		"example:\n"
		, cmd, cmd
		);
	exit( 1);
}

//================================================================
#pragma region // class Flags
//----------------------------------------------------------------
template< typename Val>
struct Bitset
{
	Val val;
	Val mask;

	constexpr static const Val allbits = Val(0) - 1; // 0xFFFFFFFF
	static_assert( allbits > Val( 0), "Val must be unsigned!");

	constexpr Bitset(		): val(   ), mask(   )	{}
	constexpr Bitset( Val x		): val( x ), mask( x )	{}
	constexpr Bitset( Val x, Val m	): val( x ), mask( m )	{}

	bool	full	   (			) const { return mask == allbits;	};
	bool	operator ==( const Val x	) const { return (x & mask) == val;	};
	operator bool	   (			) const	{ return mask;			};

	Bitset	operator ~ (			) const	{ Bitset t = { ~val & mask, mask }; return t;	};
	Bitset&	operator &=( const Bitset& x	)	{ val |= x.val; mask |= x.mask;	return *this;	};
	Bitset	operator & ( const Bitset& x	) const { Bitset t = *this;		return t &= x;	};
	Bitset& operator = ( const Val x	)	{ val = x; mask = x;		return *this;	};
	Bitset&	operator &=( const Val x	)	{ val |= x; mask |= x;		return *this;	};
	Bitset	operator & ( const Val x	) const { Bitset t = *this;		return t &= x;	};
};

typedef Bitset< DWORD> Bits;

//----------------------------------------------------------------
struct Bitname
{
		Bits bits;
		const char* name;
constexpr	Bitname(					): bits(	), name(	) {}
constexpr	Bitname( const Bits& bits_, const char* name_	): bits( bits_	), name( name_	) {}
constexpr	Bitname( const Bitname& r			): bits( r.bits	), name( r.name ) {}
};

//----------------------------------------------------------------
struct Flags
{
	typedef Array< Bitname> Names;
private:
	DWORD val;

	static const light::Str separator;
	static Names names;
public:
	Flags(		)		{}
	Flags( DWORD x	): val( x )	{}
	const char* c_str() const;
	operator const char* () const { return c_str();	}
	operator const DWORD () const { return val;	}
	//void print( std::ostream& output ) const;
};
//inline std::istream& operator>>( std::istream& input, Flags& s ) { s.read( input ); return input; }	
//inline std::ostream& operator<<( std::ostream& output, const Flags& s ) { s.print( output ); return output; }

//----------------------------------------------------------------
#define CONSTEXPR_TOLOWER1( stringLiteral)										\
[] {															\
	struct StringWrapper												\
	{														\
		const char* value = (stringLiteral);									\
	};														\
	constexpr auto a =												\
	LowerCaseStringHelper< StringWrapper, std::make_index_sequence< sizeof(stringLiteral)> >::Type::value;		\
	if( a[0] == 'c' && a[1] == 'n' && a[2] == 't' && a[3] == '_') return a + 4;					\
	if( a[0] == 'm' && a[1] == 'e' && a[2] == 'm' && a[3] == '_') return a + 4;					\
	return a;													\
}()

//#define IMAGE_SCN1( x)		Bitname( Bits( IMAGE_SCN_##x		), CONSTEXPR_TOLOWER(#x) )
//#define IMAGE_SCN2( x, mask)	Bitname( Bits( IMAGE_SCN_##x, mask	), CONSTEXPR_TOLOWER(#x) )
#define IMAGE_SCN1( x)		Bitname( Bits( IMAGE_SCN_##x		), #x )
#define IMAGE_SCN2( x, mask)	Bitname( Bits( IMAGE_SCN_##x, mask	), #x )

Flags::Names Flags::names =
{ IMAGE_SCN2( TYPE_REG		, Bits::allbits		) // 0x00000000  // Reserved.
, IMAGE_SCN1( TYPE_DSECT				) // 0x00000001  // Reserved.
, IMAGE_SCN1( TYPE_NOLOAD				) // 0x00000002  // Reserved.
, IMAGE_SCN1( TYPE_GROUP				) // 0x00000004  // Reserved.
, IMAGE_SCN1( TYPE_NO_PAD				) // 0x00000008  // Reserved.
, IMAGE_SCN1( TYPE_COPY					) // 0x00000010  // Reserved.
, IMAGE_SCN1( CNT_CODE					)
, IMAGE_SCN1( CNT_INITIALIZED_DATA			)
, IMAGE_SCN1( CNT_UNINITIALIZED_DATA			)
, IMAGE_SCN1( LNK_OTHER					) // 0x00000100  // Reserved.
, IMAGE_SCN1( LNK_INFO					)
, IMAGE_SCN1( TYPE_OVER					) // 0x00000400  // Reserved.
, IMAGE_SCN1( LNK_REMOVE				)
, IMAGE_SCN1( LNK_COMDAT				)
, IMAGE_SCN1( 0x00002000				) // 0x00002000  // Reserved.
, IMAGE_SCN1( MEM_PROTECTED				) // 0x00004000  // Obsolete
, IMAGE_SCN1( NO_DEFER_SPEC_EXC				)
, IMAGE_SCN1( GPREL					)
, IMAGE_SCN1( MEM_FARDATA				)
, IMAGE_SCN1( MEM_SYSHEAP				) // 0x00010000  // Obsolete
, IMAGE_SCN1( MEM_PURGEABLE				)
, IMAGE_SCN1( MEM_16BIT					)
, IMAGE_SCN1( MEM_LOCKED				)
, IMAGE_SCN1( MEM_PRELOAD				)
, IMAGE_SCN2( ALIGN_2BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_4BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_8BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_16BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_32BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_64BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_128BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_256BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_512BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_1024BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_2048BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_4096BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_8192BYTES	, IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN1( LNK_NRELOC_OVFL				)
, IMAGE_SCN1( MEM_DISCARDABLE				)
, IMAGE_SCN1( MEM_NOT_CACHED				)
, IMAGE_SCN1( MEM_NOT_PAGED				)
, IMAGE_SCN1( MEM_SHARED				)
, IMAGE_SCN1( MEM_EXECUTE				)
, IMAGE_SCN1( MEM_READ					)
, IMAGE_SCN1( MEM_WRITE					)
, IMAGE_SCN2( SCALE_INDEX	, Bits::allbits		) // 0x00000001  // Tls index is scaled
};

//----------------------------------------------------------------
const light::Str Flags::separator( PS(" & ")-1 );

//----------------------------------------------------------------
const char *Flags::c_str() const
{
	// для значения в поле val ищем подходящие имена в names и складываем их в flag
	Array< light::Str> flag;
	for( int i = names.size(); i > 0; )
	{
		--i;
		if( names[i].bits == val )
		{
			flag.push_back( names[i].name );
			if( names[i].bits.full() )
				break;
		}
	}

	assert( !flag.empty() );
	// если нашелся всего один флаг - просто отдаем его имя
	if( flag.size() == 1 )
		return flag[0];

	// если несколько - склеиваем из их имен строку, заносим ее в names и отдаем её
	// * считаем длину строки
	size_t str_len = 0;
	for( size_t i = flag.size(); i > 0; )
		str_len += flag[--i].size();
	str_len += separator.size() * (flag.size() - 1);

	// * склеиваем
	light::Str str( new char[str_len + 1], 0 );
	size_t i = flag.size() - 1;
	str += flag[i];
	while( i > 0 )
	{
		str += separator;
		str += flag[--i];
	}
	str += '\0';

	// заносим в names, names у нас навроде кеша
	names.push_back( Bitname( Bits( val, Bits::allbits), str ) );

	return str;
}

//----------------------------------------------------------------
//void Flags::print( std::ostream& output ) const
//{
//	for( int i = names.size(); i > 0; )
//	{
//		--i;
//		if( names[i].bits == val )
//		{
//			output << names[i].name << separator;
//			if( names[i].bits.full() )
//				break;
//		}
//	}
//}

#pragma endregion

//----------------------------------------------------------------
inline int is_minus( const char* s )
{
	return( '-' == s[0] && '\0' == s[1] );
}

//----------------------------------------------------------------
IMAGE_NT_HEADERS* load_NT_headers( const char* filename, char buf[], const size_t buflen)
{
	int filedesc = STDIN_FILENO;
	if( !is_minus( filename ) )
	{
		if( (filedesc = open( filename, O_RDONLY | O_BINARY )) < 0 )
			FATAL( filename );
	}

	int readed;
	if( (readed = read( filedesc, buf, buflen )) < 0 )
		FATAL( filename );

	close( filedesc );

	//if( readed < buflen )
	//	FATALMSG( filename, "Invalid size" );

	IMAGE_DOS_HEADER *image_dos_header = (IMAGE_DOS_HEADER*) buf;

	if( image_dos_header->e_magic != IMAGE_DOS_SIGNATURE )
		FATALMSG( filename, "Invalid DOS header" );

	IMAGE_NT_HEADERS* image_NT_headers = (IMAGE_NT_HEADERS*)
		( buf + image_dos_header->e_lfanew );

	if( image_NT_headers->Signature != IMAGE_NT_SIGNATURE )
		FATALMSG( filename, "Invalid Portable Executable header" );

	return image_NT_headers;
}

//----------------------------------------------------------------
class Section_name
{
	union { BYTE name[8]; uint64_t key; } u;
public:
	const BYTE*    c_str	() const { return u.name; };
	operator const uint64_t () const { return u.key; };
};

//----------------------------------------------------------------
struct Image_section_header {
	Section_name	name;
	union {
		DWORD   PhysicalAddress;
		DWORD   VirtualSize;
	} Misc;
	DWORD   VirtualAddress;
	DWORD   SizeOfRawData;
	DWORD   PointerToRawData;
	DWORD   PointerToRelocations;
	DWORD   PointerToLinenumbers;
	WORD    NumberOfRelocations;
	WORD    NumberOfLinenumbers;
	Flags   characteristics;
};

//----------------------------------------------------------------
enum Radix { octal = 0, decimal = 1, hexadecimal = 2 };
Radix radix = hexadecimal;

const char* const outfmt[][3] =
{ { "%-8.8s% 10o % 10o% 10o% 10o  %s\n"		  // sysv_tabstr
  , "%-8.8s% 10u % 10u% 10u% 10u  %s\n"		
  , "%-9.8s % 8X   % 8X  % 8X  % 8X  %s\n"
  }
, { "TOTAL              % 10o          % 10o\n"	  // sysv_totalstr
  , "TOTAL              % 10u          % 10u\n"
  , "TOTAL                % 8X            % 8X\n"
  }
, { "%11.8s"	, "%10.8s"	, "%8.8s"	} // berkeley_head
, { "% 11o"	, "% 10u"	, "% 8X"	} // berkeley_val
};

enum
{ sysv_tabstr	= 0
, sysv_totalstr	= 1
, berkeley_head = 2
, berkeley_val	= 3
};

Bits total_cond;
const
Bits ECRO = // exec_code_read_only
{ IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE | 0				     | 0
, IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_WRITE
};

//----------------------------------------------------------------
void print_sysv( int argc, const char* argv[] )
{
	char buf[4096];
	for( int i = 0; i < argc; i++ )
	{
		IMAGE_NT_HEADERS* NT_headers = load_NT_headers( argv[i], PS( buf ) );
		Image_section_header* section_header = (Image_section_header*) IMAGE_FIRST_SECTION( NT_headers );

		printf( "%s:\n"
			"Section  Virt Addr  Virt Size  Raw Addr  Raw Size  Flags\n"
			, argv[i]
			);
		const char* fmt = outfmt[ sysv_tabstr ][radix];
		size_t total_VirtualSize   = 0;
		size_t total_SizeOfRawData = 0;
		const auto* p = section_header;
		for( size_t s = 0; s < NT_headers->FileHeader.NumberOfSections; s++, p++ )
		{
			printf	( fmt // "%-9.8s % 8X   % 8X  % 8X  % 8X  %s\n"
				, p->name.c_str()
				, p->VirtualAddress
				, p->Misc.VirtualSize
				, p->PointerToRawData
				, p->SizeOfRawData
				, p->characteristics.c_str()
				);
			if( total_cond == p->characteristics )
			{
				total_VirtualSize   += p->Misc.VirtualSize;
				total_SizeOfRawData += p->SizeOfRawData;
			}
		}
		if( total_cond )
			printf( outfmt[ sysv_totalstr ][radix], total_VirtualSize, total_SizeOfRawData );
		printf( "\n" );
	}
	if( total_cond )
		printf( "TOTAL flags: %s & not( %s )\n", Flags( total_cond.val).c_str(), Flags( (~total_cond).val ).c_str() );
}

//----------------------------------------------------------------
void print_berkeley( int argc, const char* argv[] )
{
	//struct Section_data { Flags flags; DWORD size[50]; };
	struct Section_data
	{
		Flags flags;
		light::Vector< DWORD> sizearray;

	};
	Map< Section_name, Section_data > sections;

	char buf[4096];
	for( int i = 0; i < argc; i++ )
	{
		IMAGE_NT_HEADERS* NT_headers = load_NT_headers( argv[i], PS( buf ) );
		Image_section_header* section_header = (Image_section_header*) IMAGE_FIRST_SECTION( NT_headers );

		for( size_t s = 0; s < NT_headers->FileHeader.NumberOfSections; s++ )
		{
			auto &r = sections[ section_header[s].name ];
			if( !r.sizearray )
				r.sizearray.resize( argc );
			r.sizearray[i]	= section_header[s].Misc.VirtualSize;
			r.flags		= section_header[s].characteristics;
		}
	}

	// печатаем заголовок таблицы
	const char*
	fmt = outfmt[ berkeley_head ][radix];
	for( const auto& section : sections )
		printf( fmt, section.first.c_str() );

	if( total_cond )
		printf( fmt, "TOTAL" );
	printf( " filename\n"	);
	// печатаем таблицу
	fmt = outfmt[ berkeley_val ][radix];
	for( int i = 0; i < argc; i++ )
	{
		size_t total_sum = 0;
		for( const auto& section : sections )
		{
			size_t s = section.second.sizearray[i];
			if( total_cond == section.second.flags )
				total_sum += s;
			printf( fmt, s );
		}
		if( total_cond )
			printf( fmt, total_sum );
		printf( " %s\n", argv[i] );
	}
	if( total_cond )
	{
		printf( "\nTOTAL sections:" );
		for( const auto& section : sections )
		{
			if( total_cond == section.second.flags )
				printf( " %.8s", section.first.c_str() );
		}
		printf( "\nTOTAL    flags: %s & not( %s )\n", Flags( total_cond.val ).c_str(), Flags( (~total_cond).val ).c_str() );
	}
}

//----------------------------------------------------------------
int main( int argc, const char* argv[] )
{
	setlocale( LC_ALL, "" );

	enum { SysV, Berkeley, GNU } format = Berkeley;

	const char* cmd = argv[0];
	if( 1 == argc )
		usage( cmd );

	for(;;)
	{
		switch( getopt( argc, argv, "odxABGth" ) )
		{
		case -1:
			if( argc - optind <= 0 )
				exit( 0 );

			switch( format )
			{
			case SysV:	print_sysv	( argc - optind, &argv[optind] ); break;
			case Berkeley:	print_berkeley	( argc - optind, &argv[optind] ); break;
			case GNU:	print_berkeley	( argc - optind, &argv[optind] ); break;
			}
			exit( 0 );
						break;
		case 'o': radix = octal;	break;
		case 'd': radix = decimal;	break;
		case 'x': radix = hexadecimal;	break;
		case 'A': format = SysV;	break;
		case 'B': format = Berkeley;	break;
		case 'G': format = GNU;		break;
		case 't': total_cond = ECRO;	break;
		case 'h': usage( cmd );		break;
		default	: usage( cmd );		break;
		}
	}
}
