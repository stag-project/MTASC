/**
    Copyright [stag-project] [stag-project]

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#ifndef STAGDEER_METHOED
#define STAGDEER_METHOED

#include <string>
namespace stagdeer {
    enum class httpMethod {
        GET , POST
    };

    struct methodTool {

        static std::string methodToStr(stagdeer::httpMethod method__) {
            switch (method__) {
                case stagdeer::httpMethod::GET : {return "GET"; break;}
                case stagdeer::httpMethod::POST : {return "POST"; break;}
                default: {return "GET"; break;};
            };
        }

    };
}

#endif