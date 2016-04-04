#include "mouse.h"
#include "keyboard.h"
#include "config.h"
#include <iostream>
#include <fstream>

int main()
{
	boost::posix_time::milliseconds interval(20);
	auto button = mouse::button::RIGHT;
	keyboard::keycode hotkey = 0x91;	// VK_SCROLL

	config cfg;
	auto parse_error = config::parse("autoclicker.cfg", cfg);
	if (!parse_error && cfg.exists("interval") && cfg.exists("button") && cfg.exists("hotkey"))
	{
		interval = boost::posix_time::milliseconds(cfg.get_unsinged("interval", 20));
		button = (cfg.get_string("button", "right") == "right") ? mouse::button::RIGHT : mouse::button::LEFT;
		hotkey = cfg.get_unsinged("hotkey", 0x91);

		if (keyboard::to_string(hotkey).empty())
		{
			std::cout << "invalid hotkey detected, falling back to 0x91" << std::endl;
			hotkey = 0x91;
		}
	}
	else
	{
		std::cout << "There has been an error parsing [autoclicker.cfg]!" << std::endl;
		std::cout << "Creating [autoclicker.cfg] with default values." << std::endl;
		std::cout << "Proceeding with default values." << std::endl;
		std::cout << std::endl;

		std::ofstream of("autoclicker.cfg", std::ios::trunc);
		of << "# click interval in miliseconds" << std::endl;
		of << "interval = 20" << std::endl;
		of << std::endl;
		of << "# button to autoclick: left or right" << std::endl;
		of << "button = right" << std::endl;
		of << std::endl;
		of << "# hotkey as virtual-key code - default hotkey is SCROLL (0x91)" << std::endl;
		of << "# https://msdn.microsoft.com/library/windows/desktop/dd375731.aspx" << std::endl;
		of << "hotkey = 0x91" << std::endl;
	}

	std::cout << "CLQ Autoclicker - for the best DotA 2 player, the world has ever fucking seen" << std::endl << std::endl;
	std::cout << "Start by pressing [" << keyboard::to_string(hotkey) << "]" << std::endl;
	std::cout << "The click interval is [" << interval.total_milliseconds() << "] ms" << std::endl;
	std::cout << "The [" << ((button == mouse::button::RIGHT) ? "right" : "left") << "] mouse button will be clicked" << std::endl;
	std::cout << std::endl;
	std::cout << "Status: not clicking" << '\r';

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
				std::cout << "Status: not clicking" << '\r';
				m.click_stop();
			}
			else
			{
				std::cout << "Status: clicking    " << '\r';
				m.click_start();
			}
		}
	});

	io.run();
}