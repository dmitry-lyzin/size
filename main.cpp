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
#include <ctype.h>
#include <fcntl.h>
#include <windows.h>

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

#define IMAGE_SCN(x,y,z) IMAGE_SCN_##y, #y
#define XXXX( c0, c1, c2, c3) (((c3) << 24) + ((c2) << 16) + ((c1) << 8) + (c0))

//----------------------------------------------------------------
void print_flags( DWORD section_flags)
{
	static const struct { DWORD value; const char* name; }
	scn[] =
	{ { IMAGE_SCN( 0x00000020, CNT_CODE,		"Section contains code"				) }
	, { IMAGE_SCN( 0x00000040, CNT_INITIALIZED_DATA,"Section contains initialized data"		) }
	, { IMAGE_SCN( 0x00000080, CNT_UNINITIALIZED_DATA,"Section contains uninitialized data"		) }
	, { IMAGE_SCN( 0x00000100, LNK_OTHER,		"Reserved"					) }
	, { IMAGE_SCN( 0x00000200, LNK_INFO,		"Section contains comments or some other type of information") }
	//, { IMAGE_SCN( 0x00000400, TYPE_OVER,		"Reserved"					) }
	, { IMAGE_SCN( 0x00000800, LNK_REMOVE,		"Section contents will not become part of image") }
	, { IMAGE_SCN( 0x00001000, LNK_COMDAT,		"Section contents comdat"			) }
	//, { IMAGE_SCN( 0x00002000, 0x00002000,	"Reserved"					) }
	//, { IMAGE_SCN( 0x00004000, MEM_PROTECTED,	"Obsolete"					) }
	, { IMAGE_SCN( 0x00004000, NO_DEFER_SPEC_EXC,	"Reset speculative exceptions handling bits in the TLB entries for this section") }
	, { IMAGE_SCN( 0x00008000, GPREL,		"Section content can be accessed relative to GP") }
	, { IMAGE_SCN( 0x00008000, MEM_FARDATA,		""						) }
	//, { IMAGE_SCN( 0x00010000, MEM_SYSHEAP,	"Obsolete"					) }
	, { IMAGE_SCN( 0x00020000, MEM_PURGEABLE,	""						) }
	, { IMAGE_SCN( 0x00020000, MEM_16BIT,		""						) }
	, { IMAGE_SCN( 0x00040000, MEM_LOCKED,		""						) }
	, { IMAGE_SCN( 0x00080000, MEM_PRELOAD,		""						) }
	, { IMAGE_SCN( 0x00100000, ALIGN_1BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00200000, ALIGN_2BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00300000, ALIGN_4BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00400000, ALIGN_8BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00500000, ALIGN_16BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00600000, ALIGN_32BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00700000, ALIGN_64BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00800000, ALIGN_128BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00900000, ALIGN_256BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00A00000, ALIGN_512BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00B00000, ALIGN_1024BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00C00000, ALIGN_2048BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00D00000, ALIGN_4096BYTES,	"Default alignment if no others are specified"	) }
	, { IMAGE_SCN( 0x00E00000, ALIGN_8192BYTES,	"Default alignment if no others are specified"	) }
	//, { IMAGE_SCN( 0x00F00000, 0x00F00000,	"Unused"					) }
	, { IMAGE_SCN( 0x01000000, LNK_NRELOC_OVFL,	"Section contains extended relocations"		) }
	, { IMAGE_SCN( 0x02000000, MEM_DISCARDABLE,	"Section can be discarded"			) }
	, { IMAGE_SCN( 0x04000000, MEM_NOT_CACHED,	"Section is not cachable"			) }
	, { IMAGE_SCN( 0x08000000, MEM_NOT_PAGED,	"Section is not pageable"			) }
	, { IMAGE_SCN( 0x10000000, MEM_SHARED,		"Section is shareable"				) }
	, { IMAGE_SCN( 0x20000000, MEM_EXECUTE,		"Section is executable"				) }
	, { IMAGE_SCN( 0x40000000, MEM_READ,		"Section is readable"				) }
	, { IMAGE_SCN( 0x80000000, MEM_WRITE,		"Section is writeable"				) }
	};

	for( int i = 0; i < SIZE( scn ); i++ )
	{
		if( scn[i].value & section_flags )
		{
			enum { CNT_ = XXXX('C','N','T','_'), LNK_ = XXXX('L','N','K','_'), MEM_ = XXXX('M','E','M','_') };
			const char* name = scn[i].name;
			DWORD name_prefix = *((DWORD*) name);
			switch( name_prefix )
			{
			case CNT_:
			case LNK_:
			case MEM_:
				name += sizeof( name_prefix);
			}
			while( *name )
				putchar( tolower( *name++ ));
			putchar( ' ' );
		}
	}
}

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
		FATALMSG( filename, "Invalid PE header" );

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
		print_flags( image_section_header[i].Characteristics );
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
