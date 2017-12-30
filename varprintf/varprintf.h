#ifndef INC_PRINTF_H_0dbf3b418ef51bd59f6d997a555fc997
#define INC_PRINTF_H_0dbf3b418ef51bd59f6d997a555fc997

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>


namespace varPrintf
{
	namespace Internal
	{

		//Code for parsing and interpreting printf style format specifiers.
		struct format
		{
			enum
			{
				ALT = 1, ZP=2, LEFT=4, SPACE=8, SIGN=16, PERCENT=32, BAD=64,
				NO_PRECISION=-1, NO_WIDTH=-1
			};

			int flags;
			int  width, precision;
			char conversion;

			inline int parse(const std::string& fmt, int pos)
			{
				flags=0;
				width=NO_WIDTH;
				precision=NO_PRECISION;
				conversion=0;

				int s = fmt.size();

				if(pos == s)
				{
					flags |= BAD;
					return 0;
				}

				//Check for literal
				if(fmt[pos] == '%')
				{
					flags = PERCENT;
					return pos+1;
				}

				//Get flags (if any)
				for(; pos < s; pos++)
				{
					char c = fmt[pos];

					if(c == '#')
						flags |= ALT;
					else if(c == '0')
						flags |= ZP;
					else if(c == ' ')
						flags |= SPACE;
					else if(c == '-')
						flags |= LEFT;
					else if(c == '+')
						flags |= SIGN;
					else
						break;
				}

				//Get width
				for(; pos < s; pos++)
				{
					char c = fmt[pos];
					if(isdigit(c))
						if(width == NO_WIDTH)
							width = c - '0';
						else
							width = width * 10 + (c-'0');
					else
						break;
				}

				//Check for a precison
				if(pos < s && fmt[pos] == '.')
				{
					precision = 0;
					pos++;

					//Get width
					for(; pos < s; pos++)
					{
						char c = fmt[pos];
						if(isdigit(c))
							precision = precision * 10 + (c-'0');
						else
							break;
					}
				}

				//Now, this should be the conversion
				if(pos < s && isalpha(fmt[pos]))
					conversion = fmt[pos++];
				else
					flags = BAD;

				return pos;
			}
		};

		//To make it looks like ostream << format works
		template<class Char, class Traits> struct bound_format
		{
			bound_format(std::basic_ostream<Char, Traits>& os, const format& ff)
			:o(os), f(ff)
			{}

			std::basic_ostream<Char, Traits>& o;
			const format& f;
		};


		template<class Char, class Traits> bound_format<Char, Traits> operator<<(std::basic_ostream<Char, Traits>& o, const format& f)
		{
			return bound_format<Char, Traits>(o, f);
		}

		//Evaluate the result of osteram << format << X
		template<class Char, class Traits, class C> std::basic_ostream<Char, Traits>& operator<<(bound_format<Char,Traits> f, const C& c)
		{
			using std::ios;
			using namespace std;

			bool precision_is_max_width=0;


			//Save old stream state.
			int old_p = f.o.precision();
			Char old_fill = f.o.fill();
			ios::fmtflags old_flags = f.o.flags();

			//Conversion specific tricks
			//Defaults
			f.o.unsetf(ios::floatfield | ios::boolalpha);
			f.o.setf(ios::dec);
			f.o.fill(' ');

			//Process conversion characters. These can affect the formatting
			//parameters below.
			switch(f.f.conversion)
			{
				case 'f':
				case 'F':
					f.o.setf(ios::fixed);
					break;

				case 'e':
				case 'E':
					f.o.setf(ios::scientific);
					break;

				case 'g':
				case 'G':
					f.o.unsetf(ios::floatfield);
					break;

				case 'x':
				case 'X':
					f.o << hex;
					break;

				case 'o':
				case 'O':
					f.o << oct;
					break;

				case 'b':
				case 'B':
					f.o << boolalpha;
					break;

				case 's':
				case 'S':
					precision_is_max_width=1;
					break;

				case 'k':
				case 'K':
					return f.o;
					break;
			}



			if(f.f.width != format::NO_WIDTH)
				f.o.width(f.f.width);

			if(f.f.flags & format::ZP && !(f.f.flags & format::LEFT))
				f.o.fill('0');

			if(f.f.flags & format::SIGN)
				f.o.setf(ios::showpos);
			else
				f.o.unsetf(ios::showpos);

			if(f.f.flags & format::LEFT)
				f.o.setf(ios::left);
			else
				f.o.setf(ios::internal);

			if(f.f.flags & format::ALT)
				f.o.setf(ios::showbase | ios::showpoint);
			else
				f.o.unsetf(ios::showbase | ios::showpoint);


			if(isupper(f.f.conversion))
				f.o.setf(ios::uppercase);
			else
				f.o.unsetf(ios::uppercase);


			if(f.f.precision != format::NO_PRECISION && ! precision_is_max_width)
				f.o.precision(f.f.precision);

			if(precision_is_max_width && f.f.precision != format::NO_PRECISION)
			{
				ostringstream tmp;
				tmp.copyfmt(f.o);

				//Since we're doing the truncation by hand, then there should be
				//no width specification
				tmp << setw(0) << c;

				f.o << tmp.str().substr(0, f.f.precision);
			}
			else
				f.o << c;


			//Reset values
			f.o.precision(old_p);
			f.o.fill(old_fill);
			f.o.setf(old_flags);

			return f.o;
		}



		template<typename... Args>
		void stream_printf_runner(std::ostream& o, const std::string& fmt, int fpos, const Args... xs);

		template<typename T, typename ...Args>
		void print_or_not_and_recurse(std::ostream& o, const std::string& fmt, int fpos, const format& f, const T& x, const Args... xs)
		{
			o << f << x;
			stream_printf_runner(o, fmt, fpos, xs...);
		}

		void print_or_not_and_recurse(std::ostream& o, const std::string& fmt, int fpos, const format& f)
		{
			o << "<missing value>";
			stream_printf_runner(o, fmt, fpos);
		}

		template<typename... Args>
		void stream_printf_runner(std::ostream& o, const std::string& fmt, int fpos, const Args... xs)
		{
			for(;;)
			{
				size_t ppos = fmt.find('%', fpos);

				if(ppos == fmt.npos)
				{
					//No more format specifiers, so output the rest of the string
					o <<  fmt.substr(fpos);
					return;
				}

				//else output the strung up to the specifier
				o  << fmt.substr(fpos, ppos - fpos);

				//Parse the format string
				format f;
				int pos = f.parse(fmt, ppos+1);

				if(f.flags & format::PERCENT)
				{
					o << '%';
					fpos = pos;
					continue;
				}
				else if(f.flags & format::BAD)
				{
					o  << "<Malformed format>" << fmt.substr(ppos);
					return ;
				}
				else
				{
					print_or_not_and_recurse(o, fmt, pos, f, xs...);
					return;
				}
			}
		}
	}

	template<typename... Args>
	void Printf(const std::string& fmt, const Args... xs)
	{
		Internal::stream_printf_runner(std::cout, fmt, 0, xs...);
	}

	template<typename... Args>
	void fPrintf(std::ostream& out, const std::string& fmt, const Args... xs)
	{
		Internal::stream_printf_runner(out, fmt, 0, xs...);
	}

	template<typename... Args>
	const std::string sPrintf(const std::string& fmt, const Args... xs)
	{
		std::ostringstream str;
		Internal::stream_printf_runner(str, fmt, 0, xs...);
		return str.str();
	}
}
#endif
