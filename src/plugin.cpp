#include "log.h"

void SilentLevelUp()
{
    if (auto* p = RE::PlayerCharacter::GetSingleton())
        p->GetPlayerRuntimeData().skills->AdvanceLevel(false);
}

void OnDataLoaded()
{
    SKSE::AllocTrampoline(14);

    const auto addr = REL::RelocationID(51638, 52510).address()
        + (REL::Module::IsAE() ? 0x1012 : 0xF8E);

    SKSE::GetTrampoline().write_call<5>(addr, SilentLevelUp);
    SKSE::log::info("NoLevelUpMenu: hook installed.");
}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
    switch (a_msg->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        OnDataLoaded();
        break;
    case SKSE::MessagingInterface::kPostLoad:
        break;
    case SKSE::MessagingInterface::kPreLoadGame:
        break;
    case SKSE::MessagingInterface::kPostLoadGame:
        break;
    case SKSE::MessagingInterface::kNewGame:
        break;
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SetupLog();

    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging->RegisterListener("SKSE", MessageHandler)) {
        return false;
    }

    return true;
}