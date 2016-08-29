/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#ifndef __INIREADER_H__
#define __INIREADER_H__

#include <map>
#include <string>

// Read an INI file into easy-to-access name/value pairs. (Note that I've gone
// for simplicity here rather than speed, but it should be pretty decent.)
class INIReader {
 public:
    // Construct INIReader 
    INIReader();

    // Construct INIReader and parse given filename. See ini.h for more info
    // about the parsing.
    INIReader(const std::string &filename);

    //parse the given file, See ini.h for more detail
    int ReadFile(const std::string &filename);

    const std::map<std::string, std::map<std::string, std::string> > & GetSectionList() const;

    // Return the result of ini_parse(), i.e., 0 on success, line number of
    // first error on parse error, or -1 on file open error.
    int ParseError();

    // Get a string value from INI file, returning default_value if not found.
    std::string Get(const std::string &section, const std::string &name, const std::string &default_value);

    // Get an integer (long) value from INI file, returning default_value if
    // not found or not a valid integer (decimal "1234", "-1234", or hex "0x4d2").
    long GetInteger(const std::string &section, const std::string &name, const long &default_value);
 private:
    int _error;
    std::map<std::string, std::map<std::string, std::string> > _values;
    static int ValueHandler(void* user, const char* section, const char* name, const char* value);
};

#endif  // __INIREADER_H__
