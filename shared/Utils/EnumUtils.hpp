#pragma once

#include "System/Array.hpp"
#include "System/String.hpp"
#include "System/StringComparison.hpp"
#include "System/Type.hpp"
#include "beatsaber-hook/shared/members.hpp"
#include <string>

namespace MultiplayerCore::Utils {
    struct EnumUtils {
        template <class T>
        static StringW GetEnumName(T value) {
            auto enumType = static_cast<System::Type*>(i2c::cs_type_of<T>());
            return enumType->GetEnumName(reinterpret_cast<System::Object*>(
                i2c::functions::value_box(i2c::class_of<T>(), &value)));
        }

        template <class T>
        static T GetEnumValue(const std::string& name, bool ignoreCase = false) {
            auto enumType = static_cast<System::Type*>(i2c::cs_type_of<T>());
            auto names = enumType->GetEnumNames();
            auto comparison = ignoreCase ? System::StringComparison::OrdinalIgnoreCase : System::StringComparison::Ordinal;
            for (std::size_t index = 0; index < names.size(); ++index) {
                if (!System::String::Equals(names[index], name, comparison)) continue;
                auto value = enumType->GetEnumValues()->GetValue(static_cast<int>(index));
                return *static_cast<T*>(i2c::functions::object_unbox(reinterpret_cast<Il2CppObject*>(value)));
            }
            return T(0);
        }
    };
}
