#pragma once
#include "custom-types/shared/macros.hpp"

#include "LiteNetLib/Utils/NetDataReader.hpp"
#include "LiteNetLib/Utils/NetDataWriter.hpp"
#include "LiteNetLib/Utils/INetSerializable.hpp"

DECLARE_CLASS_CODEGEN_INTERFACES(MultiplayerCore::Networking::Abstractions, MpPacket, System::Object, LiteNetLib::Utils::INetSerializable*) {
    DECLARE_OVERRIDE_METHOD_MATCH(void, Serialize, &LiteNetLib::Utils::INetSerializable::Serialize, LiteNetLib::Utils::NetDataWriter* writer);
    DECLARE_OVERRIDE_METHOD_MATCH(void, Deserialize, &LiteNetLib::Utils::INetSerializable::Deserialize, LiteNetLib::Utils::NetDataReader* reader);

    DECLARE_DEFAULT_CTOR();
    public:
        [[nodiscard]] LiteNetLib::Utils::INetSerializable* i_INetSerializable() { return reinterpret_cast<LiteNetLib::Utils::INetSerializable*>(this); }
};

#include "beatsaber-hook/shared/members.hpp"
template<>
struct ::i2c::metadata_getter<static_cast<void (MultiplayerCore::Networking::Abstractions::MpPacket::*)(LiteNetLib::Utils::NetDataWriter*)>(&MultiplayerCore::Networking::Abstractions::MpPacket::Serialize)> {
    static const MethodInfo* method_info() {
        return i2c::find_method(i2c::class_of<MultiplayerCore::Networking::Abstractions::MpPacket*>(),
            {"Serialize", {}, {i2c::type_of<LiteNetLib::Utils::NetDataWriter*>()}});
    }
};
template<>
struct ::i2c::metadata_getter<static_cast<void (MultiplayerCore::Networking::Abstractions::MpPacket::*)(LiteNetLib::Utils::NetDataReader*)>(&MultiplayerCore::Networking::Abstractions::MpPacket::Deserialize)> {
    static const MethodInfo* method_info() {
        return i2c::find_method(i2c::class_of<MultiplayerCore::Networking::Abstractions::MpPacket*>(),
            {"Deserialize", {}, {i2c::type_of<LiteNetLib::Utils::NetDataReader*>()}});
    }
};
