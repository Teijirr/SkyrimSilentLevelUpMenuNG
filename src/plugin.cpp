#include "log.h"

class LevelUpMenuCloser final : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
	static LevelUpMenuCloser* GetSingleton() {
		static LevelUpMenuCloser singleton;
		return &singleton;
	}

	RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
		RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {

		if (!a_event || !a_event->opening) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Level up menu opening
		if (a_event->menuName == RE::LevelUpMenu::MENU_NAME) {
			if (auto* ui = RE::UI::GetSingleton()) {
				if (auto menu = ui->GetMenu(RE::LevelUpMenu::MENU_NAME)) {
					if (menu->uiMovie) {
						// Hide UI
						menu->uiMovie->SetVisible(false);

						SKSE::GetTaskInterface()->AddTask([]() {
							auto* ui = RE::UI::GetSingleton();
							if (!ui || !ui->IsMenuOpen(RE::LevelUpMenu::MENU_NAME)) {
								return;
							}

							auto menu = ui->GetMenu(RE::LevelUpMenu::MENU_NAME);
							if (!menu || !menu->uiMovie) {
								return;
							}

							// Pick magicka and accept
							RE::GFxValue emptyArgs;
							menu->uiMovie->CreateArray(&emptyArgs);
							RE::GFxValue args[2];
							args[0].SetString("addMagicka");
							args[1] = emptyArgs;
							RE::GFxValue result;
							menu->uiMovie->Invoke("_global.gfx.io.GameDelegate.call", &result, args, 2);
						});
					}
				}
			}
		}
		// Confirmation box opening right after
		else if (a_event->menuName == RE::MessageBoxMenu::MENU_NAME) {
			if (auto* ui = RE::UI::GetSingleton(); ui && ui->IsMenuOpen(RE::LevelUpMenu::MENU_NAME)) {
				if (auto menu = ui->GetMenu(RE::MessageBoxMenu::MENU_NAME)) {
					if (menu->uiMovie) {
						// Hide UI
						menu->uiMovie->SetVisible(false);

						// Accept
						SKSE::GetTaskInterface()->AddTask([]() {
							auto* ui = RE::UI::GetSingleton();
							if (!ui || !ui->IsMenuOpen(RE::MessageBoxMenu::MENU_NAME)) {
								return;
							}

							auto menu = ui->GetMenu(RE::MessageBoxMenu::MENU_NAME);
							if (!menu || !menu->uiMovie) {
								return;
							}

							// Get Current Magicka
							float originalMagicka = 0.0f;
							auto* player = RE::PlayerCharacter::GetSingleton();
							RE::ActorValueOwner* avOwner = player ? player->AsActorValueOwner() : nullptr;
							if (avOwner) {
								originalMagicka = avOwner->GetActorValue(RE::ActorValue::kMagicka);
							}

							RE::GFxValue indexArray;
							menu->uiMovie->CreateArray(&indexArray);
							RE::GFxValue zero(0.0);
							indexArray.SetElement(0, zero);

							RE::GFxValue args[2];
							args[0].SetString("buttonPress");
							args[1] = indexArray;

							RE::GFxValue result;
							menu->uiMovie->Invoke("_global.gfx.io.GameDelegate.call", &result, args, 2);

							// Restore Magicka
							if (avOwner) {
								avOwner->SetActorValue(RE::ActorValue::kMagicka, originalMagicka);
							}
						});
					}
				}
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

private:
	LevelUpMenuCloser() = default;
};

void OnDataLoaded() {
	auto* ui = RE::UI::GetSingleton();
	if (!ui) {
		SKSE::log::error("NoLevelUpMenu: RE::UI singleton not available at kDataLoaded, cannot register sink.");
		return;
	}

	ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(LevelUpMenuCloser::GetSingleton());
}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg) {
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		SKSE::log::info("NoLevelUpMenu: kDataLoaded received, installing hook.");
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
		SKSE::log::error("NoLevelUpMenu: failed to register SKSE messaging listener.");
		return false;
	}

	return true;
}