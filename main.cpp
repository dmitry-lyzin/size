#define MAX_FILENAMES	50
#define MAX_FILENAMES_S	"50"
#define MAX_SECTOIN	20
#define MAX_SECTOIN_S	"20"

//----------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <locale.h>
#include <fcntl.h>
#include <windows.h>
#include <iostream>
#include "constexpr_lowercase.h"

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
#define FATALMSG( x, errmsg) do { fprintf( stderr, "%s: " errmsg "\n", (x) ); exit( EXIT_FAILURE); } while (0)
#define SIZE(x) (sizeof (x) / sizeof( *(x)))
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

#pragma region // class Flags

//----------------------------------------------------------------
class Flags
{
	typedef struct { DWORD val; const char* name; } Names;
	static const Names names[];
	DWORD val;
public:
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
	if( a[0] == 'l' && a[1] == 'n' && a[2] == 'k' && a[3] == '_') return a + 4;					\
	if( a[0] == 'm' && a[1] == 'e' && a[2] == 'm' && a[3] == '_') return a + 4;					\
	return a;													\
}()

#define IMAGE_SCN_TYPE_REG		0x00000000  // Reserved.
#define IMAGE_SCN_TYPE_NOLOAD		0x00000002  // Reserved.
#define IMAGE_SCN_TYPE_GROUP		0x00000004  // Reserved.
#define IMAGE_SCN_TYPE_COPY		0x00000010  // Reserved.
#define IMAGE_SCN_TYPE_OVER		0x00000400  // Reserved.
#define IMAGE_SCN_0x00002000		0x00002000  // Reserved.
#define IMAGE_SCN_MEM_PROTECTED		0x00004000  // Obsolete
#define IMAGE_SCN_MEM_SYSHEAP		0x00010000  // Obsolete

#define IMAGE_SCN(x) { IMAGE_SCN_##x, CONSTEXPR_TOLOWER1(#x) }

const Flags::Names Flags::names[] =
{ IMAGE_SCN( TYPE_REG			)
, IMAGE_SCN( SCALE_INDEX		) // 0x00000001  // Tls index is scaled
//, IMAGE_SCN( TYPE_DSECT		) // 0x00000001  // Reserved.
, IMAGE_SCN( TYPE_NOLOAD		)
, IMAGE_SCN( TYPE_GROUP			)
, IMAGE_SCN( TYPE_NO_PAD		)
, IMAGE_SCN( TYPE_COPY			)

, IMAGE_SCN( CNT_INITIALIZED_DATA	)
, IMAGE_SCN( CNT_UNINITIALIZED_DATA	)

, IMAGE_SCN( LNK_INFO			)
, IMAGE_SCN( TYPE_OVER			)
, IMAGE_SCN( LNK_REMOVE			)
, IMAGE_SCN( LNK_COMDAT			)

, IMAGE_SCN( MEM_PROTECTED		)
, IMAGE_SCN( NO_DEFER_SPEC_EXC		)
, IMAGE_SCN( GPREL			)
, IMAGE_SCN( MEM_FARDATA		)
, IMAGE_SCN( MEM_SYSHEAP		)
, IMAGE_SCN( MEM_PURGEABLE		)
, IMAGE_SCN( MEM_16BIT			)
, IMAGE_SCN( MEM_LOCKED			)
, IMAGE_SCN( MEM_PRELOAD		)

, IMAGE_SCN( ALIGN_2BYTES		)
, IMAGE_SCN( ALIGN_4BYTES		)
, IMAGE_SCN( ALIGN_8BYTES		)
, IMAGE_SCN( ALIGN_16BYTES		)
, IMAGE_SCN( ALIGN_32BYTES		)
, IMAGE_SCN( ALIGN_64BYTES		)
, IMAGE_SCN( ALIGN_128BYTES		)
, IMAGE_SCN( ALIGN_256BYTES		)
, IMAGE_SCN( ALIGN_512BYTES		)
, IMAGE_SCN( ALIGN_1024BYTES		)
, IMAGE_SCN( ALIGN_2048BYTES		)
, IMAGE_SCN( ALIGN_4096BYTES		)
, IMAGE_SCN( ALIGN_8192BYTES		)
, IMAGE_SCN( ALIGN_MASK			)

, IMAGE_SCN( MEM_DISCARDABLE		)
, IMAGE_SCN( MEM_NOT_CACHED		)
, IMAGE_SCN( MEM_NOT_PAGED		)
, IMAGE_SCN( MEM_SHARED			)
, IMAGE_SCN( MEM_EXECUTE		)
, IMAGE_SCN( MEM_READ			)
, IMAGE_SCN( MEM_WRITE			)
};

//----------------------------------------------------------------
void Flags::print(std::ostream& output) const
{
	for( int i = 0; i < SIZE( names ); i++ )
	{
		if( names[i].val & val )
			output << names[i].name << ' ';
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
void print_max_info( const char* filename )
{
	char buf[4096];
	IMAGE_NT_HEADERS*	image_NT_headers	= load_NT_headers( filename, PS( buf ) );
	IMAGE_SECTION_HEADER*	image_section_header	= IMAGE_FIRST_SECTION( image_NT_headers );

	printf( "%s: -------- Section Table --------\n"
		"Name     Virt Addr  Virt Size  Raw Addr  Raw Size  Flags\n"
	      , filename
	      );
	for( WORD i = 0; i < image_NT_headers->FileHeader.NumberOfSections; i++ )
	{
		printf	( outfmt[0][radix] //"%-9.8s % 8X   % 8X  % 8X  % 8X  "
			, image_section_header[i].Name
			, image_section_header[i].VirtualAddress
			, image_section_header[i].Misc.VirtualSize
			, image_section_header[i].PointerToRawData
			, image_section_header[i].SizeOfRawData
			);

		std::cout << *((Flags*) &(image_section_header[i].Characteristics ));

		printf( "\n" );
	}
	printf( "\n" );
}

//----------------------------------------------------------------
size_t section_names_len	= 0;
size_t files_len		= 0;
uint64_t							section_names	[ MAX_SECTOIN	] = { 0 };
struct { DWORD VirtualSize[ MAX_SECTOIN]; const char* name; }	files		[ MAX_FILENAMES	] = { 0 };

//----------------------------------------------------------------
void mem_sections_size( const char* filename )
{
	char buf[4096];
	IMAGE_NT_HEADERS* image_NT_headers = load_NT_headers( filename, PS( buf ) );
	IMAGE_SECTION_HEADER* image_section_header = IMAGE_FIRST_SECTION( image_NT_headers );

	if( files_len >= SIZE( files ) )
		FATALMSG( filename, "Too many files (more " MAX_FILENAMES_S ")" );

	files[files_len].name = filename;

	for( WORD s = 0; s < image_NT_headers->FileHeader.NumberOfSections; s++ )
	{
		// ищем колонку с именем section_name
		const uint64_t section_name = *((uint64_t*) image_section_header[s].Name);
		for( size_t n = 0; n < section_names_len; n++ )
		{
			if( section_name == section_names[n] )
			{
				files[files_len].VirtualSize[n] = image_section_header[s].Misc.VirtualSize;
				goto next_s;
			}
		}

		// добавляем новую колонку
		if( section_names_len >= SIZE( section_names ) )
			FATALMSG( filename, "Too many section headers in the file (more " MAX_SECTOIN_S ")" );
		section_names			[ section_names_len] = section_name;
		files[ files_len].VirtualSize	[ section_names_len] = image_section_header[s].Misc.VirtualSize;
		section_names_len++;

	next_s:;
	}

	files_len++;
}

//----------------------------------------------------------------
NO_RETURN print_sections_size()
{
	if( !files_len )
		exit( 0 );

	// печатаем заголовок таблицы
	for( size_t col = 0; col < section_names_len; col++ )
		printf( "%9.8s", (char *) &(section_names[ col]) );
	printf( " filename\n" );

	// печатаем таблицу
	for( size_t row = 0; row < files_len; row++ )
	{
		for( size_t col = 0; col < section_names_len; col++ )
			printf( "% 9X", files[row].VirtualSize[col] );
		printf( " %s\n", files[row].name );
	}
	exit( 0 );
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
			for( int i = optind; i < argc; i++ )
				mem_sections_size( argv[i] );
			print_sections_size();
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
