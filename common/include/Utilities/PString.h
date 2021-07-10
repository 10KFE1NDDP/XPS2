#pragma once
#include "ghc/filesystem.h"
#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string>
#include "Path.h"

namespace fs = ghc::filesystem;

class PString
{
private:
	std::basic_string<char> string;
public:
	PString() = default;

	explicit PString(const fs::path& path)
	{
		string = path.u8string();
	}

	PString(const wxString& str)
	{
		string = std::string(str.utf8_str());
	}

	#ifdef _WIN32
	PString(const std::wstring& wString);
	operator std::wstring();
	#endif

	#ifdef __cpp_lib_char8_t
	PString::PString(const std::u8string& str)
	{
		string = std::string(str.begin(), str.end());
	}
	operator std::u8string();
	#endif

	// Copy Constructor
	PString(const PString& rhs)
	{
		string = rhs.string;
	}

	PString(PString&& move)
	{
		string = std::move(move.string);
	}

	// Return OS specific format
	std::string mb() const;

	// Return UTF8 explicit string
	std::string u8() const
	{
		return string;
	}

	const bool operator==(const PString& rhs)
	{
		return string == rhs.string;
	}

	operator std::string() const;
	operator wxString() const;
	explicit operator fs::path() const;

	size_t capacity() const noexcept
	{
		return string.capacity();
	}

	size_t size() const noexcept
	{
		return string.size();
	}

	char& at(size_t pos)
	{
		return string.at(pos);
	}

	const char& at(size_t pos) const
	{
		return string.at(pos);
	}

	void resize(size_t n)
	{
		string.resize(n);
	}

	void resize(size_t n, char c)
	{
		string.resize(n, c);
	}

	const char* data() const
	{
		return string.data();
	}
	
	const char* c_str() const noexcept
	{
		return string.c_str();
	}

	bool empty() const noexcept
	{
		return string.empty();
	}

	char& operator[](size_t index)
	{
		return string[index];
	}

	char operator[](size_t index) const
	{
		return string[index];
	}

	friend std::ostream& operator<<(std::ostream & os, const PString& str);
	friend std::istream& operator>>(std::istream& is, PString& str);
	friend std::istream& getline(std::istream& is, PString& s, char delim);
	~PString();
};

