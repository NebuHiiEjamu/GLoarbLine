// =====================================================================
//  CString.h                            (C) Hotline Communications 2000
// =====================================================================
//
// Declaration of our string class.
// Based on (and lifted from) the standard C++ library string.

#ifndef _H_CString_
#define _H_CString_

#include <string>

HL_Begin_Namespace_BigRedH

class CString {
	public:
		// types:
		typedef wchar_t										value_type;
		typedef	std::allocator<wchar_t>						allocator_type;
		typedef std::allocator<wchar_t>::size_type 			size_type;
		typedef std::allocator<wchar_t>::difference_type 	difference_type;
		typedef allocator_type::reference			reference;
		typedef allocator_type::const_reference  	const_reference;
		typedef allocator_type::pointer			pointer;
		typedef allocator_type::const_pointer		const_pointer;	
		typedef std::wstring::iterator 						iterator;
		typedef std::wstring::const_iterator 				const_iterator;
		typedef std::wstring::reverse_iterator 				reverse_iterator;
		typedef std::wstring::const_reverse_iterator 		const_reverse_iterator;
		
		enum { npos = -1 };
//		static const size_type npos = size_type(-1);
		
		// _lib.string.cons_ construct/copy/destroy:
						CString () {}
						CString (const std::wstring& str)
							: mString(str) {}
						CString(
							const CString& str
							,size_type pos=0
							,size_type n = npos)
							: mString(str.mString, pos, npos) {}
						CString(
							const wchar_t* s
							,size_type n)
							: mString(s,n) {}
						CString(const wchar_t* s)
							: mString(s) {}
						CString(
							size_type n
							,wchar_t c)
							: mString(n,c) {}
								
						CString(
							const_iterator begin
							,const_iterator end)
							: mString(begin, end) {}
			
		// Conversion constructors; from ASCII in various forms	
		CString (const char* inPtr);
		explicit CString (const unsigned char* inPtr);
		explicit CString (const void* inPtr, size_type inLength);
		// Assign
		CString& operator= (const CString& rhs) 
			{	mString = rhs.mString; return *this; }
		CString& operator= (const wchar_t* s)
			{ 	mString = s; return *this; }
		CString& operator= (wchar_t c)
			{ 	mString = c; return *this; }
			
		// _lib.string.iterators_ iterators:
		iterator begin() { return mString.begin(); }
		const_iterator begin() const { return mString.begin(); }
		iterator end() { return mString.end(); }
		const_iterator end() const { return mString.end(); }

		reverse_iterator rbegin() { return mString.rbegin(); }
		const_reverse_iterator rbegin() const { return mString.rbegin(); }
		reverse_iterator rend() { return mString.rend(); }
		const_reverse_iterator rend() const { return mString.rend(); }
		
		// _lib.string.capacity_ capacity:
		size_type size() const { return mString.size(); }
		size_type length() const { return mString.length(); }
		size_type max_size() const { return mString.max_size(); }
		void resize(size_type n, wchar_t c) { mString.resize(n,c); }
		void resize(size_type n) { mString.resize(n); }
		size_type capacity() const { return mString.capacity(); }
		void reserve(size_type res_arg = 0) { mString.reserve(res_arg); }
		bool empty() const { return mString.empty(); }
		// _lib.string.access_ element access:
		const_reference operator[](size_type pos) const
			{ return mString[pos]; }
		reference       operator[](size_type pos)
			{ return mString[pos]; }
		const_reference at(size_type pos) const
			{ return mString.at(pos); }
		reference       at(size_type pos)
			{ return mString.at(pos); }
		// _lib.string.modifiers_ modifiers:
		CString& operator+=(const CString& str)
			{ mString += str.mString; return *this; }
		CString& operator+=(const wchar_t* s)
			{ mString += s; return *this; }
		CString& operator+=(wchar_t c)
			{ mString += c; return *this; }
		CString& append(const CString& str)
			{ mString.append(str.mString); return *this; }
		CString& append(const CString& str, size_type pos, size_type n)
			{ mString.append(str.mString, pos, n); return *this; }
		CString& append(const wchar_t* s, size_type n)
			{ mString.append(s,n); return *this; }
		CString& append(const wchar_t* s)
			{ mString.append(s); return *this; }
		CString& append(size_type n, wchar_t c)
			{ mString.append(n,c); return *this; }
		CString& append(const_iterator first, const_iterator last)
			{ mString.append(first, last); return *this; }
		void push_back(wchar_t c)
			{ mString.append(1,c); }

		CString& assign(const CString& str)
			{ mString.assign(str.mString); return *this; }
			
		CString& assign(const CString& str, size_type pos, size_type n)
			{ mString.assign(str.mString, pos, n); return *this; }
			
		CString& assign(const wchar_t* s, size_type n)
			{ mString.assign(s,n); return *this; }
			
		CString& assign(const wchar_t* s)
			{ mString.assign(s); return *this; }
			
		CString& assign(size_type n, wchar_t c)
			{ mString.assign(n,c); return *this; }
			
		inline
		CString& assign(const_iterator first, const_iterator last)
			{	mString.assign(first, last); return *this; }
		
		CString& insert(size_type pos1, const CString& str)
			{ mString.insert(pos1, str.mString); return *this; }
			
		CString& insert(size_type pos1, const CString& str, size_type pos2, size_type n)
			{ mString.insert(pos1, str.mString, pos2, n); return *this; }
			
		CString& insert(size_type pos, const wchar_t* s, size_type n)
			{ mString.insert(pos, s, n); return *this; }
			
		CString& insert(size_type pos, const wchar_t* s)
			{ mString.insert(pos, s); return *this; }
			
		CString& insert(size_type pos, size_type n, wchar_t c)
			{ mString.insert(pos, n, c); return *this; }
			
		iterator insert(iterator p, wchar_t c)
			{ return mString.insert(p,c); }
			
		void     insert(iterator p, size_type n, wchar_t c)
			{ mString.insert(p,n,c); }

		void insert(iterator p, const_iterator first, const_iterator last)
			{ mString.insert(p,first,last);	}
	
		CString& erase(size_type pos = 0, size_type n = npos)
			{ mString.erase(pos,n); return *this; }
			
		iterator erase(iterator position)
			{ return mString.erase(position); }
			
		iterator erase(iterator first, iterator last)
			{ return mString.erase(first, last); }
			
		CString& replace(size_type pos1, size_type n1, const CString& str)
			{ mString.replace(pos1, n1, str.mString); return *this; }
			
		CString& replace(size_type pos1, size_type n1, const CString& str,
		                      size_type pos2, size_type n2)
		    { mString.replace(pos1, n1, str.mString, pos2, n2); return *this; }
		    
		CString& replace(size_type pos, size_type n1, const wchar_t* s, size_type n2)
			{ mString.replace(pos, n1, s, n2); return *this; }
			
		CString& replace(size_type pos, size_type n1, const wchar_t* s)
			{ mString.replace(pos, n1, s); return *this; }
			
		CString& replace(size_type pos, size_type n1, size_type n2, wchar_t c)
			{ mString.replace(pos, n1, n2, c); return *this; }
			
		CString& replace(iterator i1, iterator i2, const CString& str)
			{ mString.replace(i1, i2, str.mString); return *this; }
			
		CString& replace(iterator i1, iterator i2, const wchar_t* s, size_type n)
			{ mString.replace(i1, i2, s, n); return *this; }
			
		CString& replace(iterator i1, iterator i2, const wchar_t* s)
			{ mString.replace(i1, i2, s); return *this; }
			
		CString& replace(iterator i1, iterator i2, size_type n, wchar_t c)
			{ mString.replace(i1, i2, n, c); return *this; }
	
		CString& replace(iterator i1, iterator i2, const_iterator j1, const_iterator j2)
			{ mString.replace(i1, i2, j1, j2); return *this; }
			
		size_type copy(wchar_t* s, size_type n, size_type pos = 0) const
			{ return mString.copy(s,n,pos); }
			
		void swap(CString& str)
			{ mString.swap(str.mString); }
			
		// _lib.string.ops_ string operations:

		const wchar_t* c_str() const
			{ return mString.c_str(); }
		const wchar_t* data() const
			{ return mString.data(); }
		allocator_type get_allocator() const
			{ return mString.get_allocator(); }
			
		size_type find (const CString& str, size_type pos = 0) const
			{ return mString.find(str.mString,pos); }
			
		size_type find (const wchar_t* s, size_type pos, size_type n) const
			{ return mString.find(s,pos,n); }
			
		size_type find (const wchar_t* s, size_type pos = 0) const
			{ return mString.find(s,pos); }
			
		size_type find (wchar_t c, size_type pos = 0) const
			{ return mString.find(c,pos); }
			
		size_type rfind(const CString& str, size_type pos = npos) const
			{ return mString.rfind(str.mString,pos); }
			
		size_type rfind(const wchar_t* s, size_type pos, size_type n) const
			{ return mString.rfind(s,pos,n); }
			
		size_type rfind(const wchar_t* s, size_type pos = npos) const
			{ return mString.rfind(s,pos); }
			
		size_type rfind(wchar_t c, size_type pos = npos) const
			{ return mString.rfind(c,pos); }

		size_type find_first_of(const CString& str, size_type pos = 0) const
			{ return mString.find_first_of(str.mString,pos); }
			
		size_type find_first_of(const wchar_t* s, size_type pos, size_type n) const
			{ return mString.find_first_of(s,pos,n); }
			
		size_type find_first_of(const wchar_t* s, size_type pos = 0) const
			{ return mString.find_first_of(s,pos); }
			
		size_type find_first_of(wchar_t c, size_type pos = 0) const
			{ return mString.find_first_of(c,pos); }
			
		size_type find_last_of(const CString& str, size_type pos = npos) const
			{ return mString.find_last_of(str.mString, pos); }
			
		size_type find_last_of(const wchar_t* s, size_type pos, size_type n) const
			{ return mString.find_last_of(s,pos,n); }
			
		size_type find_last_of(const wchar_t* s, size_type pos = npos) const
			{ return mString.find_last_of(s,pos); }
			
		size_type find_last_of(wchar_t c, size_type pos = npos) const
			{ return mString.find_last_of(c,pos); }
			
		size_type find_first_not_of(const CString& str, size_type pos = 0) const
			{ return mString.find_first_not_of(str.mString, pos); }
			
		size_type find_first_not_of(const wchar_t* s, size_type pos, size_type n) const
			{ return mString.find_first_not_of(s,pos,n); }
			
		size_type find_first_not_of(const wchar_t* s, size_type pos = 0) const
			{ return mString.find_first_not_of(s,pos); }
			
		size_type find_first_not_of(wchar_t c, size_type pos = 0) const
			{ return mString.find_first_not_of(c,pos); }
			
		size_type find_last_not_of (const CString& str, size_type pos = npos) const
			{ return mString.find_last_not_of(str.mString,npos); }
			
		size_type find_last_not_of (const wchar_t* s, size_type pos, size_type n) const
			{ return mString.find_last_not_of(s,pos,n); }
			
		size_type find_last_not_of (const wchar_t* s, size_type pos = npos) const
			{ return mString.find_last_not_of(s,pos); }
			
		size_type find_last_not_of (wchar_t c, size_type pos = npos) const
			{ return mString.find_last_not_of(c,pos); }
			
		CString substr(size_type pos = 0, size_type n = npos) const
			{ return mString.substr(pos,n); }
			
		int compare(const CString& str) const
			{ return mString.compare(str.mString); }
			
		int compare(size_type pos1, size_type n1, const CString& str) const
			{ return mString.compare(pos1, n1, str.mString); }
			
		int compare(size_type pos1, size_type n1, const CString& str,
		            size_type pos2, size_type n2) const
		    { return mString.compare(pos1, n1, str.mString, pos2, n2); }
		    
		int compare(const wchar_t* s) const
			{ return mString.compare(s); }
			
		int compare(size_type pos1, size_type n1, const wchar_t* s) const
			{ return mString.compare(pos1, n1, s); }
			
		int compare(size_type pos1, size_type n1, const wchar_t* s, size_type n2) const
			{ return mString.compare(pos1, n1, s, n2); }

		int compareNoCase (const CString& str) const;
		
		// Comparison operations	
		bool operator== (const CString& rhs) const;
		bool operator!= (const CString& rhs) const
			{ return !((*this) == rhs); }
		
		bool operator< (const CString& rhs) const;
		bool operator<= (const CString& rhs) const
			{ return ((*this) == rhs || (*this) < rhs); }
		bool operator> (const CString& rhs) const
			{ return !((*this) <= rhs); }
		bool operator>= (const CString& rhs) const
			{ return !((*this) < rhs); }
		
	private:				
		void Init (const void* inPtr, size_type inLength);
		std::wstring mString;	
};

// Operators
inline CString 
operator+ (const CString& lhs, const CString& rhs)
{
	CString retVal = lhs;
	retVal += rhs;
	return retVal;
}

inline CString
operator+ (const CString& lhs, wchar_t rhs)
{
	CString retVal = lhs;
	retVal += rhs;
	return retVal;
}

HL_End_Namespace_BigRedH

#endif