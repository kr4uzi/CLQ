#include "keyboard.h"
#include <cassert>		// assert
#ifdef WIN32
#include <Windows.h>
#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <cstdio>
#endif

struct keyboard_impl
{
#ifdef WIN32
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

	static std::wstring keycode_to_string(keyboard::keycode vkey)
	{
		auto scode = MapVirtualKeyEx(vkey, 0, GetKeyboardLayout(0));
		wchar_t key_name[32];
        auto len = GetKeyNameTextW(scode << 16, key_name, sizeof(key_name) / sizeof(key_name[0]));
        if (len > 0)
            return std::wstring(key_name, key_name + len);
		
        return L"unknown";
	}
#elif defined(__APPLE__)
    static CGEventRef KeyboardHookCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void * refcon)
    {
        using message = keyboard::message;
        message msg;
        
        switch( type )
        {
            case kCGEventKeyDown:
                msg = message::KEYDOWN;
                break;
            case kCGEventKeyUp:
                msg = message::KEYUP;
                break;
                
            default:
                return event;
        }
        
        auto keyboard = keyboard::get_instance();
        auto key = (keyboard::keycode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
        keyboard->service.post([keyboard, msg, key](){
            keyboard->on_key_press(msg, key);
        });
        
        return event;
    }
    
    static std::wstring keycode_to_string(keyboard::keycode vkey)
    {
        TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();
        CFDataRef uchr = (CFDataRef)TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
        const UCKeyboardLayout *keyboardLayout = (const UCKeyboardLayout*)CFDataGetBytePtr(uchr);
        
        if(keyboardLayout)
        {
            UInt32 deadKeyState = 0;
            UniCharCount maxStringLength = 255;
            UniCharCount actualStringLength = 0;
            UniChar unicodeString[maxStringLength];
            
            OSStatus status = UCKeyTranslate(keyboardLayout, vkey, kUCKeyActionDown, 0, LMGetKbdType(), 0, &deadKeyState, maxStringLength, &actualStringLength, unicodeString);
            
            if (actualStringLength == 0 && deadKeyState)
            {
                status = UCKeyTranslate(keyboardLayout, kVK_Space, kUCKeyActionDown, 0, LMGetKbdType(), 0, &deadKeyState, maxStringLength, &actualStringLength, unicodeString);
            }
            
            if(actualStringLength > 0 && status == noErr)
                return std::wstring(unicodeString, unicodeString + actualStringLength);
        }
        
        return L"unknown";
    }
    
#endif
};

keyboard * keyboard::instance = nullptr;

std::wstring keyboard::to_string(keycode vkey)
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
#ifdef WIN32
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
#elif defined(__APPLE__)
    CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp);
    CFMachPortRef eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, eventMask, keyboard_impl::KeyboardHookCallback, nullptr);
    
    if (!eventTap)
    {
        std::fprintf(stderr, "failed to create eventtap!");
        std::exit(1);
    }
    
    
    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    CFRunLoopRun();
#endif
	running = false;
}

bool keyboard::get_scroll_lock()
{
#ifdef WIN32
	return GetKeyState(VK_SCROLL) == 1;
#elif defined(__APPLE__)
    return false;
#endif
}

void keyboard::set_scroll_lock(bool flag)
{
	bool locked = get_scroll_lock();

	if (flag ^ locked)
	{
#ifdef WIN32
		INPUT input[2];
		ZeroMemory(input, sizeof(input));
		input[0].type = input[1].type = INPUT_KEYBOARD;
		input[0].ki.wVk = input[1].ki.wVk = VK_SCROLL;
		input[1].ki.dwFlags = KEYEVENTF_KEYUP;

		SendInput(2, input, sizeof(INPUT));
#elif defined(__APPLE__)
        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
        CGEventRef saveCommandDown = CGEventCreateKeyboardEvent(source, (CGKeyCode)0x91, true);
        CGEventRef saveCommandUp = CGEventCreateKeyboardEvent(source, (CGKeyCode)0x91, false);
        
        CGEventPost(kCGAnnotatedSessionEventTap, saveCommandDown);
        CGEventPost(kCGAnnotatedSessionEventTap, saveCommandUp);
        
        CFRelease(saveCommandUp);
        CFRelease(saveCommandDown);
        CFRelease(source);
#endif
    }
}