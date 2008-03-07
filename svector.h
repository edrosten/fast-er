#ifndef SVECTOR_H
#define SVECTOR_H


#include <vector>
#include <iostream>
#ifdef SAFE_VECTOR
namespace std{

template<class T> class foovector: public std::vector<T>
{
	public:
		foovector()
		{}

		foovector(size_t n)
		:std::vector<T>(n)
		{}

		template<class C> foovector(size_t n, const C& v)
		:std::vector<T>(n, v)
		{}

		template<class C> foovector(const C& begin, const C& end)
		:std::vector<T>(begin, end)
		{}


		foovector(const std::vector<T>& v)
		:std::vector<T>(v)
		{}

		foovector& operator=(const std::vector<T>& v)
		{
			std::vector<T>::operator=(v);
			return *this;
		}

		T&back() { return this->operator[](std::vector<T>::size()-1); }
		const T&back() const { return this->operator[](std::vector<T>::size()-1); }

		T&front() { return this->operator[]();}
		const T&front() const { return this->operator[]();}

		T&operator[](size_t n) { 
			try{
				return std::vector<T>::at(n); 
			}
			catch(...)
			{
				std::cerr << "Bad access n=" << n << " size=" << std::vector<T>::size() << endl;
				throw;
			}
		}
		const T&operator[](size_t n)const {
			try{
				return std::vector<T>::at(n); 
			}
			catch(...)
			{
				std::cerr << "Bad access n=" << n << " size=" << std::vector<T>::size() << endl;
				throw;
			}
		}
};

template<> class foovector<bool>: public std::vector<bool>
{
	public:
		foovector()
		{}

		foovector(size_t n)
		:std::vector<bool>(n)
		{}

		template<class C> foovector(size_t n, const C& v)
		:std::vector<bool>(n, v)
		{}

		template<class C> foovector(const C& begin, const C& end)
		:std::vector<bool>(begin, end)
		{}


		foovector(const std::vector<bool>& v)
		:std::vector<bool>(v)
		{}

		foovector& operator=(const std::vector<bool>& v)
		{
			std::vector<bool>::operator=(v);
			return *this;
		}

		std::_Bit_reference operator[](size_t n) { return std::vector<bool>::at(n); }
		const bool operator[](size_t n)const { return std::vector<bool>::at(n); }
};


}

#define vector foovector
#endif
#endif
