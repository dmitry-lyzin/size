#ifndef LIGHT_H
#define LIGHT_H

#include <string.h>
#include <assert.h>
#include <array>
#include <initializer_list>

// для переключения между легкими и стандартными контейнерами (light:: и std::) применяйте нижеследующие:
//template <class Val> using Vector = light::Vector< Val, 10>;
//template <class Val> using Vector = std::vector< Val>;

namespace light
{

//----------------------------------------------------------------
template <class Val, size_t N>
struct Vector
{
	Val a[N];
	size_t s = 0;

	using ValX = std::conditional_t<(sizeof( Val ) > 8), Val&, Val>;
public:
	using value_type	= typename	 Val;
	using iterator		= typename	 Val*;
	using const_iterator	= typename const Val*;

	// Если я объявлю этот конструктор - пропадет возможность статической инициализации.
	// Что делать?
	//template <typename... ValX>
	//Vector( const ValX ...T ): a{ T... }, s( sizeof...(T) ) {}
	//Vector& operator=( std::initializer_list< ValX> list );
const	size_t max_size	(		) const { return std::size(a);				}
const	size_t size	(		) const { return s;					}
const	bool is_full	(		) const	{ return s >= max_size();			}
const	bool empty	(		) const	{ return !s;					}
	void resize	( size_t ns	)	{ assert( ns <= max_size()	); s = ns;	}
	void push_back	( const ValX x	)	{ assert( !is_full()		); a[s++] = x;	}
	Val* begin	(		)	{ return a;					}
	Val* end	(		)	{ return &a[s];					}
	Val& front	(		)	{ return a[0];					}
	Val& back	(		)	{ return a[s - 1];				}
	Val& operator*	(		)	{ return a[0];					}
	Val& operator[]	( size_t i	)	{ assert( i < s			); return a[i]; }
	Val& operator[]	( ptrdiff_t i	)	{ assert( i >= 0 && i < int( s)	); return a[i]; }
const	Val* begin	(		) const	{ return a;					}
const	Val* end	(		) const	{ return &a[s];					}
const	ValX front	(		) const	{ return a[0];					}
const	ValX back	(		) const	{ return a[s - 1];				}
const	ValX operator*	(		) const	{ return a[0];					}
const	ValX operator[]	( size_t i	) const { static_assert( i < s,			"out of range"); return a[i]; }
const	ValX operator[]	( ptrdiff_t i	) const { static_assert( i >= 0 && i < int( s),	"out of range"); return a[i]; }
};

//----------------------------------------------------------------
//template <class Val, size_t N>
//inline Vector< Val, N>& Vector< Val, N>::operator=( std::initializer_list< ValX> list )
//{
//	resize( list.size() );
//	std::copy( list.begin(), list.end(), a );
//	return *this;
//}

//----------------------------------------------------------------
template	< class First,	class Second	>
struct pair	{ First first;	Second second;	};

//----------------------------------------------------------------
template <class Key, class Value, size_t N>
struct Map: public Vector< pair< Key, Value>, N>
{
	using value_type = pair< Key, Value>;

	iterator exist( const Key& key )
	{
		iterator p = begin();
		for( size_t i = size(); i > 0; ++p )
		{
			--i;
			if( key == p->first )
				return p;
		}
		return NULL;
	}
	Value& operator[]( const Key& key )
	{
		iterator p = begin();
		for( size_t i = size(); i > 0; ++p )
		{
			--i;
			if( key == p->first )
				return p->second;
		}
		value_type tmp = { key, Value() };
		push_back( tmp );
		return back().second;
	}
};

//----------------------------------------------------------------
class Str // std::string_view ?
{
	char* b;
	char* e;
public:
	Str(				): b( NULL	), e( NULL			) {}
	Str( const char* x, size_t s	): b( (char*) x	), e( (char*) x + s		) {}
	Str( const char* x		): b( (char*) x	), e( (char*) x + strlen(x)	) {}

	size_t size		() const { return e - b; }
	char* begin		() const { return b; }
	char* end		() const { return e; }
	operator const char*	() const { return b; }

	Str& operator+=( const char x )
	{
		*e++ = x;
		return *this;
	}
	Str& operator+=( const Str& x )
	{
		memcpy( e, x.begin(), x.size() );
		e += x.size();
		return *this;
	}
};


//----------------------------------------------------------------
template <class iterator>
class reverse_iterator
{
	iterator i;
public:
	explicit operator iterator() const { return i; }

	reverse_iterator(			) = default;
	reverse_iterator( const iterator r	): i( r ) {}
	// clang-format off
	//template <class Other>
	//reverse_iterator( const reverse_iterator< Other>& r ) : i( r.i ) {}
	//template <class Other>
	//reverse_iterator& operator=( const reverse_iterator< Other>& r ) { i = r.i; return *this; }
	// clang-format on

	auto& operator* () const { iterator tmp = i; return *--tmp;	}
	auto* operator->() const { iterator tmp = i; --tmp; return tmp;	}

	auto& operator[]( ptrdiff_t off			) const { return i[-off - 1];	}
	bool  operator==( const reverse_iterator r	) const { return i == r.i;	}
	bool  operator!=( const reverse_iterator r	) const { return i != r.i;	}

	reverse_iterator& operator++(     ) { --i;				return *this;	}
	reverse_iterator  operator++( int ) { reverse_iterator tmp =*this; --i;	return tmp;	}
	reverse_iterator& operator--(	  ) { ++i;				return *this;	}
	reverse_iterator  operator--( int ) { reverse_iterator tmp =*this; ++i;	return tmp;	}

	reverse_iterator  operator+ ( ptrdiff_t off	) const	{		return i - off; }
	reverse_iterator& operator+=( ptrdiff_t off	)	{ i -= off;	return *this;	}
	reverse_iterator  operator- ( ptrdiff_t off	) const	{		return i + off;	}
	reverse_iterator& operator-=( ptrdiff_t off	)	{ i += off;	return *this;	}
};

//----------------------------------------------------------------
template< class Src>
class Revers
{
	Src& src;
public:
	using value_type	= typename Src::value_type;
	using iterator		= typename reverse_iterator< typename Src::iterator>;
	using const_iterator	= typename reverse_iterator< typename Src::const_iterator>;

	Revers( Src& x ) : src( x ) {};
	const Revers& operator=( const Src& r ) { src = r; return *this; }

	auto		size  () const	{ return src.size();	}
	iterator	begin ()	{ return src.end();	}
	iterator	end   ()	{ return src.begin();	}
	const_iterator	begin () const	{ return src.end();	}
	const_iterator	end   () const	{ return src.begin();	}
};

#define REVERS(x) light::Revers< decltype(x) >( x )

}
#endif
