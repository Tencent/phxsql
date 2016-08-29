// Read an INI file into easy-to-access name/value pairs.

// inih and INIReader are released under the New BSD license (see LICENSE.txt).
// Go to the project home page for more info:
//
// https://github.com/benhoyt/inih

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include "../ini.h"
#include "phxcomm/INIReader.h"

using std::string;

INIReader::INIReader() {
}

INIReader::INIReader(const string &filename) {
    _error = ini_parse(filename.c_str(), ValueHandler, this);
}

int INIReader::ReadFile(const std::string &filename) {
    return ini_parse(filename.c_str(), ValueHandler, this);
}

int INIReader::ParseError() {
    return _error;
}

const std::map<std::string, std::map<std::string, std::string> > & INIReader::GetSectionList() const {
    return _values;
}

string INIReader::Get(const string &section, const string &name, const string &default_value) {
    std::map<std::string, std::map<std::string, std::string> >::iterator f_section = _values.find(section);
    if (f_section != _values.end()) {
        std::map<std::string, std::string>::iterator f_name = f_section->second.find(name);
        if (f_name != f_section->second.end()) {
            return f_name->second;
        }
    }

    return default_value;
}

long INIReader::GetInteger(const string &section, const string &name, const long &default_value) {
    string valstr = Get(section, name, "");
    const char* value = valstr.c_str();
    char* end;
    // This parses "1234" (decimal) and also "0x4D2" (hex)
    long n = strtol(value, &end, 0);
    return end > value ? n : default_value;
}

int INIReader::ValueHandler(void* user, const char* section, const char* name, const char* value) {
    INIReader* reader = (INIReader*) user;
    reader->_values[section][name] = value;
    return 1;
}

