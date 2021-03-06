#include <map>
#include <array>
#include <bitset>
#include <limits>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <exception>
#include <unordered_map>

#include "utf8tools.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////

class CException : public exception {
public:
	explicit CException( const string& message ) :
		msg( message )
	{
	}

	virtual const char* what() const noexcept override
	{
		return msg.c_str();
	}

private:
	string msg;
};

///////////////////////////////////////////////////////////////////////////////

const string ReplacementsCP1251 =
	"                                "
	" !      ()  ,-. 0123456789:;   ?"
	" abcdefghijklmnopqrstuvwxyz     "
	" abcdefghijklmnopqrstuvwxyz     "
	"  ,  .                --        "
	"        \xE5    -          \xE5       "
	"\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF"
	"\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF"
	"\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF"
	"\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF";

///////////////////////////////////////////////////////////////////////////////

inline bool IsCharAlphaOrDigit( const char c )
{
	/*
		"                                "
		"                0123456789      "
		" ABCDEFGHIJKLMNOPQRSTUVWXYZ     "
		" abcdefghijklmnopqrstuvwxyz     "
		"                                "
		"�                               "
		"��������������������������������"
		"��������������������������������"
	*/
	static const vector<bool> alphasAndDigits{
		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 1,1,1,1,1,1,1,1, 1,1,0,0,0,0,0,0,
		0,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,0,0,0,0,0,
		0,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,0,0,0,0,0,
		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1
	};
	return alphasAndDigits[static_cast<unsigned char>( c )];
}

///////////////////////////////////////////////////////////////////////////////

void TextReplace( string& text, const string& replacements )
{
	for( char& c : text ) {
		c = replacements[static_cast<unsigned char>( c )];
	}
}

///////////////////////////////////////////////////////////////////////////////

void PrepareTextFile( const string& sourceFilename, const string& destFilename )
{
	ifstream src( sourceFilename );
	ofstream dest( destFilename, ios_base::out | ios_base::binary );

	if( !src.good() ) {
		throw CException( "Cannot read text file `" + sourceFilename + "`." );
	}

	string line;
	while( src.good() ) {
		getline( src, line );
		if( !ConvertUtf8ToWindows1251( line, ' ' ) ) {
			throw CException( "Cannot read as valid UTF-8 text file `" + sourceFilename + "` ." );
		}
		TextReplace( line, ReplacementsCP1251 );
		line += '\n';
		dest.write( line.data(), line.length() );
	}
}

///////////////////////////////////////////////////////////////////////////////

bool System( const string& arg )
{
	if( system( nullptr ) == 0 ) {
		throw CException( " Function `int system( const char* )` is not supported." );
	}

#ifdef _WIN32
	const int returnCode = system( ( "\"" + arg + "\"" ).c_str() );
#else
	const int returnCode = system( arg.c_str() );
#endif

	return ( returnCode == 0 );
}

///////////////////////////////////////////////////////////////////////////////

vector<string> SplitString( const string& str, const char* delimiters = " \t\r",
	bool preserveEmptyStrings = false )
{
	vector<string> strings;
	size_t offset = 0;
	while( offset < str.length() ) {
		size_t delimiterPos = str.find_first_of( delimiters, offset );
		if( delimiterPos == string::npos ) {
			break;
		}

		const string part = str.substr( offset, delimiterPos - offset );
		if( preserveEmptyStrings || !part.empty() ) {
			strings.push_back( part );
		}

		offset = delimiterPos + 1;
	}

	const string part = str.substr( offset );
	if( preserveEmptyStrings || !part.empty() ) {
		strings.push_back( part );
	}

	return strings;
}

///////////////////////////////////////////////////////////////////////////////

class CDictionaries {
	friend class CFinder;

public:
	CDictionaries();

	bool IsEmpty() const { return levels.empty(); }
	void AddFile( const string& dictionaryFilename, size_t dictionaryIndex = 1 );
	void AddLine( const string& line, size_t dictionaryIndex = 1 );

private:
	size_t wordIndex;

	typedef basic_string<size_t> CWords;
	struct CLevel {
		unordered_map<string, size_t> WordToIndex;
		unordered_map<CWords, size_t> PrefixToDictionary;
	};
	vector<CLevel> levels;
};

CDictionaries::CDictionaries() :
	wordIndex( 0 )
{
}

void CDictionaries::AddFile( const string& dictionaryFilename, size_t dictionaryIndex )
{
	ifstream dictionaryFile( dictionaryFilename );
	if( !dictionaryFile.good() ) {
		throw CException( "Cannot read dictionary `" + dictionaryFilename + "`." );
	}
	string line;
	while( dictionaryFile.good() ) {
		getline( dictionaryFile, line );
		ConvertUtf8ToWindows1251( line );
		TextReplace( line, ReplacementsCP1251 );
		AddLine( line, dictionaryIndex );
	}
}

void CDictionaries::AddLine( const string& line, size_t dictionaryIndex )
{
	if( dictionaryIndex == 0 ) {
		throw logic_error( "CDictionaries::AddLine invalid dictionaryIndex" );
	}

	vector<string> strings = SplitString( line );
	if( strings.empty() ) {
		return;
	}

	if( levels.size() < strings.size() ) {
		levels.resize( strings.size() );
	}

	CWords words;
	words.reserve( strings.size() );
	for( size_t i = 0; i < strings.size(); i++ ) {
		++wordIndex;

		auto p1 = levels[i].WordToIndex.insert( make_pair( strings[i], wordIndex ) );
		words.push_back( p1.first->second );

		auto p2 = levels[i].PrefixToDictionary.insert( make_pair( words, 0 ) );
		if( i == strings.size() - 1 ) {
			if( p2.first->second != 0 && p2.first->second != dictionaryIndex ) {
				throw CException( "Duplicates were found in the dictionaries." );
			}
			p2.first->second = dictionaryIndex;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

class CFinder {
public:
	struct CMatch {
		size_t Begin;
		size_t End;
		size_t Dictionary;

		CMatch( size_t begin, size_t end, size_t dictionary ) :
			Begin( begin ),
			End( end ),
			Dictionary( dictionary )
		{
		}
	};
	typedef vector<CMatch> CMatches;

	explicit CFinder( const CDictionaries& dictionaries );

	void Reset();
	void Push( const string& word );
	void Finish();
	const CMatches& Matches() const { return matches; }

private:
	const CDictionaries& dictionaries;
	CDictionaries::CWords words;
	size_t count;
	size_t dictionary;
	size_t wordIndex;
	CMatches matches;

	void addMatch( size_t begin, size_t end, size_t dictionary );
	bool addWord( const string& word );
	void dump();
	bool processWords();
};

CFinder::CFinder( const CDictionaries& _dictionaries ) :
	dictionaries( _dictionaries )
{
	Reset();
}

void CFinder::Reset()
{
	words.clear();
	count = 0;
	dictionary = 0;
	wordIndex = 0;
	matches.clear();
}

void CFinder::Push( const string& word )
{
	while( !addWord( word ) ) {
		if( words.empty() ) {
			wordIndex++;
			break;
		} else if( count > 0 ) {
			dump();
		} else {
			words.erase( words.begin() );
			wordIndex++;
		}
	}
}

void CFinder::Finish()
{
	if( count > 0 ) {
		dump();
	}

	while( !words.empty() ) {
		if( processWords() && count > 0 ) {
			dump();
		} else if( !words.empty() ) {
			words.erase( words.begin() );
			wordIndex++;
		}
	}
}

void CFinder::addMatch( size_t begin, size_t end, size_t dictionary )
{
	matches.emplace_back( begin, end, dictionary );
}

bool CFinder::addWord( const string& word )
{
	const CDictionaries::CLevel& level = dictionaries.levels[words.size()];
	auto wordIndex = level.WordToIndex.find( word );
	if( wordIndex != level.WordToIndex.end() ) {
		words.push_back( wordIndex->second );
		if( processWords() ) {
			return true;
		}
		words.pop_back();
	}
	return false;
}

void CFinder::dump()
{
	if( count == 0 || words.size() < count ) {
		throw logic_error( "CFinder::dump()" );
	}
	addMatch( wordIndex, wordIndex + count, dictionary );
	words.erase( words.begin(), words.begin() + count );
	wordIndex += count;
	count = 0;
	dictionary = 0;
}

bool CFinder::processWords()
{
	const CDictionaries::CLevel& level = dictionaries.levels[words.size() - 1];
	auto prefixIndex = level.PrefixToDictionary.find( words );
	if( prefixIndex != level.PrefixToDictionary.end() ) {
		if( prefixIndex->second > 0 ) {
			count = words.size();
			dictionary = prefixIndex->second;
			if( count == dictionaries.levels.size() ) {
				dump();
			}
		}
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

struct CInterval {
	size_t Begin;
	size_t End;

	CInterval() :
		Begin( numeric_limits<size_t>::max() ),
		End( 0 )
	{
	}

	CInterval( size_t begin, size_t end ) :
		Begin( begin ),
		End( end )
	{
	}

	bool Defined() const
	{
		return ( Begin < End );
	}

	size_t Length() const
	{
		return ( End - Begin );
	}

	bool HasNoIntersection( const CInterval& another ) const
	{
		return ( End <= another.Begin || another.End <= Begin );
	}

	void Offset( size_t offset )
	{
		Begin += offset;
		End += offset;
	}
};

enum TNamedEntityType {
	NET_None,
	NET_Org,
	NET_Person,
	NET_Location
};

const char* NamedEntityTypeText( TNamedEntityType type )
{
	switch( type ) {
		case NET_Org:
			return "Org";
		case NET_Person:
			return "Person";
		case NET_Location:
			return "Location";
		default:
			break;
	}
	return "None";
}

struct CNamedEntity : public CInterval {
	TNamedEntityType Type;

	CNamedEntity() :
		Type( NET_None )
	{
	}

	bool Defined() const
	{
		return ( Type != NET_None && CInterval::Defined() );
	}

	bool SetType( const string& type )
	{
		Type = NET_None;
		if( type == "Person" ) {
			Type = NET_Person;
		} else if( type == "Org" || type == "LocOrg" ) {
			Type = NET_Org;
		} else if( type == "Location" ) {
			Type = NET_Location;
		}
		return ( Type != NET_None );
	}
};

class CNamedEntities : public vector<CNamedEntity> {
public:
	CNamedEntities()
	{
	}

	void Read( const string& baseFilename );
	void Sort();
	bool Check() const;
};

void CNamedEntities::Read( const string& baseFilename )
{
	clear();

	const string spansFilename = baseFilename + ".spans";
	const string objectsFilename = baseFilename + ".objects";

	ifstream spans( spansFilename );
	if( !spans.good() ) {
		throw CException( "File `" + spansFilename + "` not found." );
	}
	ifstream objects( objectsFilename );
	if( !objects.good() ) {
		throw CException( "File `" + objectsFilename + "` not found." );
	}

	// read spans
	map<size_t, pair<size_t, size_t>> spansBeginEnd;
	while( spans.good() ) {
		string ignore;
		size_t id;
		size_t offset;
		size_t length;
		spans >> id >> ignore >> offset >> length;
		if( spans.good() ) {
			spansBeginEnd.insert( make_pair( id, make_pair( offset, offset + length ) ) );
			spans.ignore( numeric_limits<streamsize>::max(), '\n' );
		} else if( spans.eof() ) {
			spans.clear( ios::eofbit );
		}
	}
	if( spans.fail() ) {
		throw CException( "Bad spans file `" + spansFilename + "` format." );
	}

	// read objects
	while( objects.good() ) {
		objects.ignore( numeric_limits<streamsize>::max(), ' ' );
		if( objects.eof() ) {
			objects.clear( ios_base::eofbit );
			break;
		}

		string type;
		objects >> type;

		CNamedEntity entity;
		if( entity.SetType( type ) ) {
			while( objects.good() ) {
				objects.ignore( numeric_limits<streamsize>::max(), ' ' );
				if( isdigit( objects.peek() ) ) {
					size_t id;
					objects >> id;
					auto span = spansBeginEnd.find( id );
					if( span == spansBeginEnd.end() ) {
						objects.setstate( ios::failbit );
						break;
					}
					entity.Begin = min( entity.Begin, span->second.first );
					entity.End = max( entity.End, span->second.second );
				} else {
					break;
				}
			}
			if( entity.Defined() ) {
				push_back( entity );
			} else {
				objects.setstate( ios::failbit );
			}
		}
		if( objects.good() ) {
			objects.ignore( numeric_limits<streamsize>::max(), '\n' );
		}
	}
	Sort();
	if( !Check() ) {
		objects.setstate( ios::failbit );
	}
	if( objects.fail() ) {
		throw CException( "Bad objects file `" + objectsFilename + "` format." );
	}
	/*for( const CNamedEntity& entity : *this ) {
		cout << NamedEntityTypeText( entity.Type ) << " " << entity.Begin << " " << entity.End << endl;
	}*/
}

void CNamedEntities::Sort()
{
	struct Predicate {
		bool operator()( const CNamedEntity& ne1, const CNamedEntity& ne2 )
		{
			return ( ne1.Begin < ne2.Begin
				|| ( ne1.Begin == ne2.Begin && ne1.End < ne2.End ) );
		}
	};
	sort( begin(), end(), Predicate() );

	CNamedEntities tmp = move( *this );
	for( const CNamedEntity& entity : tmp ) {
		if( empty() || back().HasNoIntersection( entity ) ) {
			push_back( entity );
		} else if( back().Length() < entity.Length() ) {
			back() = entity;
		}
	}
}

bool CNamedEntities::Check() const
{
	size_t offset = 0;
	for( const CNamedEntity& entity : *this ) {
		if( entity.Begin < offset ) {
			return false;
		}
		offset = entity.End;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

struct CToken : public CInterval {
	string Text;
	string Lexem;
};

///////////////////////////////////////////////////////////////////////////////

class CTokens : public vector<CToken> {
public:
	CTokens()
	{
	}

	void Parse( const string& stemedFile );

	void Load( const string& filename );
	void Save( const string& filename ) const;

private:
	static void restorePlainText( string& text );
};

void CTokens::Load( const string& filename )
{
	clear();
	ifstream input( filename );
	while( input.good() ) {
		CToken token;
		input >> token.Begin >> token.End >> ws;
		getline( input, token.Text, '\t' );
		getline( input, token.Lexem );
		if( input.fail() ) {
			throw CException( "Bad todua-tokens file `" + filename + "` format." );
		}
		push_back( token );
		input >> ws;
	}
}

void CTokens::Save( const string& filename ) const
{
	ofstream output( filename );
	for( const CToken& token : *this ) {
		output << token.Begin << "\t" << token.End << "\t"
			<< token.Text << "\t" << token.Lexem << endl;
	}
}

void CTokens::restorePlainText( string& text )
{
	size_t from = 0;
	size_t to = 0;
	while( from < text.length() ) {
		if( text[from] == '_' ) {
			text[to] = ' ';
		} else if( text[from] == '\\' ) {
			from++;
			if( text[from] == 'n' || text[from] == 'r' ) {
				text[to] = '\n';
			} else {
				text[to] = text[from];
			}
		} else {
			text[to] = text[from];
		}
		to++;
		from++;
	}
	if( !text.empty() ) {
		text.erase( to );
	}
}

void CTokens::Parse( const string& filename )
{
	clear();
	ifstream input( filename );
	size_t offset = 0;
	while( input.good() ) {
		string line;
		getline( input, line );
		if( input.eof() ) {
			break;
		}
		if( !line.empty() && line.back() == '\r' ) {
			line.pop_back();
		}

		const size_t pos = line.find( '{' );
		if( pos == string::npos ) {
			restorePlainText( line );
			size_t lineOffset = 0;
			while( lineOffset < line.length() ) {
				char c = line[lineOffset];
				if( IsCharAlphaOrDigit( c ) ) {
					const size_t beginPos = lineOffset;
					while( lineOffset < line.length() && IsCharAlphaOrDigit( line[lineOffset] ) ) {
						lineOffset++;
					}
					CToken token;
					token.Text = line.substr( beginPos, lineOffset - beginPos );
					token.Lexem = token.Text;
					if( token.Lexem.find_first_not_of( "0123456789" ) == string::npos ) {
						token.Lexem = "#";
					}
					token.Begin = offset + beginPos;
					token.End = offset + lineOffset;
					push_back( token );
				} else if( c == ' ' || c == '\n' ) {
					lineOffset++;
					continue;
				} else {
					CToken token;
					token.Text = string( 1, c );
					token.Lexem = token.Text;
					token.Begin = offset + lineOffset;
					lineOffset++;
					token.End = offset + lineOffset;
					push_back( token );
				}
			}
			offset += lineOffset;
		} else {
			CToken token;
			token.Text = line.substr( 0, pos );
			const size_t startPos = pos + 1;
			const size_t endPos = line.find_first_of( "?|}", startPos );
			if( endPos == string::npos ) {
				throw CException( "Bad file `" + filename + "` format." );
			}
			token.Lexem = line.substr( startPos, endPos - startPos );
			token.Begin = offset;
			offset += token.Text.length();
			token.End = offset;
			push_back( token );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

const char* InternalNamedEntityTypeText( TNamedEntityType type )
{
	switch( type ) {
		case NET_Org:
			return "$O";
		case NET_Person:
			return "$P";
		case NET_Location:
			return "$L";
		default:
			break;
	}
	throw logic_error( "InternalNamedEntityTypeText" );
}

void SetNamedEntitiyTokenTypes( const CNamedEntities& namedEntities, CTokens& tokens )
{
	CTokens tmp = move( tokens );

	auto token = tmp.cbegin();
	for( const CNamedEntity& entity : namedEntities ) {
		if( token == tmp.cend() ) {
			throw CException( "Objects does not matched with tokens." );
		}

		while( token != tmp.cend() && entity.HasNoIntersection( *token ) ) {
			tokens.push_back( *token );
			++token;
		}
		if( token != tmp.cend() ) {
			tokens.push_back( *token );
			tokens.back().Lexem = InternalNamedEntityTypeText( entity.Type );
			for( ++token; token != tmp.cend() && !entity.HasNoIntersection( *token ); ++token ) {
				tokens.back().Text += " " + token->Text;
				tokens.back().End = token->End;
			}
		}
	}
	while( token != tmp.cend() ) {
		tokens.push_back( *token );
		++token;
	}
}

///////////////////////////////////////////////////////////////////////////////

void ProcessTokensByDictionaries( const CDictionaries& dictionaries, CTokens& tokens )
{
	if( dictionaries.IsEmpty() ) {
		return;
	}

	CTokens tmp = move( tokens );

	CFinder finder( dictionaries );
	for( const CToken& token : tmp ) {
		finder.Push( token.Lexem );
	}
	finder.Finish();

	size_t tokenIndex = 0;
	for( const CFinder::CMatch& match : finder.Matches() ) {
		for( ; tokenIndex <= match.Begin; tokenIndex++ ) {
			tokens.push_back( tmp[tokenIndex] );
		}
		tokens.back().Lexem = "@" + to_string( match.Dictionary );
		for( ; tokenIndex < match.End; tokenIndex++ ) {
			tokens.back().Text += " " + tmp[tokenIndex].Text;
			tokens.back().End = tmp[tokenIndex].End;
		}
	}
	for( ; tokenIndex < tmp.size(); tokenIndex++ ) {
		tokens.push_back( tmp[tokenIndex] );
	}
}

///////////////////////////////////////////////////////////////////////////////

vector<string> MakeAllVariants( const string& text )
{
	vector<string> variants;
	const size_t beginPos = text.find( '[' );
	if( beginPos == string::npos ) {
		variants.push_back( text );
		return variants;
	}

	const size_t endPos = text.find_first_of( ']', beginPos );
	if( endPos == string::npos ) {
		return vector<string>(); // error
	}

	const string prefix = text.substr( 0, beginPos );
	const string middle = text.substr( beginPos + 1, endPos - ( beginPos + 1 ) );
	const string suffix = text.substr( endPos + 1 );

	vector<string> middles = SplitString( middle, "|", true /* preserveEmptyStrings */ );
	if( middles.size() == 1 ) {
		middles.push_back( "" );
	}
	vector<string> suffixes = MakeAllVariants( suffix );

	for( const string& suf : suffixes ) {
		for( const string& mid : middles ) {
			variants.push_back( prefix + mid + suf );
		}
	}
	return variants;
}

///////////////////////////////////////////////////////////////////////////////

class CUtf8TextFile {
public:
	explicit CUtf8TextFile( const string& filename );

	string Text( CInterval interval ) const;

private:
	typedef array<char, 4> CChar;
	vector<CChar> chars;
};

CUtf8TextFile::CUtf8TextFile( const string& filename )
{
	ifstream input( filename );
	if( !input.good() ) {
		throw CException( "File `" + filename + "` not found." );
	}

	do {
		string line;
		getline( input, line );
		if( !line.empty() && line.back() == '\r' ) {
			line.pop_back();
		}

		size_t index = 0;
		for( char c : line ) {
			const unsigned char uc = static_cast<unsigned char>( c );
			if( uc >= 128 && uc < 192 ) {
				chars.back()[++index] = c;
			} else {
				index = 0;
				chars.push_back( { c, '\0', '\0', '\0' } );
			}
		}
		chars.push_back( { '\n', '\0', '\0', '\0' } );
	} while( input.good() );
}

string CUtf8TextFile::Text( CInterval interval ) const
{
	if( !interval.Defined() ) {
		throw logic_error( "CUtf8TextFile::Text() undefined interval" );
	}
	string text;
	for( size_t i = interval.Begin; i < interval.End; i++ ) {
		text.push_back( chars[i][0] );
		for( size_t j = 1; j < chars[i].size() && chars[i][j] != '\0'; j++ ) {
			text.push_back( chars[i][j] );
		}
	}
	return text;
}

///////////////////////////////////////////////////////////////////////////////

struct COccupation {
	CInterval Who;
	CInterval Where;
	CInterval Job;

	bool Check() const
	{
		return Who.Defined() && Where.Defined();
	}
	void Write( ostream& output, const CUtf8TextFile& textFle ) const
	{
		if( !Check() ) {
			throw logic_error( "bad occupation" );
		}
		output << "Occupation" << endl;
		output << "who:" << textFle.Text( Who ) << endl;
		if( Where.Defined() ) {
			output << "where:" << textFle.Text( Where ) << endl;
		}
		if( Job.Defined() ) {
			output << "job:" << textFle.Text( Job ) << endl;
		}
	}
};

///////////////////////////////////////////////////////////////////////////////

class CVariantDefs {
public:
	CVariantDefs();

	size_t AddVariant( string& variant );
	COccupation Occupation( const size_t variantIndex,
		CTokens::const_iterator firstMatchedToken ) const;

private:
	vector<COccupation> variants;

	bool addToken( string& token, CInterval interval );
	bool addInterval( CInterval& dest, CInterval newInterval ) const;
};

CVariantDefs::CVariantDefs()
{
}

bool CVariantDefs::addInterval( CInterval& dest, CInterval interval ) const
{
	if( !dest.Defined() ) {
		dest = interval;
	} else if( dest.End == interval.Begin ) {
		dest.End = interval.End;
	} else {
		return false;
	}
	return true;
}

bool CVariantDefs::addToken( string& token, CInterval interval )
{
	COccupation& occupation = variants.back();
	if( token == "$P" ) {
		return addInterval( occupation.Who, interval );
	}

	if( token == "$O" || token == "$L" ) {
		return addInterval( occupation.Where, interval );
	}

	const size_t tildePos = token.find_first_of( "~" );
	if( tildePos == string::npos ) {
		return true;
	}
	if( tildePos == 0 ) {
		return false;
	}

	const string afterTidle = token.substr( tildePos );
	token = token.substr( 0, tildePos );

	if( afterTidle == "~job" ) {
		return addInterval( occupation.Job, interval );
	} else if( afterTidle == "~where" ) {
		return addInterval( occupation.Where, interval );
	} else if( afterTidle == "~who" ) {
		return addInterval( occupation.Who, interval );
	}

	return ( afterTidle == "~" );
}

size_t CVariantDefs::AddVariant( string& variant )
{
	variants.push_back( COccupation() );
	vector<string> tokens = SplitString( variant );
	CInterval interval( 0, 1 );
	for( string& token : tokens ) {
		if( !addToken( token, interval ) ) {
			throw CException( "Invalid format" );
		}
		interval.Offset( 1 );
	}
	if( !variants.back().Check() ) {
		throw CException( "Invalid format" );
	}
	variant.clear();
	for( const string& token : tokens ) {
		variant += token + " ";
	}
	return variants.size();
}

COccupation CVariantDefs::Occupation( const size_t variantIndex,
	CTokens::const_iterator firstMatchedToken ) const
{
	COccupation occupation = variants[variantIndex - 1];
	if( occupation.Who.Defined() ) {
		occupation.Who.Begin = ( firstMatchedToken + occupation.Who.Begin )->Begin;
		occupation.Who.End = ( firstMatchedToken + occupation.Who.End - 1 )->End;
	}
	if( occupation.Where.Defined() ) {
		occupation.Where.Begin = ( firstMatchedToken + occupation.Where.Begin )->Begin;
		occupation.Where.End = ( firstMatchedToken + occupation.Where.End - 1 )->End;
	}
	if( occupation.Job.Defined() ) {
		occupation.Job.Begin = ( firstMatchedToken + occupation.Job.Begin )->Begin;
		occupation.Job.End = ( firstMatchedToken + occupation.Job.End - 1 )->End;
	}
	if( !occupation.Check() ) {
		throw logic_error( "CVariantDefs::Occupation" );
	}
	return occupation;
}

///////////////////////////////////////////////////////////////////////////////

void LoadTemplates( const string& templatesFilename,
	CDictionaries& dictionaries, CVariantDefs& variantDefs )
{
	ifstream templatesFile( templatesFilename );
	if( !templatesFile.good() ) {
		throw CException( "Cannot read templates `" + templatesFilename + "`." );
	}
	size_t lineNumber = 0;
	do {
		string line;
		++lineNumber;
		getline( templatesFile, line );
		ConvertUtf8ToWindows1251( line );
		vector<string> variants = MakeAllVariants( line );
		if( variants.empty() ) {
			throw CException( "Invalid templates `" + templatesFilename + "`"
				" line " + to_string( lineNumber ) + "." );
		}
		if( variants.size() == 1 && variants.front().empty() ) {
			variants.clear();
		}

		for( string& variant : variants ) {
			const size_t index = variantDefs.AddVariant( variant );
			dictionaries.AddLine( variant, index );
		}
	} while( templatesFile.good() );
}

///////////////////////////////////////////////////////////////////////////////

const char* const MystemExeName = "mystem";

string GetMystemPath( const string& exePath )
{
	const size_t pos = exePath.find_last_of( "\\/" );
	if( pos != string::npos ) {
		return exePath.substr( 0, pos + 1 ) + MystemExeName;
	}
	return MystemExeName;
}

///////////////////////////////////////////////////////////////////////////////

void ParseTokens( const string& baseFilename, CTokens& tokens,
	const string& mystemPath = MystemExeName )
{
	// prepare file
	const string tempFilename1 = "temp1.txt";
	const string tempFilename2 = "temp2.txt";
	const string mystrem = "\"" + mystemPath + "\" -ncwd --eng-gr -e cp1251 "
		+ tempFilename1 + " " + tempFilename2;
	PrepareTextFile( baseFilename + ".txt", tempFilename1 );
	if( !System( mystrem ) ) {
		throw CException( "Cannot run `mystem`." );
	}

	// extract tokens
	tokens.Parse( tempFilename2 );

#ifdef _WIN32
	System( ( "DEL " + tempFilename1 + " " + tempFilename2 ).c_str() );
#else
	System( ( "rm " + tempFilename1 + " " + tempFilename2 ).c_str() );
#endif
}

///////////////////////////////////////////////////////////////////////////////

class COccupations : public vector<COccupation> {
public:
	void Fill( const CTokens& tokens,
		const CDictionaries& templates, const CVariantDefs& variantDefs );
	void Write( const string& baseFilename ) const;
};

void COccupations::Fill( const CTokens& tokens,
	const CDictionaries& templates, const CVariantDefs& variantDefs )
{
	CFinder finder( templates );
	for( const CToken& token : tokens ) {
		finder.Push( token.Lexem );
	}
	finder.Finish();

	for( const CFinder::CMatch& match : finder.Matches() ) {
		// add occupation
		push_back( variantDefs.Occupation( match.Dictionary, tokens.cbegin() + match.Begin ) );
	}
}

void COccupations::Write( const string& baseFilename ) const
{
	CUtf8TextFile sourceFile( baseFilename + ".txt" );
	ofstream output( baseFilename + ".task3" );
	for( const COccupation& occupation : *this ) {
		occupation.Write( output, sourceFile );
		output << endl;
	}
}

///////////////////////////////////////////////////////////////////////////////

int main( int argc, const char* argv[] )
{
	try {
#ifdef _WIN32
		system( "chcp 1251" );
#endif
		if( argc < 3 ) {
			throw CException( "Too few arguments.\n"
				"Usage: occup BASE_FILENAME TEMPLATES_FILENAME [DICTIONARIES]..\n"
				"Example: occup Book_100 Templates.txt ListWork.txt ListOccupation.txt" );
		}

		// base filename (without extension)
		const string baseFilename = argv[1];
		const string templatesFilename = argv[2];
		const string toduaTokensFilename = baseFilename + ".todua-tokens";

		// templates
		CDictionaries templates;
		CVariantDefs variantDefs;
		LoadTemplates( templatesFilename, templates, variantDefs );

		// replaces
		CDictionaries dictionaries;
		for( int arg = 3; arg < argc; arg++ ) {
			dictionaries.AddFile( argv[arg], arg - 2 );
		}

		// prepare tokens
		CTokens tokens;
		tokens.Load( toduaTokensFilename );

		if( tokens.empty() ) {
			ParseTokens( baseFilename, tokens, GetMystemPath( argv[0] ) );

			// extract named entities
			CNamedEntities namedEntities;
			namedEntities.Read( baseFilename );

			// set named entity type for tokens
			SetNamedEntitiyTokenTypes( namedEntities, tokens );

			// dump token for future executions.
			tokens.Save( toduaTokensFilename );
		}

		// Normalize by dictionaries
		ProcessTokensByDictionaries( dictionaries, tokens );

		// Write result
		COccupations occupations;
		occupations.Fill( tokens, templates, variantDefs );
		occupations.Write( baseFilename );
	} catch( exception& e ) {
		cerr << "Error: " << e.what() << endl;
		return 1;
	} catch( ... ) {
		cerr << "Unknown error!" << endl;
		return 1;
	}

	return 0;
}
