/*
	Tencent is pleased to support the open source community by making PhxSQL available.
	Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
	Licensed under the GNU General Public License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
	
	https://opensource.org/licenses/GPL-2.0
	
	Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#pragma once

#include "INIReader.h"

namespace phxsql {

class PhxBaseConfig : public INIReader {
 public:
    PhxBaseConfig();

    virtual ~PhxBaseConfig() {
    }

    //the file name of config file, including path prefix
    //filename describes the full path information of the config file
    int ReadFileWithConfigDirPath(const std::string &filename);

    //the file name of config file, not including path prefix
    //full path will be formed from s_default_path + filename
    int ReadFile(const std::string &filename);

    //get string from section, which matches the section and  name(key),
    //it will be filled with the default_value if not exists
    std::string Get(const std::string &section, const std::string &name, const std::string &default_value);

    //get string from section, which matches the section and  name(key),
    //it will be filled with the default_value if not exists
    int GetInteger(const std::string &section, const std::string &name, const int &default_value);

    //set the default path, where config files are located
    static void SetDefaultPath(const char *config_path);

    //get the default path, where config files are located
    static std::string GetDefaultPath();

    //get local ip
    static std::string GetInnerIP();

 protected:
    //get the config file
    virtual void ReadConfig() = 0;

    static std::string GetBasePath();

    static char *InitInnerIP();

    //real read the config to initialize
    int RealReadFile(const std::string &filename);

 private:
    std::string filename_;
    static std::string s_default_path;
    static std::string inner_ip_;
};

}
