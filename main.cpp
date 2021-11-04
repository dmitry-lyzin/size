#define MAX_FILENAMES	50
#define MAX_FILENAMES_S	"50"
#define MAX_SECTOIN	20

//----------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <locale.h>
#include <fcntl.h>
#include <assert.h>
#include <windows.h>
#include <iostream>
#include "constexpr_lowercase.h"

//----------------------------------------------------------------
#pragma region // Array, Map

//----------------------------------------------------------------
template <class Cls, size_t aaa>
class Array
{
	Cls a[aaa];
	size_t s = 0;
public:
constexpr	size_t max_size	(		) const { return std::size(a);			};
		size_t size	(		) const { return s;				};
		bool is_full	(		) const	{ return s >= std::size(a);		};
		Cls* new_end	(		)	{ assert( !is_full()); return &a[s++];	};
		void push_back	( const Cls& x	)	{ assert( !is_full()); a[s++] = x;	};
		Cls* begin	(		)	{ return a;				};
		Cls* end	(		)	{ return &a[s];				};
		Cls& operator*	(		)	{ return a[0];				};
		Cls& operator[]	( size_t i	)	{ assert( i    && i<s ); return a[i];	};
		Cls& operator[]	( int i		)	{ assert( i>=0 && i<s ); return a[i];	};
constexpr const	Cls* begin	(		) const	{ return a;				};
constexpr const	Cls* end	(		) const	{ return &a[s];				};
constexpr const	Cls& operator*	(		) const	{ return a[0];				};
constexpr const	Cls& operator[]	( size_t i	) const { static_assert( i    && i<s, "out of range"); return a[i]; };
constexpr const	Cls& operator[]	( int i		) const { static_assert( i>=0 && i<s, "out of range"); return a[i]; };
};

//----------------------------------------------------------------
template	< class Key,	class Value	>
struct Pair	{ Key key;	Value value;	};

//----------------------------------------------------------------
template <class Key, class Value, size_t size>
struct Map: public Array< Pair< Key, Value>, size>
{
	typedef Pair< Key, Value> KeyVal;
	////template <typename Key, typename Value>
	//class Helper
	//{
	//	Map* pmap;
	//	Key m_key;
	//public:
	//	operator Value& () const
	//	{
	//		KeyVal* p = pmap->find( m_key );
	//		if( p )
	//			return p->value;
	//		p = pmap->new_end();
	//		p->key = m_key;
	//		return p->value;
	//	}
	//	Helper& operator=( const Value& newValue ) const
	//	{
	//		pmap->insert( m_key, newValue );
	//		return *this;
	//	}
	//};
	KeyVal* find( const Key& key )
	{
		KeyVal* p = begin();
		for( size_t i = size(); i > 0; ++p )
		{
			--i;
			if( key == (*p).key )
				return p;
		}
		return NULL;
	};

	Value& operator[]( const Key& key )
	{
		KeyVal* p = begin();
		for( size_t i = size(); i > 0; ++p )
		{
			--i;
			if( key == (*p).key )
				return p->value;
		}
		p = new_end();
		p->key = key;
		//p->value = false;
		return p->value;
	};
};

#pragma endregion

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
int radix = 1;
const char* outfmt[][2] =
{ { "%-8.8s% 10u % 10u% 10u% 10u  "	, "%-9.8s % 8X   % 8X  % 8X  % 8X  "	}
, { "%s:\t%u\n"				, "%s:\t%X\n"				}
};

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
class Flags
{
	DWORD val;

	constexpr static const char separator[] = " ";
	typedef struct { DWORD val; DWORD mask; const char* name; } Names;
	static size_t names_len;
	static Names  names[140];
public:
	const char* c_str() const;
	operator const char* () const { return c_str(); };
	void print( std::ostream& output ) const;
};
//inline std::istream& operator>>( std::istream& input, Flags& s ) { s.read( input ); return input; }	
inline std::ostream& operator<<( std::ostream& output, const Flags& s ) { s.print( output ); return output; }

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

#define IMAGE_SCN_TYPE_REG		0x00000000  // Reserved.
#define IMAGE_SCN_TYPE_DSECT		0x00000001  // Reserved.
#define IMAGE_SCN_TYPE_NOLOAD		0x00000002  // Reserved.
#define IMAGE_SCN_TYPE_GROUP		0x00000004  // Reserved.
#define IMAGE_SCN_TYPE_COPY		0x00000010  // Reserved.
#define IMAGE_SCN_TYPE_OVER		0x00000400  // Reserved.
#define IMAGE_SCN_0x00002000		0x00002000  // Reserved.
#define IMAGE_SCN_MEM_PROTECTED		0x00004000  // Obsolete
#define IMAGE_SCN_MEM_SYSHEAP		0x00010000  // Obsolete

#define IMAGE_SCN1( x)		{ IMAGE_SCN_##x, IMAGE_SCN_##x,	CONSTEXPR_TOLOWER1(#x) }
#define IMAGE_SCN2( x, mask)	{ IMAGE_SCN_##x, mask,		CONSTEXPR_TOLOWER1(#x) }

constexpr int BEGIN__LINE__ = __LINE__;
Flags::Names Flags::names[] =
{ IMAGE_SCN2( TYPE_REG,		0xFFFFFFFF		) // 0x00000000  // Reserved.
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
, IMAGE_SCN2( ALIGN_2BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_4BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_8BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_16BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_32BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_64BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_128BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_256BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_512BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_1024BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_2048BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_4096BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN2( ALIGN_8192BYTES,	IMAGE_SCN_ALIGN_MASK	)
, IMAGE_SCN1( LNK_NRELOC_OVFL				)
, IMAGE_SCN1( MEM_DISCARDABLE				)
, IMAGE_SCN1( MEM_NOT_CACHED				)
, IMAGE_SCN1( MEM_NOT_PAGED				)
, IMAGE_SCN1( MEM_SHARED				)
, IMAGE_SCN1( MEM_EXECUTE				)
, IMAGE_SCN1( MEM_READ					)
, IMAGE_SCN1( MEM_WRITE					)
, IMAGE_SCN2( SCALE_INDEX,	0xFFFFFFFF		) // 0x00000001  // Tls index is scaled
};
// извратный способ посчитать длину ЗАПОЛНЕНОЙ части массива names
size_t Flags::names_len = __LINE__ - BEGIN__LINE__ - 4;

//----------------------------------------------------------------
const char *Flags::c_str() const
{
	// для значения в поле val ищем подходящие имена в names и складываем их в flag
	size_t flags = 0;
	const Names *flag[64];
	for( int i = names_len; i > 0; )
	{
		--i;
		if( (val & names[i].mask) == names[i].val )
		{
			assert( SIZE( flag) > flags );
			flag[ flags++] = &names[i];
			if( names[i].mask == 0xFFFFFFFF )
				break;
		}
	}

	assert( flags );
	// если нашелся всего один флаг - просто отдаем его имя
	if( flags == 1 )
		return flag[0]->name;

	// если несколько - склеиваем из их имен строку, заносим ее в names и отдаем её
	// * считаем длину строки
	size_t str_len = 0;
	size_t strlen_flag_name[ SIZE( flag )];
	for( size_t i = flags; i > 0; )
	{
		--i;
		size_t cur_strlen_flag_name = strlen( flag[i]->name );
		strlen_flag_name[i] = cur_strlen_flag_name;
		str_len += cur_strlen_flag_name;
	}
	str_len += STRLEN( separator) * (flags - 1);

	char* str = new char[ str_len + 1];

	// * склеиваем
	char* p = str;
	--flags;
	memcpy( p, flag[flags]->name, strlen_flag_name[flags] );
	p += strlen_flag_name[flags];
	while( flags > 0 )
	{
		--flags;
		memcpy( p, separator, STRLEN( separator ) );
		p += STRLEN( separator );
		memcpy( p, flag[flags]->name, strlen_flag_name[flags] );
		p += strlen_flag_name[flags];
	}
	*p = 0;
	assert( p - str == str_len );

	// заносим в names, names у нас навроде кеша
	assert( SIZE( names ) > names_len );
	names[ names_len++] = { val, 0xFFFFFFFF, str };

	return str;
}

//----------------------------------------------------------------
void Flags::print( std::ostream& output ) const
{
	for( int i = names_len; i > 0; )
	{
		--i;
		if( (val & names[i].mask) == names[i].val )
		{
			output << names[i].name << separator;
			if( names[i].mask == 0xFFFFFFFF )
				break;
		}
	}
}

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
	operator const uint64_t&() const { return u.key; };
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
void print_max_info( const char* filename )
{
	char buf[4096];
	IMAGE_NT_HEADERS*	NT_headers	= load_NT_headers( filename, PS( buf ) );
	Image_section_header*	section_header	= (Image_section_header*)IMAGE_FIRST_SECTION( NT_headers );

	printf( "%s: -------- Section Table --------\n"
		"Name     Virt Addr  Virt Size  Raw Addr  Raw Size  Flags\n"
	      , filename
	      );
	for( WORD i = 0; i < NT_headers->FileHeader.NumberOfSections; i++ )
	{
		printf	( "%-9.8s % 8X   % 8X  % 8X  % 8X  %s\n" //outfmt[0][radix] 
			, section_header[i].name.c_str()
			, section_header[i].VirtualAddress
			, section_header[i].Misc.VirtualSize
			, section_header[i].PointerToRawData
			, section_header[i].SizeOfRawData
			, section_header[i].characteristics.c_str()
			);
	}
	printf( "\n" );
}

//----------------------------------------------------------------
Map< Section_name, DWORD[MAX_FILENAMES], MAX_SECTOIN> sections;

//----------------------------------------------------------------
void load_n_printf_sizes( int argc, const char* argv[] )
{
	if( !argc )
		return;

	if( argc > MAX_FILENAMES )
		FATALMSG( argv[ MAX_FILENAMES], "Too many files (must be <= " MAX_FILENAMES_S ")" );

	for( int i = 0; i < argc; i++ )
	{
		char buf[4096];
		IMAGE_NT_HEADERS* NT_headers = load_NT_headers( argv[i], PS( buf ) );
		Image_section_header* section_header = (Image_section_header*) IMAGE_FIRST_SECTION( NT_headers );

		for( size_t s = 0; s < NT_headers->FileHeader.NumberOfSections; s++ )
			sections[ section_header[s].name ][i] = section_header[s].Misc.VirtualSize;
	}

	// печатаем заголовок таблицы
	for( auto& section : sections )
		printf( "%9.8s", section.key.c_str() );
	printf( " filename\n" );

	// печатаем таблицу
	for( int i = 0; i < argc; i++ )
	{
		for( auto& section : sections )
			printf( "% 9X", section.value[i] );
		printf( " %s\n", argv[i] );
	}
}

//----------------------------------------------------------------
int main( int argc, const char* argv[] )
{
	setlocale( LC_ALL, "" );

	const char* cmd = argv[0];
	if( 1 == argc )
		usage( cmd );

	while( 1)
	{
		switch( getopt( argc, argv, "hodxm:" ) )
		{
		case -1:
			load_n_printf_sizes( argc - optind, &argv[optind] );
			exit( 0 );
							break;
		case 'o': radix = 0;			break;
		case 'd': radix = 0;			break;
		case 'x': radix = 1;			break;
		case 'm': print_max_info( optarg );	break;
		case 'h': usage( cmd );			break;
		default	: usage( cmd );			break;
		}
	}
}
