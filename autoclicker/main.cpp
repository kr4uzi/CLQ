#include "mouse.h"
#include "keyboard.h"
#include "config.h"
#include <iostream>
#include <fstream>

#ifdef WIN32
#define AUTOCLICKER_DEFAULT_HOTKEY 0x91 // scroll lock
#elif defined(__APPLE__)
#define AUTOCLICKER_DEFAULT_HOTKEY 0x6B // scroll lock
#endif

#define AUTOCLICKER_XSTR(s) AUTOCLICKER_STR(s)
#define AUTOCLICKER_STR(s) #s

int main()
{
	boost::posix_time::milliseconds interval(20);
	auto button = mouse::button::RIGHT;
	keyboard::keycode hotkey = AUTOCLICKER_DEFAULT_HOTKEY;

	config cfg;
	auto parse_error = config::parse("autoclicker.cfg", cfg);
	if (!parse_error && cfg.exists("interval") && cfg.exists("button") && cfg.exists("hotkey"))
	{
		interval = boost::posix_time::milliseconds(cfg.get_unsinged("interval", 20));
		button = (cfg.get_string("button", "right") == "right") ? mouse::button::RIGHT : mouse::button::LEFT;
		hotkey = cfg.get_unsinged("hotkey", AUTOCLICKER_DEFAULT_HOTKEY);

		if (keyboard::to_string(hotkey).empty())
		{
			std::cout << "invalid hotkey detected, falling back to " AUTOCLICKER_XSTR(AUTOCLICKER_DEFAULT_HOTKEY) << std::endl;
			hotkey = AUTOCLICKER_DEFAULT_HOTKEY;
		}
	}
	else
	{
		std::cout << "There has been an error parsing [autoclicker.cfg]!" << std::endl;
		std::cout << "Creating [autoclicker.cfg] with default values." << std::endl;
		std::cout << "Proceeding with default values." << std::endl;
		std::cout << std::endl;

		std::ofstream of("autoclicker.cfg", std::ios::trunc);
		of << "# click interval in milliseconds" << std::endl;
		of << "interval = 20" << std::endl;
		of << std::endl;
		of << "# button to autoclick: left or right" << std::endl;
		of << "button = right" << std::endl;
		of << std::endl;
        of << "# hotkey as virtual-key code - default hotkey is SCROLL (" AUTOCLICKER_XSTR(AUTOCLICKER_DEFAULT_HOTKEY) ")" << std::endl;
#ifdef WIN32
		of << "# https://msdn.microsoft.com/library/windows/desktop/dd375731.aspx" << std::endl;
#elif defined(__APPLE__)
        of << "# /System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/Headers/Events.h" << std::endl;
#endif
		of << "hotkey = " AUTOCLICKER_XSTR(AUTOCLICKER_DEFAULT_HOTKEY) << std::endl;
	}

	std::cout << "CLQ Autoclicker - for the best DotA 2 player, the world has ever fucking seen" << std::endl << std::endl;
	std::wcout << "Start by pressing [" << keyboard::to_string(hotkey) << "]" << std::endl;
	std::cout << "The click interval is [" << interval.total_milliseconds() << "] ms" << std::endl;
	std::cout << "The [" << ((button == mouse::button::RIGHT) ? "right" : "left") << "] mouse button will be clicked" << std::endl;
	std::cout << std::endl;
    std::cout << "Status: not clicking" << '\r' << std::flush;

	boost::asio::io_service io;
	mouse m(io, button, interval);

	keyboard::initialize(io);
	keyboard::get_instance()->set_scroll_lock(false);
	keyboard::get_instance()->on_key_press.connect([&](keyboard::message msg, keyboard::keycode vkey)
	{        
		if (msg == keyboard::message::KEYUP && vkey == hotkey)
		{
			if (m.is_clicking())
			{
				std::cout << "Status: not clicking" << '\r' << std::flush;
				m.click_stop();
			}
			else
			{
				std::cout << "Status: clicking    " << '\r' << std::flush;
				m.click_start();
			}
		}
	});

	io.run();
}