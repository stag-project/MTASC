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

#ifndef STAGDEER_OPERATOR_UTIL
#define STAGDEER_OPERATOR_UTIL

#include <type_traits>
#include <utility>

namespace stagdeer {
    namespace util {

        template<typename Tp , typename Up>
        auto operator$(Tp&& __M_VALUE, Up&& __M_METHOD) {
            return __M_METHOD(std::forward<Tp>(__M_VALUE));
        }

        template<typename Tp , typename Up>
        auto operator|(Tp&& __M_VALUE , Up&& __M_METHOD) {
            return __M_METHOD(std::forward<Tp>(__M_VALUE));
        }

        namespace lambda_trais {
            template<bool Tp>
            struct constraint {
                using type = int;
            };

            template<>
            struct constraint<false> {};

            template<typename Tp , typename ... Args>
            struct M_get_lambda_ret_Tp {
                using __M_ret_lmdba = std::invoke_result_t<Tp, Args ...>;
            };

            template<typename Tp , typename Up>
            struct M_is_retTp {
                static constexpr bool __is_M_ret_Tp = std::is_same_v<Tp, Up>;
            };
        }
    }
}

#endif