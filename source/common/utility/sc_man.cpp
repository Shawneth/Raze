
//**************************************************************************
//**
//** sc_man.c : Heretic 2 : Raven Software, Corp.
//**
//** $RCSfile: sc_man.c,v $
//** $Revision: 1.3 $
//** $Date: 96/01/06 03:23:43 $
//** $Author: bgokey $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include <string.h>
#include <stdlib.h>
#include "sc_man.h"
#include "cmdlib.h"
#include "templates.h"
#include "printf.h"
#include "name.h"
#include "v_text.h"
#include "cache1d.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// FScanner Constructor
//
//==========================================================================

FScanner::FScanner()
{
	ScriptOpen = false;
}

//==========================================================================
//
// FScanner Destructor
//
//==========================================================================

FScanner::~FScanner()
{
	// Humm... Nothing to do in here.
}

//==========================================================================
//
// FScanner Copy Constructor
//
//==========================================================================

FScanner::FScanner(const FScanner &other)
{
	ScriptOpen = false;
	*this = other;
}

//==========================================================================
//
// FScanner :: operator =
//
//==========================================================================

FScanner &FScanner::operator=(const FScanner &other)
{
	if (this == &other)
	{
		return *this;
	}
	if (!other.ScriptOpen)
	{
		Close();
		return *this;
	}

	// Copy protected members
	ScriptOpen = true;
	ScriptName = other.ScriptName;
	ScriptBuffer = other.ScriptBuffer;
	ScriptPtr = other.ScriptPtr;
	ScriptEndPtr = other.ScriptEndPtr;
	AlreadyGot = other.AlreadyGot;
	AlreadyGotLine = other.AlreadyGotLine;
	LastGotToken = other.LastGotToken;
	LastGotPtr = other.LastGotPtr;
	LastGotLine = other.LastGotLine;
	CMode = other.CMode;
	Escape = other.Escape;
	StateMode = other.StateMode;
	StateOptions = other.StateOptions;

	// Copy public members
	if (other.String == other.StringBuffer)
	{
		memcpy(StringBuffer, other.StringBuffer, sizeof(StringBuffer));
		BigStringBuffer = "";
		String = StringBuffer;
	}
	else
	{
		// Past practice means the string buffer must be writeable, which
		// removes some of the benefit from using an FString to store
		// the big string buffer.
		BigStringBuffer = other.BigStringBuffer;
		String = BigStringBuffer.LockBuffer();
	}
	StringLen = other.StringLen;
	TokenType = other.TokenType;
	Number = other.Number;
	Float = other.Float;
	Name = other.Name;
	Line = other.Line;
	End = other.End;
	Crossed = other.Crossed;

	return *this;
}

//==========================================================================
//
// FScanner :: Open
//
//==========================================================================

void FScanner::Open (const char *name)
{
	auto fr = kopenFileReader(name, 0);
	if (!fr.isOpen())
	{
		I_Error("Could not find script lump '%s'\n", name);
	}
	Close();
	auto data = fr.ReadPadded(1);
	ScriptBuffer = data;
	ScriptName = name;
	//LumpNum = lump;
	PrepareScript();
}

//==========================================================================
//
// FScanner :: Open
//
//==========================================================================

FScanner::FScanner(int lump)
{
	if ((unsigned)lump >= fileSystem.GetNumEntries())
	{
		I_Error("Invalid file index %d\n", lump);
	}
	Close();
	auto data = fileSystem.GetFileData(lump, 1);
	ScriptBuffer = data;
	ScriptName = fileSystem.GetFileName(lump);
	LumpNum = lump;
	PrepareScript();
}

//==========================================================================
//
// FScanner :: OpenFile
//
// Loads a script from a file. Uses new/delete for memory allocation.
//
//==========================================================================

void FScanner::OpenFile (const char *name)
{
	Close ();
	FileReader fr;
	if (!fr.OpenFile(name)) return;
	auto data = fr.ReadPadded(1);
	ScriptBuffer = data;
	ScriptName = name;	// This is used for error messages so the full file name is preferable
	LumpNum = -1;
	PrepareScript ();
}

//==========================================================================
//
// FScanner :: OpenMem
//
// Prepares a script that is already in memory for parsing. The memory is
// copied, so you can do whatever you want with it after opening it.
//
//==========================================================================

void FScanner::OpenMem (const char *name, const char *buffer, int size)
{
	OpenString(name, FString(buffer, size));
}

//==========================================================================
//
// FScanner :: OpenString
//
// Like OpenMem, but takes a string directly.
//
//==========================================================================

void FScanner::OpenString (const char *name, FString buffer)
{
	Close ();
	ScriptBuffer = buffer;
	ScriptName = name;
	LumpNum = -1;
	PrepareScript ();
}

//==========================================================================
//
// FScanner :: PrepareScript
//
// Prepares a script for parsing.
//
//==========================================================================

void FScanner::PrepareScript ()
{
	// The scanner requires the file to end with a '\n', so add one if
	// it doesn't already.
	if (ScriptBuffer.Len() == 0 || ScriptBuffer[ScriptBuffer.Len() - 1] != '\n')
	{
		// If the last character in the buffer is a null character, change
		// it to a newline. Otherwise, append a newline to the end.
		if (ScriptBuffer.Len() > 0 && ScriptBuffer[ScriptBuffer.Len() - 1] == '\0')
		{
			ScriptBuffer.LockBuffer()[ScriptBuffer.Len() - 1] = '\n';
			ScriptBuffer.UnlockBuffer();
		}
		else
		{
			ScriptBuffer += '\n';
		}
	}

	ScriptPtr = &ScriptBuffer[0];
	ScriptEndPtr = &ScriptBuffer[ScriptBuffer.Len()];
	Line = 1;
	End = false;
	ScriptOpen = true;
	String = StringBuffer;
	AlreadyGot = false;
	LastGotToken = false;
	LastGotPtr = NULL;
	LastGotLine = 1;
	CMode = false;
	Escape = true;
	StateMode = 0;
	StateOptions = false;
	StringBuffer[0] = '\0';
	BigStringBuffer = "";
}

//==========================================================================
//
// FScanner :: Close
//
//==========================================================================

void FScanner::Close ()
{
	ScriptOpen = false;
	ScriptBuffer = "";
	BigStringBuffer = "";
	StringBuffer[0] = '\0';
	String = StringBuffer;
}

//==========================================================================
//
// FScanner :: SavePos
//
// Saves the current script location for restoration later
//
//==========================================================================

const FScanner::SavedPos FScanner::SavePos ()
{
	SavedPos pos;

	CheckOpen ();
	if (End)
	{
		pos.SavedScriptPtr = NULL;
	}
	else
	{
		pos.SavedScriptPtr = ScriptPtr;
	}
	pos.SavedScriptLine = Line;
	return pos;
}

//==========================================================================
//
// FScanner :: RestorePos
//
// Restores the previously saved script location
//
//==========================================================================

void FScanner::RestorePos (const FScanner::SavedPos &pos)
{
	if (pos.SavedScriptPtr)
	{
		ScriptPtr = pos.SavedScriptPtr;
		Line = pos.SavedScriptLine;
		End = false;
	}
	else
	{
		End = true;
	}
	AlreadyGot = false;
	LastGotToken = false;
	Crossed = false;
}

//==========================================================================
//
// FScanner :: isText
//
// Checks if this is a text file.
//
//==========================================================================

bool FScanner::isText()
{
	for(unsigned int i=0;i<ScriptBuffer.Len();i++)
	{
		int c = ScriptBuffer[i];
		if (c < ' ' && c != '\n' && c != '\r' && c != '\t') return false;
	}
	return true;
}

//==========================================================================
//
// FScanner :: SetCMode
//
// Enables/disables C mode. In C mode, more characters are considered to
// be whole words than in non-C mode.
//
//==========================================================================

void FScanner::SetCMode (bool cmode)
{
	CMode = cmode;
}

//==========================================================================
//
// FScanner :: SetEscape
//
// Turns the escape sequence \" in strings on or off. If it's off, that
// means you can't include quotation marks inside strings.
//
//==========================================================================

void FScanner::SetEscape (bool esc)
{
	Escape = esc;
}

//==========================================================================
//
// FScanner :: SetStateMode
//
// Enters state mode. This mode is very permissive for identifiers, which
// it returns as TOK_NonWhitespace. The only character sequences that are
// not returned as such are these:
//
//   * stop
//   * wait
//   * fail
//   * loop
//   * goto - Automatically exits state mode after it's seen.
//   * :
//   * ;
//   * } - Automatically exits state mode after it's seen.
//
// Quoted strings are returned as TOK_NonWhitespace, minus the quotes. Once
// two consecutive sequences of TOK_NonWhitespace have been encountered
// (which would be the state's sprite and frame specifiers), nearly normal
// processing resumes, with the exception that various identifiers
// used for state options will be returned as tokens and not identifiers.
// This ends once a ';' or '{' character is encountered.
//
//==========================================================================

void FScanner::SetStateMode(bool stately)
{
	StateMode = stately ? 2 : 0;
	StateOptions = stately;
}

//==========================================================================
//
// FScanner::ScanString
//
// Set tokens true if you want TokenType to be set.
//
//==========================================================================

bool FScanner::ScanString (bool tokens)
{
	const char *marker, *tok;
	bool return_val;

	CheckOpen();
	if (AlreadyGot)
	{
		AlreadyGot = false;
		if (!tokens || LastGotToken)
		{
			return true;
		}
		ScriptPtr = LastGotPtr;
		Line = LastGotLine;
	}

	Crossed = false;
	if (ScriptPtr >= ScriptEndPtr)
	{
		End = true;
		return false;
	}

	LastGotPtr = ScriptPtr;
	LastGotLine = Line;

	// In case the generated scanner does not use marker, avoid compiler warnings.
	marker;
#include "sc_man_scanner.h"
	LastGotToken = tokens;
	return return_val;
}

//==========================================================================
//
// FScanner :: GetString
//
//==========================================================================

bool FScanner::GetString ()
{
	return ScanString (false);
}

//==========================================================================
//
// FScanner :: MustGetString
//
//==========================================================================

void FScanner::MustGetString (void)
{
	if (FScanner::GetString() == false)
	{
		ScriptError ("Missing string (unexpected end of file).");
	}
}

//==========================================================================
//
// FScanner :: MustGetStringName
//
//==========================================================================

void FScanner::MustGetStringName (const char *name)
{
	MustGetString ();
	if (Compare (name) == false)
	{
		ScriptError ("Expected '%s', got '%s'.", name, String);
	}
}

//==========================================================================
//
// FScanner :: CheckString
//
// Checks if the next token matches the specified string. Returns true if
// it does. If it doesn't, it ungets it and returns false.
//
//==========================================================================

bool FScanner::CheckString (const char *name)
{
	if (GetString ())
	{
		if (Compare (name))
		{
			return true;
		}
		UnGet ();
	}
	return false;
}

//==========================================================================
//
// FScanner :: GetToken
//
// Sets sc_Float, sc_Number, and sc_Name based on sc_TokenType.
//
//==========================================================================

bool FScanner::GetToken ()
{
	if (ScanString (true))
	{
		if (TokenType == TK_NameConst)
		{
			Name = FName(String);
		}
		else if (TokenType == TK_IntConst)
		{
			char *stopper;
			// Check for unsigned
			if (String[StringLen - 1] == 'u' || String[StringLen - 1] == 'U' ||
				String[StringLen - 2] == 'u' || String[StringLen - 2] == 'U')
			{
				TokenType = TK_UIntConst;
				BigNumber = (int64_t)strtoull(String, &stopper, 0);
				Number = (int)clamp(BigNumber, 0, UINT_MAX);
				Float = (unsigned)Number;
			}
			else
			{
				BigNumber = strtoll(String, &stopper, 0);
				Number = (int)clamp(BigNumber, INT_MIN, INT_MAX);
				Float = Number;
			}
		}
		else if (TokenType == TK_FloatConst)
		{
			char *stopper;
			Float = strtod(String, &stopper);
		}
		else if (TokenType == TK_StringConst)
		{
			StringLen = strbin(String);
		}
		return true;
	}
	return false;
}

//==========================================================================
//
// FScanner :: MustGetAnyToken
//
//==========================================================================

void FScanner::MustGetAnyToken (void)
{
	if (GetToken () == false)
	{
		ScriptError ("Missing token (unexpected end of file).");
	}
}

//==========================================================================
//
// FScanner :: TokenMustBe
//
//==========================================================================

void FScanner::TokenMustBe (int token)
{
	if (TokenType != token)
	{
		FString tok1 = TokenName(token);
		FString tok2 = TokenName(TokenType, String);
		ScriptError ("Expected %s but got %s instead.", tok1.GetChars(), tok2.GetChars());
	}
}

//==========================================================================
//
// FScanner :: MustGetToken
//
//==========================================================================

void FScanner::MustGetToken (int token)
{
	MustGetAnyToken ();
	TokenMustBe(token);
}

//==========================================================================
//
// FScanner :: CheckToken
//
// Checks if the next token matches the specified token. Returns true if
// it does. If it doesn't, it ungets it and returns false.
//
//==========================================================================

bool FScanner::CheckToken (int token)
{
	if (GetToken ())
	{
		if (TokenType == token)
		{
			return true;
		}
		UnGet ();
	}
	return false;
}

//==========================================================================
//
// FScanner :: GetNumber
//
//==========================================================================

bool FScanner::GetNumber ()
{
	char *stopper;

	CheckOpen();
	if (GetString())
	{
		if (strcmp (String, "MAXINT") == 0)
		{
			Number = INT_MAX;
		}
		else
		{
			BigNumber = strtoll(String, &stopper, 0);
			Number = (int)clamp(BigNumber, INT_MIN, INT_MAX);
			if (*stopper != 0)
			{
				ScriptError ("SC_GetNumber: Bad numeric constant \"%s\".", String);
			}
		}
		Float = Number;
		return true;
	}
	else
	{
		return false;
	}
}

//==========================================================================
//
// FScanner :: MustGetNumber
//
//==========================================================================

void FScanner::MustGetNumber ()
{
	if (GetNumber() == false)
	{
		ScriptError ("Missing integer (unexpected end of file).");
	}
}

//==========================================================================
//
// FScanner :: CheckNumber
//
// similar to GetNumber but ungets the token if it isn't a number 
// and does not print an error
//
//==========================================================================

bool FScanner::CheckNumber ()
{
	char *stopper;

	if (GetString())
	{
		if (String[0] == 0)
		{
			UnGet();
			return false;
		}
		else if (strcmp (String, "MAXINT") == 0)
		{
			BigNumber = INT64_MAX;
			Number = INT_MAX;
		}
		else
		{
			BigNumber = strtoll (String, &stopper, 0);
			Number = (int)clamp(BigNumber, INT_MIN, INT_MAX);
			if (*stopper != 0)
			{
				UnGet();
				return false;
			}
		}
		Float = Number;
		return true;
	}
	else
	{
		return false;
	}
}

//==========================================================================
//
// FScanner :: CheckFloat
//
// [GRB] Same as SC_CheckNumber, only for floats
//
//==========================================================================

bool FScanner::CheckFloat ()
{
	char *stopper;

	if (GetString())
	{
		if (String[0] == 0)
		{
			UnGet();
			return false;
		}
	
		Float = strtod (String, &stopper);
		if (*stopper != 0)
		{
			UnGet();
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}
}


//==========================================================================
//
// FScanner :: GetFloat
//
//==========================================================================

bool FScanner::GetFloat ()
{
	char *stopper;

	CheckOpen ();
	if (GetString())
	{
		Float = strtod (String, &stopper);
		if (*stopper != 0)
		{
			ScriptError ("SC_GetFloat: Bad numeric constant \"%s\".", String);
		}
		Number = (int)Float;
		return true;
	}
	else
	{
		return false;
	}
}

//==========================================================================
//
// FScanner :: MustGetFloat
//
//==========================================================================

void FScanner::MustGetFloat ()
{
	if (GetFloat() == false)
	{
		ScriptError ("Missing floating-point number (unexpected end of file).");
	}
}

//==========================================================================
//
// FScanner :: UnGet
//
// Assumes there is a valid string in String.
//
//==========================================================================

void FScanner::UnGet ()
{
	AlreadyGot = true;
	AlreadyGotLine = LastGotLine;	// in case of an error we want the line of the last token.
}

//==========================================================================
//
// FScanner :: MatchString
//
// Returns the index of the first match to String from the passed
// array of strings, or -1 if not found.
//
//==========================================================================

int FScanner::MatchString (const char * const *strings, size_t stride)
{
	int i;

	assert(stride % sizeof(const char*) == 0);

	stride /= sizeof(const char*);

	for (i = 0; *strings != NULL; i++)
	{
		if (Compare (*strings))
		{
			return i;
		}
		strings += stride;
	}
	return -1;
}

//==========================================================================
//
// FScanner :: MustMatchString
//
//==========================================================================

int FScanner::MustMatchString (const char * const *strings, size_t stride)
{
	int i;

	i = MatchString (strings, stride);
	if (i == -1)
	{
		ScriptError ("Unknown keyword '%s'", String);
	}
	return i;
}

//==========================================================================
//
// FScanner :: Compare
//
//==========================================================================

bool FScanner::Compare (const char *text)
{
	return (stricmp (text, String) == 0);
}

//==========================================================================
//
// FScanner :: TokenName
//
// Returns the name of a token.
//
//==========================================================================

FString FScanner::TokenName (int token, const char *string)
{
	static const char *const names[] =
	{
#define xx(sym,str)		str,
#include "sc_man_tokens.h"
	};

	FString work;

	if (token > ' ' && token < 256)
	{
		work = '\'';
		work += token;
		work += '\'';
	}
	else if (token >= TK_Identifier && token < TK_LastToken)
	{
		work = names[token - TK_Identifier];
		if (string != NULL && token >= TK_Identifier && token <= TK_FloatConst)
		{
			work += ' ';
			char quote = (token == TK_StringConst) ? '"' : '\'';
			work += quote;
			work += string;
			work += quote;
		}
	}
	else
	{
		FString work;
		work.Format ("Unknown(%d)", token);
		return work;
	}
	return work;
}

//==========================================================================
//
// FScanner::GetMessageLine
//
//==========================================================================

int FScanner::GetMessageLine()
{
	return AlreadyGot? AlreadyGotLine : Line;
}

//==========================================================================
//
// FScanner::ScriptError
//
//==========================================================================

void FScanner::ScriptError (const char *message, ...)
{
	FString composed;

	if (message == NULL)
	{
		composed = "Bad syntax.";
	}
	else
	{
		va_list arglist;
		va_start (arglist, message);
		composed.VFormat (message, arglist);
		va_end (arglist);
	}

	I_Error ("Script error, \"%s\" line %d:\n%s\n", ScriptName.GetChars(),
		AlreadyGot? AlreadyGotLine : Line, composed.GetChars());
}

//==========================================================================
//
// FScanner::ScriptMessage
//
//==========================================================================

void FScanner::ScriptMessage (const char *message, ...)
{
	FString composed;

	if (message == NULL)
	{
		composed = "Bad syntax.";
	}
	else
	{
		va_list arglist;
		va_start (arglist, message);
		composed.VFormat (message, arglist);
		va_end (arglist);
	}

	Printf (TEXTCOLOR_RED "Script error, \"%s\" line %d:\n" TEXTCOLOR_RED "%s\n", ScriptName.GetChars(),
		AlreadyGot? AlreadyGotLine : Line, composed.GetChars());
}

//==========================================================================
//
// FScanner :: CheckOpen
//
//==========================================================================

void FScanner::CheckOpen()
{
	if (ScriptOpen == false)
	{
		I_Error ("SC_ call before SC_Open().");
	}
}


//==========================================================================
//
// ParseHex
//
//==========================================================================

int ParseHex(const char* hex)
{
		const char* str;
	int num;

	num = 0;
	str = hex;

	while (*str)
	{
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str - '0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str - 'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str - 'A';
		else {
			Printf("Bad hex number: %s\n", hex);
			return 0;
		}
		str++;
	}

	return num;
}

 