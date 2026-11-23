#pragma once

#include "scotland2/shared/loader.hpp"
#include "beatsaber-hook/shared/hooking.hpp"
#include "paper2_scotland2/shared/logger.hpp"
#include <vector>

namespace MultiplayerCore {
    class Hooking {
    private:
        inline static std::vector<void (*)()> installFuncs;

    public:
        static void AddInstallFunc(void (*installFunc)()) {
            installFuncs.push_back(installFunc);
        }

        static inline void InstallHooks() {
            for (auto& func : installFuncs) func();
        }
    };

}

#define AUTO_INSTALL_PATCH(name_) \
    struct Auto_Patch_##name_ {                                                 \
        Auto_Patch_##name_() {                                                  \
            ::MultiplayerCore::Hooking::AddInstallFunc(Patch_##name_);          \
        }                                                                       \
    };                                                                          \
    static Auto_Patch_##name_ Auto_Patch_Instance_##name_

#define HOOK_AUTO_INSTALL_ORIG(name_)                                                                       \
    struct Auto_Hook_##name_ {                                                                              \
        static void Auto_Hook_##name_##_Install() {                                                         \
            static constexpr auto logger = Paper::ConstLoggerContext(MOD_ID "_Install_" #name_);            \
            ::i2c::install_hook_orig<Hook_##name_>(logger);                                               \
        }                                                                                                   \
        Auto_Hook_##name_() { ::MultiplayerCore::Hooking::AddInstallFunc(Auto_Hook_##name_##_Install); }    \
    };                                                                                                      \
    static Auto_Hook_##name_ Auto_Hook_Instance_##name_

#define HOOK_AUTO_INSTALL(name_)                                                                            \
    struct Auto_Hook_##name_ {                                                                              \
        static void Auto_Hook_##name_##_Install() {                                                         \
            static constexpr auto logger = Paper::ConstLoggerContext(MOD_ID "_Install_" #name_);            \
            ::i2c::install_hook<Hook_##name_>(logger);                                                   \
        }                                                                                                   \
        Auto_Hook_##name_() { ::MultiplayerCore::Hooking::AddInstallFunc(Auto_Hook_##name_##_Install); }    \
    };                                                                                                      \
    static Auto_Hook_##name_ Auto_Hook_Instance_##name_

#define MAKE_AUTO_HOOK_MATCH(name_, mPtr, retval, ...)                                                                                              \
    struct Hook_##name_                                                                                                                             \
    {                                                                                                                                               \
        using func_t = retval (*)(__VA_ARGS__);                                                                                                   \
        static constexpr auto cast_test = []<typename Method>() { return requires { static_cast<Method>(mPtr); }; }; \
        using method_t = typename ::i2c::detail::method_check<cast_test, func_t>::type; \
        static_assert(cast_test.operator()<method_t>(), "Hook method signature does not match!"); \
        static_assert(::i2c::detail::match_hookable<static_cast<method_t>(mPtr)>, "Method cannot be hooked!"); \
        constexpr static const char* name() { return #name_; }                                                                                      \
        static const MethodInfo* addr() { return ::i2c::metadata_getter<static_cast<method_t>(mPtr)>::method_info(); }                        \
        static inline flamingo::HookHandle install_handle; \
        static inline flamingo::HookPriority install_priority; \
        static func_t* trampoline() { return &name_; }                                                                                            \
        static inline retval (*name_)(__VA_ARGS__) = nullptr;                                                                                       \
        static func_t hook() { return &hook_##name_; }                                           \
        static retval hook_##name_(__VA_ARGS__);                                                                                                    \
    };                                                                                                                                              \
    HOOK_AUTO_INSTALL(name_);                                                                                                                       \
    retval Hook_##name_::hook_##name_(__VA_ARGS__)

#define MAKE_AUTO_HOOK_FIND_VERBOSE(name_, infoGet, retval, ...)                                            \
    struct Hook_##name_ {                                                                                   \
        constexpr static const char* name() { return #name_; }                                              \
        static const MethodInfo* addr() { return infoGet; }                                              \
        using func_t = retval (*)(__VA_ARGS__);                                                           \
        static inline flamingo::HookHandle install_handle; \
        static inline flamingo::HookPriority install_priority; \
        static func_t* trampoline() { return &name_; }                                                    \
        static inline retval (*name_)(__VA_ARGS__) = nullptr;                                               \
        static func_t hook() { return &hook_##name_; }   \
        static retval hook_##name_(__VA_ARGS__);                                                            \
    };                                                                                                      \
    HOOK_AUTO_INSTALL_ORIG(name_);                                                                          \
    retval Hook_##name_::hook_##name_(__VA_ARGS__)

#define MAKE_AUTO_HOOK_ORIG_MATCH(name_, mPtr, retval, ...)                                                                                         \
    struct Hook_##name_                                                                                                                             \
    {                                                                                                                                               \
        using func_t = retval (*)(__VA_ARGS__);                                                                                                   \
        static constexpr auto cast_test = []<typename Method>() { return requires { static_cast<Method>(mPtr); }; }; \
        using method_t = typename ::i2c::detail::method_check<cast_test, func_t>::type; \
        static_assert(cast_test.operator()<method_t>(), "Hook method signature does not match!"); \
        static_assert(::i2c::detail::match_hookable<static_cast<method_t>(mPtr)>, "Method cannot be hooked!"); \
        constexpr static const char* name() { return #name_; }                                                                                      \
        static const MethodInfo* addr() { return ::i2c::metadata_getter<static_cast<method_t>(mPtr)>::method_info(); }                        \
        static inline flamingo::HookHandle install_handle; \
        static inline flamingo::HookPriority install_priority; \
        static func_t* trampoline() { return &name_; }                                                                                            \
        static inline retval (*name_)(__VA_ARGS__) = nullptr;                                                                                       \
        static func_t hook() { return &hook_##name_; }                                           \
        static retval hook_##name_(__VA_ARGS__);                                                                                                    \
    };                                                                                                                                              \
    HOOK_AUTO_INSTALL_ORIG(name_);                                                                                                                  \
    retval Hook_##name_::hook_##name_(__VA_ARGS__)
