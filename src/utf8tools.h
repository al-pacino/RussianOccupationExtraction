#pragma once

#include <string>

// Convert UTF-8 text to CP1251 text (remove all non cp1251 symbols).
bool ConvertUtf8ToWindows1251( std::string& text );

// Convert UTF-8 text to CP1251 text (replace all non cp1251 symbols with replacement).
bool ConvertUtf8ToWindows1251( std::string& text, const char replacemnt );
