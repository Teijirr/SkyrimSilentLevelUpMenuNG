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

		if (a_event->menuName == RE::LevelUpMenu::MENU_NAME) {
			if (auto* ui = RE::UI::GetSingleton()) {
				if (auto menu = ui->GetMenu(RE::LevelUpMenu::MENU_NAME)) {
					if (menu->uiMovie) {
						menu->uiMovie->SetVisible(false);
					}
				}
			}

			// First Enter key to select the attribute (e.g., Magicka)
			SKSE::GetTaskInterface()->AddTask([]() {
				//SendScaleformEnter(RE::LevelUpMenu::MENU_NAME);
				//SendNativeAcceptEvent();
				SendMenuAcceptInput();
			});
		}
		// Confirmation box opening right after
		else if (a_event->menuName == RE::MessageBoxMenu::MENU_NAME) {
			if (auto* ui = RE::UI::GetSingleton(); ui && ui->IsMenuOpen(RE::LevelUpMenu::MENU_NAME)) {
				if (auto menu = ui->GetMenu(RE::MessageBoxMenu::MENU_NAME)) {
					if (menu->uiMovie) {
						menu->uiMovie->SetVisible(false);
					}
				}

				// Second Enter key to confirm the choice
				SKSE::GetTaskInterface()->AddTask([]() {
					//SendScaleformEnter(RE::MessageBoxMenu::MENU_NAME);
					//SendNativeAcceptEvent();
					SendMenuAcceptInput();

					auto player = RE::PlayerCharacter::GetSingleton();

					if (player) {
						auto avOwner = player->AsActorValueOwner();

						if (avOwner) {
							avOwner->ModActorValue(RE::ActorValue::kMagicka, -10.0f);
						}
					}
				});
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

private:
	LevelUpMenuCloser() = default;

	/*
	static void SendScaleformEnter(const RE::BSFixedString& a_menuName) {
		auto* ui = RE::UI::GetSingleton();
		if (!ui) return;

		auto menu = ui->GetMenu(a_menuName);
		if (!menu || !menu->uiMovie) return;

		RE::GFxKeyEvent keyDownEvent;
		keyDownEvent.type = RE::GFxEvent::EventType::kKeyDown;
		keyDownEvent.keyCode = RE::GFxKey::Code::kReturn;

		RE::GFxKeyEvent keyUpEvent;
		keyUpEvent.type = RE::GFxEvent::EventType::kKeyUp;
		keyUpEvent.keyCode = RE::GFxKey::Code::kReturn;

		menu->uiMovie->HandleEvent(keyDownEvent);
		menu->uiMovie->HandleEvent(keyUpEvent);
	}
	*/
	/*
	static void SendNativeAcceptEvent() {
		// Création de l'événement natif de validation
		auto* buttonEvent = RE::ButtonEvent::Create(
			RE::INPUT_DEVICE::kKeyboard,
			"Accept",
			0x1C,
			1.0f,
			0.0f
		);

		if (!buttonEvent) return;

		buttonEvent->next = nullptr;

		auto* menuControls = RE::MenuControls::GetSingleton();
		if (menuControls) {
			// Cast vers l'interface de base pour exposer ProcessEvent
			auto* inputSink = static_cast<RE::BSTEventSink<RE::InputEvent*>*>(menuControls);

			// Correction ici : On crée le pointeur de base requis par la signature
			RE::InputEvent* inputEvent = buttonEvent;

			// On passe l'adresse du pointeur (&inputEvent) pour obtenir le type RE::InputEvent* const*
			inputSink->ProcessEvent(&inputEvent, nullptr);
		}
	}
	*/

	static void ProcessFlatAcceptInput() {
		auto* buttonEvent = RE::ButtonEvent::Create(
			RE::INPUT_DEVICE::kKeyboard,
			"Accept",
			0x1C,
			1.0f,
			0.0f
		);

		if (!buttonEvent) return;
		buttonEvent->next = nullptr;

		auto* menuControls = RE::MenuControls::GetSingleton();
		if (menuControls) {
			auto* inputSink = static_cast<RE::BSTEventSink<RE::InputEvent*>*>(menuControls);
			RE::InputEvent* inputEvent = buttonEvent;
			inputSink->ProcessEvent(&inputEvent, nullptr);
		}
	}

	static void SimulateButtonPress(WORD vkey) {
		HWND hwnd = ::FindWindowExA(nullptr, nullptr, "Skyrim VR", nullptr);
		if (hwnd) {
			HWND foreground = GetForegroundWindow();
			if (foreground && hwnd == foreground) {
				INPUT input = {};
				input.type = INPUT_KEYBOARD;
				input.ki.wScan = static_cast<WORD>(MapVirtualKeyA(vkey, MAPVK_VK_TO_VSC));
				input.ki.time = 0;
				input.ki.dwExtraInfo = 0;
				input.ki.wVk = vkey;

				input.ki.dwFlags = 0;
				SendInput(1, &input, sizeof(INPUT));

				::Sleep(30);

				input.ki.dwFlags = KEYEVENTF_KEYUP;
				SendInput(1, &input, sizeof(INPUT));
			}
		}
	}

	static void ProcessVRAcceptInput() {
		SKSE::log::trace("ProcessVRAcceptInput: Spawning detached macro thread.");

		std::thread t1([]() {
			SKSE::log::trace("ProcessVRAcceptInput Thread: Pressing 'E' to select Magicka.");
			SimulateButtonPress(0x45);

			::Sleep(100);

			SKSE::log::trace("ProcessVRAcceptInput Thread: Pressing 'Enter' to confirm choice.");
			SimulateButtonPress(VK_RETURN);

			SKSE::log::info("ProcessVRAcceptInput Thread: E -> Enter sequence completed successfully.");
			});

		t1.detach();
	}

	static void SendMenuAcceptInput() {
		if (REL::Module::IsVR()) {
			ProcessVRAcceptInput();
		}
		else {
			ProcessFlatAcceptInput();
		}
	}
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