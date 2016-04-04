#pragma once
#include <boost/signals2.hpp>
#include <boost/asio/io_service.hpp>

#include <thread>

class keyboard
{
	friend struct keyboard_impl;

public:
	enum class message
	{
		KEYDOWN,
		KEYUP,
		SYSKEYDOWN,
		SYSKEYUP
	};

	typedef unsigned int keycode;
	static std::wstring to_string(keycode vkey);

private:
	std::thread thread;

	boost::asio::io_service& service;
	boost::asio::io_service::work work;
	bool running = true;

	static keyboard * instance;

public:
	static keyboard * initialize(boost::asio::io_service& service);
	static keyboard * get_instance();

	boost::signals2::signal<void(message, keycode)> on_key_press;

	bool get_scroll_lock();
	void set_scroll_lock(bool flag);

private:
	keyboard(boost::asio::io_service& service);
	~keyboard();

	void loop();
};
