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
#include "Lex.h"

#include <iostream>
#include <iomanip>
#include <exception>
#include <memory>

//-----------------------------------------------------------------------------
// The lexer is robust to regular and wide strings. These defines are for the 
// application's benefit (e.g. printing output), so the lexer does not require
// them in any way.
//
// To see this in action on Windows, switch the build configuration between
// 'Debug' and 'Debug Unicode'.
//
// See Lex.h for more information
//-----------------------------------------------------------------------------
#ifndef _T
#	undef _T
#endif

#ifdef _UNICODE
#	define tchar	wchar_t
#   define tstring  wstring
#   define tregex   wregex
#   define tcout    wcout
#	define _T(x)	L##x
#else
#	define tchar	char
#   define tstring  string
#   define tregex   regex
#   define tcout    cout
#	define _T(x)	x
#endif

using namespace std;

//-----------------------------------------------------------------------------
// Here is the example script to be tokenized. It has plenty of horrible 
// spacing to demonstrate the robustness of the lexer to poorly formatted code.
//-----------------------------------------------------------------------------
const tchar* c_script = 
    _T("script \"TestScript\"\n")
    _T("\n")
    _T("\n")
    _T("// This is a comment\n")
    _T("function TestFunc\n")
    _T("{\n")
    _T("		TestInstruction 1, 2, 3  // This is also a comment\n")
    _T("\n")
    _T("    Teapot \"A    string\",	    jazzy\n")
    _T("}\n")
    _T("\n")
    _T("\n")
    _T("function Ginger\n")
    _T("{\n")
    _T("    Hello, World    \n")
    _T("    \n")
    _T("}\n");

//-----------------------------------------------------------------------------
// In this example we map token definitions (see main()) to ID numbers listed
// here. These are some common ones.
//-----------------------------------------------------------------------------
enum TOKEN_ID
{
    TOKEN_COMMENT,
    TOKEN_FUNCTION,
    TOKEN_SCRIPT,
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_COMMA,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_WHITESPACE,
    TOKEN_NEWLINE
};

//-----------------------------------------------------------------------------
// Each token that gets identified is stored with it's location within the
// source stream. This means any errors at hypothetical later stages (e.g. 
// parsing) can be correctly mapped back to the location within the source 
// text.
//-----------------------------------------------------------------------------
struct MatchedToken
{
    Lex::Location Location;
    TOKEN_ID ID;
    tstring Lexeme;
};

//-----------------------------------------------------------------------------
// When the lexer identifies a valid token, it calls operator() on whatever
// you pass in to the analyze() function. In this example we simply accumulate
// a list of tokens.
//-----------------------------------------------------------------------------
struct MatchedTokenList
{
    vector<MatchedToken> Tokens;

    void operator ()(
        const Lex::Location& location,
        TOKEN_ID id, 
        tstring::const_iterator leximeBegin, 
        tstring::const_iterator leximeEnd)
    {
        MatchedToken t;
        t.Location = location;
        t.ID = id;
        t.Lexeme.append(leximeBegin, leximeEnd);
        Tokens.push_back(move(t));
    }
};

//-----------------------------------------------------------------------------
// We throw this from our error handling function (see below).
//-----------------------------------------------------------------------------
struct SyntaxErrorException
{
    SyntaxErrorException(const Lex::Location& loc)
        : Location(loc)
    {
    }

    Lex::Location Location;
};

//-----------------------------------------------------------------------------
// The lexer calls this if an unknown token is found. The location of the
// token is provided. In this example we throw a SyntaxErrorException.
//-----------------------------------------------------------------------------
struct ErrorHandler
{
    void operator ()(
        const Lex::Location& location)
    {
        throw SyntaxErrorException(location);
    }
};

//-----------------------------------------------------------------------------
const tchar* TokenIDToString(TOKEN_ID id);

//-----------------------------------------------------------------------------
// Take an input stream (given above) and print out the tokens
//-----------------------------------------------------------------------------
int main(int argc, tchar* argv[])
{
    Lex::Lexer<TOKEN_ID> lex;
    lex.define(TOKEN_COMMENT,    _T("//.*\\n"));
    lex.define(TOKEN_FUNCTION,   _T("function"));
    lex.define(TOKEN_SCRIPT,     _T("script"));
    lex.define(TOKEN_IDENTIFIER, _T("[a-zA-Z_][a-zA-Z0-9_]*"));
    lex.define(TOKEN_INTEGER,    _T("[0-9]+"));
    lex.define(TOKEN_FLOAT,      _T("[0-9]+\\.[0-9]*"));
    lex.define(TOKEN_STRING,     _T("\".*\""));
    lex.define(TOKEN_COMMA,      _T(","));
    lex.define(TOKEN_LBRACE,     _T("\\{"));
    lex.define(TOKEN_RBRACE,     _T("\\}"));
    lex.define(TOKEN_WHITESPACE, _T("[ \\t]+"));
    lex.define(TOKEN_NEWLINE,    _T("(\\r?\\n)+"));
    
    try
    {
        MatchedTokenList matches;
        lex.analyze(c_script, matches, ErrorHandler());

        for (auto& token : matches.Tokens)
        {
			tcout << _T("Line ")
                  << token.Location.line_number 
		          << _T(", col ")
                  << token.Location.within_line
                  << _T(": ")
                  << TokenIDToString(token.ID) 
                  << _T(" '");
				  
            for (const tstring::value_type* c = token.Lexeme.c_str(); *c; ++c)
            {
                switch (*c)
                {
                case L'\n': tcout << _T("\\n"); break; 
                case L'\t': tcout << _T("\\t"); break; 
                default: tcout << *c;
                }
            }
            tcout << _T("'") << endl;
        }
    }
    catch (const SyntaxErrorException& syntaxError)
    {
        auto syntaxErrorPtr = &c_script[syntaxError.Location.global];
        tcout << _T("SYNTAX ERROR: Line ") 
              << syntaxError.Location.line_number
              << _T(", col ")
			  << syntaxError.Location.within_line
              << _T(": ") 
			  << syntaxErrorPtr;
    }
    catch (const exception& ex)
    {
        tcout << _T("EXCEPTION: ");
#ifdef _UNICODE
        size_t errLen = strlen(ex.what());
        std::unique_ptr<tchar> str(new tchar[errLen+1]);
        mbtowc(str.get(), ex.what(), errLen+1);
        printf("%s\n", str.get());
#else
        tcout << ex.what() << endl;
#endif
    }

	return 0;
}

//-----------------------------------------------------------------------------
// Converts a TOKEN_ID enum to a string
//-----------------------------------------------------------------------------
const tchar* TokenIDToString(TOKEN_ID id)
{
#define IDSTR(x)    case TOKEN_##x: return _T(#x);
    switch (id)
    {
    IDSTR(COMMENT)
    IDSTR(FUNCTION)
    IDSTR(SCRIPT)
    IDSTR(IDENTIFIER)
    IDSTR(INTEGER)
    IDSTR(FLOAT)
    IDSTR(STRING)
    IDSTR(COMMA)
    IDSTR(LBRACE)
    IDSTR(RBRACE)
    IDSTR(WHITESPACE)
    IDSTR(NEWLINE)
    default: throw exception("Bad token ID.");
    }
#undef IDSTR
}

