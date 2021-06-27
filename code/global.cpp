#include "global.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

std::string WIN_WideToUTF8(const wchar_t* buffer, int size)
{
	int length = WideCharToMultiByte(CP_UTF8, 0, buffer, size, NULL, 0, NULL, NULL);
	std::string output(length, 0);
	WideCharToMultiByte(CP_UTF8, 0, buffer, size, output.data(), length, NULL, NULL);
	return output;
}

std::wstring WIN_UTF8ToWide(const char* buffer, int size)
{
	int length = MultiByteToWideChar(CP_UTF8, 0, buffer, size, NULL, 0);
	std::wstring output(length, 0);
	MultiByteToWideChar(CP_UTF8, 0, buffer, size, output.data(), length);
	return output;
}

std::string WIN_GetFormattedGLE()
{
	// Retrieve the system error message for the last-error code

	wchar_t* lpMsgBuf = NULL;
	DWORD dw = GetLastError();

	// It is possible to set the current codepage to utf8, and get utf8 messages from this
	// but then I would need to use unique_ptr<> with a custom deleter for LocalFree
	// to get the data without any redundant copying.
	int buflen = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS |
			FORMAT_MESSAGE_MAX_WIDTH_MASK, // this removes the extra newline
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&lpMsgBuf),
		0,
		NULL);

	std::string str(WIN_WideToUTF8(lpMsgBuf, buflen));

	LocalFree(lpMsgBuf);

	return str;
}
#endif

bool implement_CHECK(bool cond, const char* expr, const char* file, int line)
{
	if(!cond)
	{
		std::string stack_message;
		serrf(
			"\nCheck failed\n"
			"File: %s, Line %d\n"
			"Expression: `%s`\n"
			"\nStacktrace:\n"
			"%s\n",
			file,
			line,
			expr,
			stack_message.c_str());

		return false;
	}

	return true;
}

// serr buffer lazy initialized.
std::shared_ptr<std::string> internal_get_serr_buffer()
{
	static thread_local std::shared_ptr<std::string> buffer;
	if(!buffer) { buffer = std::make_shared<std::string>(); }
	return buffer;
}

static void serr_safe_stacktrace(int skip = 0)
{
	(void)skip;
#if 0
    if(cv_stacktrace_on_serr.get_value() == 2.0 || (cv_stacktrace_on_serr.get_value() == 1.0 && !serr_check_error()))
	{
        std::string msg;
        msg += "StackTrace (cv_stacktrace_on_serr):\n";
        debug_stacktrace_string(msg, skip+1);
        msg += '\n';
        slog_raw(msg.data(), msg.length());
        internal_get_serr_buffer()->append(msg);
    }
#endif
}

std::string serr_get_error()
{
	return std::move(*internal_get_serr_buffer());
}
bool serr_check_error()
{
	return !internal_get_serr_buffer()->empty();
}

void slog_raw(const char* msg, size_t len)
{
	fwrite(msg, 1, len, stdout);
}
void serr_raw(const char* msg, size_t len)
{
	serr_safe_stacktrace(1);
	slog_raw(msg, len);
	internal_get_serr_buffer()->append(msg, msg + len);
}

void slog(const char* msg)
{
	// don't use puts because it inserts a newline.
	fputs(msg, stdout);
}

void serr(const char* msg)
{
	serr_safe_stacktrace(1);
	slog(msg);
	internal_get_serr_buffer()->append(msg);
}

void slogf(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
#ifdef WIN32
	// win32 has a compatible C standard library, but annex k prevents exploits or something.
	vfprintf_s(stdout, msg, args);
#else
	vfprintf(stdout, msg, args);
#endif
	va_end(args);
}

void serrf(const char* msg, ...)
{
	serr_safe_stacktrace(1);

	int length;
	va_list args;

	va_start(args, msg);
	std::unique_ptr<char[]> buffer = unique_vasprintf(&length, msg, args);
	va_end(args);

	slog_raw(buffer.get(), length);
	internal_get_serr_buffer()->append(buffer.get(), buffer.get() + length);
}

std::unique_ptr<char[]> unique_vasprintf(int* length, const char* msg, va_list args)
{
	int ret;
	va_list temp_args;

	// it says you should copy if you use valist more than once.
	va_copy(temp_args, args);
#ifdef WIN32
	char buf[1];
	// win32 has a compatible C standard library, but annex k prevents exploits or something.
	ret = vsnprintf_s(buf, 1, msg, temp_args);
#else
	ret = vsnprintf(NULL, 0, msg, temp_args);
#endif
	va_end(temp_args);

	ASSERT(ret != -1);
	if(ret == -1)
	{
		char message[] = "?badformat?";
		std::unique_ptr<char[]> buffer(new char[sizeof(message)]);
		std::copy_n(message, sizeof(message), buffer.get());
		if(length != NULL) { *length = sizeof(message) - 1; }
		return buffer;
	}

	std::unique_ptr<char[]> buffer(new char[ret + 1]);

#ifdef WIN32
	ret = vsprintf_s(buffer.get(), ret + 1, msg, args);
#else
	ret = vsnprintf(buffer.get(), ret + 1, msg, args);
#endif

	ASSERT(ret != -1);

	if(length != NULL) { *length = ret; }

	return buffer;
}
