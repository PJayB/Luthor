/*
    ---------------------------------------------------------------------------
    LUTHOR: a quick-n-dirty lexical analysis library for tokenizing a character
    stream using regular expressions.
    ---------------------------------------------------------------------------
	
    Copyright (C) 2013 Peter J. B. Lewis

    Permission is hereby granted, free of charge, to any person obtaining a 
    copy of this software and associated documentation files (the "Software"), 
    to deal in the Software without restriction, including without limitation 
    the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the 
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
    DEALINGS IN THE SOFTWARE.
*/
#pragma once
#ifndef _LEX_H_
#define _LEX_H_

#include <regex>

// To default Lex to Unicode or not, #define LEX_UNICODE as 0 or 1 before
// including Lex.h. This is not mandatory, however, as you can still override
// it when defining the Lexer: 
// 
//      Lex::Lexer<TokenID, std::string, std::regex>
//      Lex::Lexer<TokenID, std::wstring, std::wregex>
//
#ifndef LEX_UNICODE
#	ifdef _UNICODE
#		define LEX_UNICODE 1
#	else
#		define LEX_UNICODE 0
#	endif
#endif

namespace Lex
{

//-----------------------------------------------------------------------------
// This defines a location within a source stream. 
// line_number: The line within the file.
// within_line: The index of the character within that line (a.k.a. column)
// global: The 0-based offset into the stream
//-----------------------------------------------------------------------------
struct Location
{
    size_t line_number;
    size_t within_line;
    size_t global;
};

//-----------------------------------------------------------------------------
// Default implementations of string and regex based on Unicode build settings.
//-----------------------------------------------------------------------------
#if LEX_UNICODE
    typedef std::wstring default_string; 
    typedef std::wregex default_regex;
#else
    typedef std::string default_string;
    typedef std::regex default_regex;
#endif

//-----------------------------------------------------------------------------
// The Lexer is the main body of the Luthor library. It accepts three template
// parameters that determine the inputs and outputs of the Lexer:
//     _TokenID: A struct or class that identify matched tokens. Usually this
//               would be an enum, but you could make it anything: pointer, 
//               string, COM object... As long as your Match Handler can use it
//               to identify a token.
//     _String:  [OPTIONAL] A string class to use with the regex. Luthor has 
//               been tested with std::string and std::wstring.
//     _Regex:   [OPTIONAL] A regex class. Use std::regex or std::wregex.
//-----------------------------------------------------------------------------
template<
    typename _TokenID, 
    typename _String = default_string, 
    typename _Regex = default_regex>

class Lexer
{
public:

    // Map a token identifier to a regular expression defining that token
    void define(const _TokenID& id, const _String& definitionRegex)
    {
        m_expressions.push_back(TokenDef(id, definitionRegex));
    }

    // Analyze an character stream. This function takes two functors that are
    // called when a token is matched or fails to match. These functors should
    // implement operator(). See Example.cpp.
    template<
		typename _MatchFunc, 
		typename _ErrorFunc>

    void analyze(
		const _String& script, 
		_MatchFunc& onMatch, 
		_ErrorFunc& onError)
    {
        Location location;
        location.line_number = 1;
        location.within_line = 1;
        location.global = 0;

        auto start = std::begin(script);
        auto cursor = start;
        auto end = std::end(script);
        auto lastLineBegin = start;
        while (cursor < end)
        {
            // Match it against any of the tokens
            TokenMatch match = SearchRegex(cursor, end);

            location.global = cursor - start;
            location.within_line = 1 + cursor - lastLineBegin;

            if (match.Token == std::end(m_expressions))
            {
                onError(location);
            } else {
                onMatch(location, 
                    match.Token->ID, 
                    match.LexemeStart, 
                    match.LexemeEnd);
            }

            location.line_number += CountLineNums(
                cursor, 
                match.LexemeEnd, 
                lastLineBegin);
            cursor = match.LexemeEnd;
        }
    }

private:

    typedef typename _String::const_iterator _StringIt;

    struct TokenDef
    {
        TokenDef()
        {
        }

        TokenDef(const _TokenID& id, const _String& regex)
            : ID(id)
            , Expr(regex, std::regex::optimize)
        {
        }

        _Regex Expr;
        _TokenID ID;
    };

    struct TokenMatch
    {
        typename std::vector<TokenDef>::const_iterator Token;
        _StringIt LexemeStart;
        _StringIt LexemeEnd;
    };

    typename std::vector<TokenDef>::const_iterator MatchRegex(
        _StringIt start,
        _StringIt& end) const
    {
        // TODO: does an allocation happen here? That would suck :(
        std::match_results<_StringIt> results;
        for (auto expr = std::begin(m_expressions); 
             expr != std::end(m_expressions); 
             ++expr)
        {
            if (std::regex_search(start, end, results, expr->Expr,
                std::regex_constants::match_flag_type::match_continuous |
                std::regex_constants::match_flag_type::match_not_null |
                std::regex_constants::match_flag_type::format_no_copy |
                std::regex_constants::match_flag_type::format_first_only))
            {
                end = start + results.str().size();
                return expr;
            }
        }

        return std::end(m_expressions);
    }

    TokenMatch SearchRegex(
        _StringIt start,
        _StringIt end) const
    {
        TokenMatch match;
        match.LexemeStart = start;
        match.LexemeEnd = end; //start < end ? start + 1 : start;
        match.Token = std::end(m_expressions);
    
        if (start >= end)
        {
            return match;
        }

        match.Token = MatchRegex(start, match.LexemeEnd);

        // If there are no matches, return the start of the lexime so we can 
        // throw up an error at this location
        if (match.Token == std::end(m_expressions))
            match.LexemeEnd = match.LexemeStart;

        return match;
    }

    size_t CountLineNums(
        _StringIt a, 
        _StringIt b, 
        _StringIt& lineLineBegin)
    {
        size_t lineCount = 0;
        for ( ; a < b; ++a)
        {
            if (*a == (const _String::value_type)'\n')
            {
                lineLineBegin = a + 1;
                ++lineCount;
            }
        }
        return lineCount;
    }

    std::vector<TokenDef> m_expressions;
};

}

#endif
