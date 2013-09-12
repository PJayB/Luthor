LUTHOR
======

This library contains a quick-n-dirty lexical analyzer/tokenizer/scanner or whatever you want to call it.
Basically, it turns a stream of characters into a stream of tokens. These tokens are defined by regular expressions.

Example
-------

Take the following token definitions:

    Lex::Lexer<TOKEN_ID> lex;
    lex.define(TOKEN_COMMENT,    _T("//.*\\n"));
    lex.define(TOKEN_FUNCTION,   _T("function"));
    lex.define(TOKEN_SCRIPT,     _T("script"));
    lex.define(TOKEN_IDENTIFIER, _T("[a-zA-Z_][a-zA-Z0-9_]*"));
    lex.define(TOKEN_INTEGER,    _T("[0-9]+"));
    lex.define(TOKEN_STRING,     _T("\".*\""));
    lex.define(TOKEN_COMMA,      _T(","));
    lex.define(TOKEN_LBRACE,     _T("\\{"));
    lex.define(TOKEN_RBRACE,     _T("\\}"));
    lex.define(TOKEN_WHITESPACE, _T("[ \\t]+"));
    lex.define(TOKEN_NEWLINE,    _T("(\\r?\\n)+"));
    
And the following text:

    script "TestScript"
    function TestFunc
    {
		// Hello, World
		Check 1, 2
    }
    
Mix well:

    MatchedTokenList matches;
    lex.analyze(scriptSource, matches, errorHandler);

And you get something like this:

	Line 1, col 1: SCRIPT 'script'
	Line 1, col 7: WHITESPACE ' '
	Line 1, col 8: STRING '"TestScript"'
	Line 1, col 20: NEWLINE '\n'
	Line 2, col 1: FUNCTION 'function'
	Line 2, col 9: WHITESPACE ' '
	Line 2, col 10: IDENTIFIER 'TestFunc'
	Line 2, col 18: NEWLINE '\n'
	Line 3, col 1: LBRACE '{'
	Line 3, col 2: NEWLINE '\n'
	Line 4, col 1: WHITESPACE '\t'
	Line 4, col 2: COMMENT '// Hello, World\n'
	Line 5, col 1: WHITESPACE '\t'
	Line 5, col 2: IDENTIFIER 'Check'
	Line 5, col 7: WHITESPACE ' '
	Line 5, col 8: INTEGER '1'
	Line 5, col 9: COMMA ','
	Line 5, col 10: WHITESPACE ' '
	Line 5, col 11: INTEGER '2'
	Line 5, col 12: NEWLINE '\n'
	Line 6, col 1: RBRACE '}'
	Line 6, col 2: NEWLINE '\n'

Contact
-------
luthor at pjblewis dot com
