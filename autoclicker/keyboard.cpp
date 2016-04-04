#include "keyboard.h"
#include <cassert>		// assert
#include <Windows.h>

struct keyboard_impl
{
	static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION && !(((PKBDLLHOOKSTRUCT)lParam)->flags & LLKHF_INJECTED))
		{
			using message = keyboard::message;
			message msg;

			switch (wParam)
			{
			case WM_KEYDOWN:
				msg = message::KEYDOWN;
				break;
			case WM_SYSKEYDOWN:
				msg = message::SYSKEYDOWN;
				break;
			case WM_KEYUP:
				msg = message::KEYUP;
				break;
			case WM_SYSKEYUP:
				msg = message::SYSKEYUP;
				break;

			default:
				// unknown message, dont forward
				goto end;
			}

			auto keyboard = keyboard::get_instance();
			keyboard->service.post([keyboard, msg, key = ((PKBDLLHOOKSTRUCT)lParam)->vkCode](){
				keyboard->on_key_press(msg, key);
			});
		}

	end:
		return CallNextHookEx(nullptr, nCode, wParam, lParam);
	}

	static std::string keycode_to_string(keyboard::keycode vkey)
	{
		auto scode = MapVirtualKeyEx(vkey, 0, GetKeyboardLayout(0));
		char key_name[32];
		if (GetKeyNameText(scode << 16, key_name, sizeof(key_name)) == 0)
			return "";

		return std::string(key_name);
	}
};

keyboard * keyboard::instance = nullptr;

std::string keyboard::to_string(keycode vkey)
{
	return keyboard_impl::keycode_to_string(vkey);
}

keyboard * keyboard::initialize(boost::asio::io_service& service)
{
	assert(!instance && "you can only call initialize(io_service&) once!");

	return instance = new keyboard(service);
}

keyboard * keyboard::get_instance()
{
	assert(instance && "initialize(io_service&) has to be called first!");

	return instance;
}

keyboard::keyboard(boost::asio::io_service& service)
	: service(service), work(service)
{
	thread = std::thread(&keyboard::loop, this);
}

keyboard::~keyboard()
{
	running = false;
	thread.join();
}

void keyboard::loop()
{
	auto hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_impl::LowLevelKeyboardProc, nullptr, 0);
	if (hook)
	{
		MSG msg;
		while (running && GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	UnhookWindowsHookEx(hook);
	running = false;
}

bool keyboard::get_scroll_lock()
{
	return GetKeyState(VK_SCROLL) == 1;
}

void keyboard::set_scroll_lock(bool flag)
{
	bool locked = get_scroll_lock();

	if (flag ^ locked)
	{
		INPUT input[2];
		ZeroMemory(input, sizeof(input));
		input[0].type = input[1].type = INPUT_KEYBOARD;
		input[0].ki.wVk = input[1].ki.wVk = VK_SCROLL;
		input[1].ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(2, input, sizeof(INPUT));
	}
}