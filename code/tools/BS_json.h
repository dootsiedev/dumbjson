#pragma once

#ifndef DISABLE_BS_JSON
#include "BS_archive.h"

#include <rapidjson/reader.h>
#include <rapidjson/error/en.h>

#include <rapidjson/prettywriter.h>

namespace rj = rapidjson;

std::string b64decode(const void* data, size_t len);
std::string base64_encode(const void* src, size_t len);

template<class Reader, size_t max_line_length = 100>
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void print_json_error(Reader& reader, size_t offset)
{
	char buffer[max_line_length + 1];
	size_t line_n = 0;
	size_t count = 0;
	size_t line_start = 0;
	char* pos = buffer;
	char* end = buffer + max_line_length;
	while(true)
	{
		if(pos < end)
		{
			*pos = reader.Take();
			++count;
			if(*pos == '\n')
			{
				if(offset < count)
				{
					break;
				}
				++line_n;
				pos = buffer;
				line_start = count;
			}
			else if(*pos == '\0')
			{
				break;
			}
			else
				++pos;
		}
		else
		{
			char c = reader.Take();
			++count;
			if(c == '\n')
			{
				if(offset < count)
				{
					break;
				}
				++line_n;
				pos = buffer;
				line_start = count;
			}
			else if(c == '\0')
			{
				break;
			}
		}
	}
	if(offset <= count)
	{
		// clear any control values (mainly for windows \r\n)
		char* line_end = std::remove_if(buffer, pos, [](unsigned char _1) {
			return _1 != '\n' && _1 != '\t' && (_1 < 32 || _1 == 127);
		});

		*line_end = '\0';

		// tabs to spaces because it makes the column index clearer.
		// std::replace(buffer, line_end, '\t', ' ');

		serrf(
			"Line: %zu\n"
			"Col: %zu\n"
			">>>%s\n",
			line_n + 1,
			offset - line_start + 1,
			buffer);
	}
	else
	{
		// TODO (dootsie): I didn't test this enough
		// technically this is reachable if the stream
		// gives a different result after rewinding,
		// like for example if the first pass had content,
		// but the 2nd pass is empty.
		ASSERT(false && "unreachable");
	}
}

class internal_bool_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_bool_json_handler>
{
public:
	BS_bool_cb call;
	void* ud;

	explicit internal_bool_json_handler(BS_bool_cb cb, void* ud_)
	: call(cb)
	, ud(ud_)
	{
	}
	bool Default()
	{
		serr("expected bool\n");
		return false;
	}
	bool Bool(bool b)
	{
		return call(b, ud);
	}
};

class internal_int32_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_int32_json_handler>
{
public:
	BS_int32_cb call;
	void* ud;

	explicit internal_int32_json_handler(BS_int32_cb cb, void* ud_)
	: call(cb)
	, ud(ud_)
	{
	}
	bool Default()
	{
		serr("expected int\n");
		return false;
	}
	bool Int(int i)
	{
		return call(i, ud);
	}
	bool Uint(unsigned u)
	{
		// I don't know why but without this, it triggers -Wsign-compare on gcc
		size_t max_size = std::numeric_limits<int>::max();
		if(u > max_size)
		{
			serrf("number too large, max: %zu, result: %u\n", max_size, u);
			return false;
		}
		return call(static_cast<int>(u), ud);
	}
};

class internal_uint32_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_uint32_json_handler>
{
public:
	BS_uint32_cb call;
	void* ud;

	explicit internal_uint32_json_handler(BS_uint32_cb cb, void* ud_)
	: call(cb)
	, ud(ud_)
	{
	}
	bool Default()
	{
		serr("expected uint\n");
		return false;
	}
	bool Uint(unsigned u)
	{
		return call(u, ud);
	}
};

class internal_int64_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_int64_json_handler>
{
public:
	BS_int64_cb call;
	void* ud;

	explicit internal_int64_json_handler(BS_int64_cb cb, void* ud_)
	: call(cb)
	, ud(ud_)
	{
	}
	bool Default()
	{
		serr("expected int64\n");
		return false;
	}
	bool Int(int i)
	{
		return call(i, ud);
	}
	bool Uint(unsigned u)
	{
		return call(u, ud);
	}
	bool Int64(int64_t i)
	{
		return call(i, ud);
	}
	bool Uint64(uint64_t u)
	{
		if(u <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
		{
			return call(static_cast<int64_t>(u), ud);
		}
		serrf(
			"number too large, max: %" PRId64 ", result: %" PRIu64 "\n",
			std::numeric_limits<int64_t>::max(),
			u);
		return false;
	}
};

class internal_uint64_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_uint64_json_handler>
{
public:
	BS_uint64_cb call;
	void* ud;

	explicit internal_uint64_json_handler(BS_uint64_cb cb, void* ud_)
	: call(cb)
	, ud(ud_)
	{
	}
	bool Default()
	{
		serr("expected uint64\n");
		return false;
	}
	bool Uint(unsigned u)
	{
		return call(u, ud);
	}
	bool Uint64(uint64_t u)
	{
		return call(u, ud);
	}
};

template<class Callback>
class internal_double_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_double_json_handler<Callback>>
{
public:
	Callback call;

	explicit internal_double_json_handler(Callback cb)
	: call(cb)
	{
	}
	bool Default()
	{
		serr("expected double\n");
		return false;
	}
// I am tempted to disable this, just for strictness.
#if 1
	bool Int(int i)
	{
		return call(i);
	}
	bool Uint(unsigned u)
	{
		return call(u);
	}
#endif
	bool Double(double d)
	{
		return call(d);
	}
};

template<class T>
class internal_uint_promote_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_uint_promote_json_handler<T>>
{
public:
	typedef bool (*Callback)(T, void*);
	Callback call;
	void* ud;

	explicit internal_uint_promote_json_handler(Callback cb, void* ud_)
	: call(cb)
	, ud(ud_)
	{
		static_assert(std::numeric_limits<T>::max() <= std::numeric_limits<unsigned>::max());
		static_assert(std::numeric_limits<T>::min() == std::numeric_limits<unsigned>::min());
	}
	bool Default()
	{
		serr("expected unsigned number\n");
		return false;
	}
	bool Uint(unsigned u)
	{
		if(u <= std::numeric_limits<T>::max())
		{
			return call(static_cast<T>(u), ud);
		}
		serrf(
			"number too large, max: %zu, result: %u\n",
			static_cast<size_t>(std::numeric_limits<T>::max()),
			u);
		return false;
	}
};

template<class T>
class internal_int_promote_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_int_promote_json_handler<T>>
{
public:
	typedef bool (*Callback)(T, void*);
	Callback call;
	void* ud;

	explicit internal_int_promote_json_handler(Callback cb, void* ud_)
	: call(cb)
	, ud(ud_)
	{
		static_assert(std::numeric_limits<T>::max() <= std::numeric_limits<int>::max());
	}
	bool Default()
	{
		serr("expected unsigned number\n");
		return false;
	}
	bool Int(int i)
	{
		if(i > std::numeric_limits<T>::max())
		{
			serrf("number too large, max: %d, result: %d\n", std::numeric_limits<T>::max(), i);
			return false;
		}
		if(i < std::numeric_limits<T>::min())
		{
			serrf("number too small, min: %d, result: %d\n", std::numeric_limits<T>::min(), i);
			return false;
		}
		return call(static_cast<T>(i), ud);
	}
	bool Uint(unsigned u)
	{
		if(u > static_cast<unsigned>(std::numeric_limits<T>::max()))
		{
			serrf("number too large, max: %u, result: %u\n", std::numeric_limits<T>::max(), u);
			return false;
		}
		return call(static_cast<T>(u), ud);
	}
};

class internal_string_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_string_json_handler>
{
public:
	BS_string_cb call;
	void* ud;
	size_t max_size;

	explicit internal_string_json_handler(BS_string_cb cb, void* ud_, size_t max_size_)
	: call(cb)
	, ud(ud_)
	, max_size(max_size_)
	{
	}
	bool Default()
	{
		serr("expected string\n");
		return false;
	}
	bool String(const char* str, rj::SizeType length, bool)
	{
		if(length <= max_size)
		{
			return call(str, length, ud);
		}
		serrf("string too large, max: %zu, size: %zu\n", max_size, static_cast<size_t>(length));
		return false;
	}
};

class internal_data_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_data_json_handler>
{
public:
	BS_string_cb call;
	void* ud;
	size_t max_size;

	explicit internal_data_json_handler(BS_string_cb cb, void* ud_, size_t max_size_)
	: call(cb)
	, ud(ud_)
	, max_size(max_size_)
	{
	}
	bool Default()
	{
		serr("expected data\n");
		return false;
	}
	bool String(const char* str, rj::SizeType length, bool)
	{
		std::string tmp = b64decode(str, length);
		if(tmp.size() <= max_size)
		{
			return call(tmp.data(), tmp.size(), ud);
		}
		serrf("string too large, max: %zu size: %zu\n", max_size, tmp.size());
		return false;
	}
};

class internal_key_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_key_json_handler>
{
public:
	const char* s;
	size_t size;

	explicit internal_key_json_handler(const char* s_, size_t size_)
	: s(s_)
	, size(size_)
	{
	}
	bool Default()
	{
		serrf("expected key: \"%.*s\"\n", static_cast<int>(size), s);
		return false;
	}
	bool Key(const char* str, rj::SizeType length, bool)
	{
		if(length == size && strncmp(str, s, size) == 0)
		{
			return true;
		}
		serrf(
			"mismatching keys expected: \"%.*s\", result: \"%.*s\"\n",
			static_cast<int>(length),
			str,
			static_cast<int>(size),
			s);
		return false;
	}
};

class internal_startobject_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_startobject_json_handler>
{
public:
	bool Default()
	{
		serr("expected '{'\n");
		return false;
	}
	bool StartObject()
	{
		return true;
	}
};

class internal_endobject_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_endobject_json_handler>
{
public:
	bool Default()
	{
		serr("expected '}'\n");
		return false;
	}
	bool EndObject(rj::SizeType)
	{
		return true;
	}
};

class internal_startarray_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_startarray_json_handler>
{
public:
	bool Default()
	{
		serr("expected '['\n");
		return false;
	}
	bool StartArray()
	{
		return true;
	}
};

class internal_endarray_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_endarray_json_handler>
{
public:
	bool Default()
	{
		serr("expected ']'\n");
		return false;
	}
	bool EndArray(rj::SizeType)
	{
		return true;
	}
};

class internal_null_json_handler
: public rj::BaseReaderHandler<rj::UTF8<>, internal_null_json_handler>
{
public:
	bool Default()
	{
		serr("expected null\n");
		return false;
	}
	bool Null()
	{
		return true;
	}
};

template<class InputStream>
class BS_JsonReader : public BS_Archive
{
public:
	rj::Reader reader;
	InputStream& stream;

	explicit BS_JsonReader(InputStream& stream_)
	: stream(stream_)
	{
		// this wont exist with rapidjson from package managers
		// this requires the newest version of rapidjson from github.
		// it does not help that rapidjson has not icremented their version number,
		// so 1.1.0 could be the package version, but 1.1.0 is also the github version...
		reader.IterativeParseInit();
	}

	bool IsReader() const override
	{
		return true;
	}
	bool IsWriter() const override
	{
		return false;
	}

	bool Good() const override
	{
		return !reader.HasParseError();
	}
	bool Finish(const char* info) override
	{
		info = (info == NULL) ? "<unspecified>" : info;
		if(reader.HasParseError())
		{
			serrf(
				"Failed to parse json: %s\n"
				"RapidJson Error: %s\n"
				"Size: %zu\n"
				"Offset: %zu\n",
				info,
				GetParseError_En(reader.GetParseErrorCode()),
				stream.Size(),
				reader.GetErrorOffset());
			// rewind the stream to print the offending line
			stream.Rewind();
			print_json_error(stream, reader.GetErrorOffset());
			return false;
		}

		if(!reader.IterativeParseComplete())
		{
			serrf(
				"Failed to parse json: %s\n"
				"Error: incomplete json\n"
				"Size: %zu\n"
				"Cursor: %zu\n",
				info,
				stream.Size(),
				stream.Tell());
			return false;
		}
		return true;
	}

	bool Null() override
	{
		if(reader.HasParseError()) return false;
		internal_null_json_handler handler;
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool Bool(bool& b) override
	{
		return Bool_CB({}, internal_read_simple_cb<bool>, &b);
	}
	bool Bool_CB(bool b, BS_bool_cb cb, void* ud) override
	{
		(void)b; // unused
		if(reader.HasParseError()) return false;
		internal_bool_json_handler handler(cb, ud);
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}

	bool Int8(int8_t& i) override
	{
		return Int8_CB({}, internal_read_simple_cb<int8_t>, &i);
	}
	bool Int8_CB(int8_t i, BS_int8_cb cb, void* ud) override
	{
		(void)i; // unused
		if(reader.HasParseError()) return false;
		internal_int_promote_json_handler<int8_t> handler(cb, ud);
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool Int16(int16_t& i) override
	{
		return Int16_CB({}, internal_read_simple_cb<int16_t>, &i);
	}
	bool Int16_CB(int16_t i, BS_int16_cb cb, void* ud) override
	{
		(void)i; // unused
		if(reader.HasParseError()) return false;
		internal_int_promote_json_handler<int16_t> handler(cb, ud);
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool Int32(int32_t& i) override
	{
		return Int32_CB({}, internal_read_simple_cb<int32_t>, &i);
	}
	bool Int32_CB(int32_t i, BS_int32_cb cb, void* ud) override
	{
		(void)i; // unused
		if(reader.HasParseError()) return false;
		internal_int32_json_handler handler(cb, ud);
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool Int64(int64_t& i) override
	{
		return Int64_CB({}, internal_read_simple_cb<int64_t>, &i);
	}
	bool Int64_CB(int64_t i, BS_int64_cb cb, void* ud) override
	{
		(void)i; // unused
		if(reader.HasParseError()) return false;
		internal_int64_json_handler handler(cb, ud);
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}

	bool Uint8(uint8_t& u) override
	{
		return Uint8_CB({}, internal_read_simple_cb<uint8_t>, &u);
	}
	bool Uint8_CB(uint8_t u, BS_uint8_cb cb, void* ud) override
	{
		(void)u; // unused
		if(reader.HasParseError()) return false;
		internal_uint_promote_json_handler<uint8_t> handler(cb, ud);
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool Uint16(uint16_t& u) override
	{
		return Uint16_CB({}, internal_read_simple_cb<uint16_t>, &u);
	}
	bool Uint16_CB(uint16_t u, BS_uint16_cb cb, void* ud) override
	{
		(void)u; // unused
		if(reader.HasParseError()) return false;
		internal_uint_promote_json_handler<uint16_t> handler(cb, ud);
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool Uint32(uint32_t& u) override
	{
		return Uint32_CB({}, internal_read_simple_cb<uint32_t>, &u);
	}
	bool Uint32_CB(uint32_t u, BS_uint32_cb cb, void* ud) override
	{
		(void)u; // unused
		if(reader.HasParseError()) return false;
		internal_uint32_json_handler handler(cb, ud);
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool Uint64(uint64_t& u) override
	{
		return Uint64_CB({}, internal_read_simple_cb<uint64_t>, &u);
	}
	bool Uint64_CB(uint64_t u, BS_uint64_cb cb, void* ud) override
	{
		(void)u; // unused
		if(reader.HasParseError()) return false;
		internal_uint64_json_handler handler(cb, ud);
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}

	bool Float(float& d) override
	{
		return Float_CB({}, internal_read_simple_cb<float>, &d);
	}
	bool Float_CB(float d, BS_float_cb cb, void* ud) override
	{
		(void)d; // unused
		if(reader.HasParseError()) return false;
		internal_double_json_handler handler([cb, ud](double d_) {
			if(!std::isfinite(d_))
			{
				serrf("invalid float: %f\n", d_);
				return false;
			}
			return cb(static_cast<float>(d_), ud);
		});
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool Double(double& d) override
	{
		return Double_CB({}, internal_read_simple_cb<double>, &d);
	}
	bool Double_CB(double d, BS_double_cb cb, void* ud) override
	{
		(void)d; // unused
		if(reader.HasParseError()) return false;
		internal_double_json_handler handler([cb, ud](double d_) {
			if(!std::isfinite(d_))
			{
				serrf("invalid double: %f\n", d_);
				return false;
			}
			return cb(d_, ud);
		});
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}

	bool String(std::string& str) override
	{
		return StringZ_CB(str, BS_MAX_STRING_SIZE, internal_read_string_cb, &str);
	}
	bool String_CB(std::string_view str, BS_string_cb cb, void* ud) override
	{
		return StringZ_CB(str, BS_MAX_STRING_SIZE, cb, ud);
	}
	bool StringZ(std::string& str, size_t max_size) override
	{
		return StringZ_CB(str, max_size, internal_read_string_cb, &str);
	}
	bool StringZ_CB(std::string_view str, size_t max_size, BS_string_cb cb, void* ud) override
	{
		(void)str;
		if(reader.HasParseError()) return false;
		internal_string_json_handler handler{cb, ud, max_size};
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}

	// can't use string because json cannot store binary (0-255)
	bool Data(std::string& str) override
	{
		return Data_CB(str, internal_read_string_cb, &str);
	}
	bool Data_CB(std::string_view str, BS_string_cb cb, void* ud) override
	{
		(void)str;
		if(reader.HasParseError()) return false;
		internal_data_json_handler handler{cb, ud, BS_MAX_STRING_SIZE};
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool DataZ(std::string& str, size_t max_size) override
	{
		return DataZ_CB(str, max_size, internal_read_string_cb, &str);
	}
	bool DataZ_CB(std::string_view str, size_t max_size, BS_string_cb cb, void* ud) override
	{
		(void)str;
		if(reader.HasParseError()) return false;
		internal_data_json_handler handler{cb, ud, max_size};
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}

	// json only
	bool Key(std::string_view str) override
	{
		(void)str;
		if(reader.HasParseError()) return false;
		internal_key_json_handler handler(str.data(), str.size());
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool StartObject() override
	{
		if(reader.HasParseError()) return false;
		internal_startobject_json_handler handler;
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool EndObject() override
	{
		if(reader.HasParseError()) return false;
		internal_endobject_json_handler handler;
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool StartArray() override
	{
		if(reader.HasParseError()) return false;
		internal_startarray_json_handler handler;
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
	bool EndArray() override
	{
		if(reader.HasParseError()) return false;
		internal_endarray_json_handler handler;
		return reader.IterativeParseNext<rj::kParseCommentsFlag>(stream, handler);
	}
};

// you could set JsonOutput to rj::StringBuffer
template<class OutputStream, class JsonFormat = rj::PrettyWriter<OutputStream>>
class BS_JsonWriter : public BS_Archive
{
public:
	JsonFormat writer;
	bool good = true;

	explicit BS_JsonWriter(OutputStream& output)
	: writer(output)
	{
	}

	bool IsReader() const override
	{
		return false;
	}
	bool IsWriter() const override
	{
		return true;
	}
	bool Good() const override
	{
		return good;
	}
	bool Finish(const char* info) override
	{
		info = (info == NULL) ? "<unspecified>" : info;
		if(!writer.IsComplete())
		{
			serrf("Failed to write json: %s\n", info);
			return false;
		}
		return good;
	}
	bool Null() override
	{
		good = good && CHECK(writer.Null());
		return good;
	}
	bool Bool(bool& b) override
	{
		good = good && CHECK(writer.Bool(b));
		return good;
	}
	bool Bool_CB(bool b, BS_bool_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		good = good && CHECK(writer.Bool(b));
		return good;
	}
	bool Int8(int8_t& i) override
	{
		good = good && CHECK(writer.Int(i));
		return good;
	}
	bool Int8_CB(int8_t i, BS_int8_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		good = good && CHECK(writer.Int(i));
		return good;
	}
	bool Int16(int16_t& i) override
	{
		good = good && CHECK(writer.Int(i));
		return good;
	}
	bool Int16_CB(int16_t i, BS_int16_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		good = good && CHECK(writer.Int(i));
		return good;
	}
	bool Int32(int32_t& i) override
	{
		good = good && CHECK(writer.Int(i));
		return good;
	}
	bool Int32_CB(int32_t i, BS_int32_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		good = good && CHECK(writer.Int(i));
		return good;
	}
	bool Int64(int64_t& i) override
	{
		good = good && CHECK(writer.Int64(i));
		return good;
	}
	bool Int64_CB(int64_t i, BS_int64_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		good = good && CHECK(writer.Int64(i));
		return good;
	}
	bool Uint8(uint8_t& u) override
	{
		good = good && CHECK(writer.Uint(u));
		return good;
	}
	bool Uint8_CB(uint8_t u, BS_uint8_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		good = good && CHECK(writer.Uint(u));
		return good;
	}
	bool Uint16(uint16_t& u) override
	{
		good = good && CHECK(writer.Uint(u));
		return good;
	}
	bool Uint16_CB(uint16_t u, BS_uint16_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		good = good && CHECK(writer.Uint(u));
		return good;
	}
	bool Uint32(uint32_t& u) override
	{
		good = good && CHECK(writer.Uint(u));
		return good;
	}
	bool Uint32_CB(uint32_t u, BS_uint32_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		good = good && CHECK(writer.Uint(u));
		return good;
	}
	bool Uint64(uint64_t& u) override
	{
		good = good && CHECK(writer.Uint64(u));
		return good;
	}
	bool Uint64_CB(uint64_t u, BS_uint64_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		good = good && CHECK(writer.Uint64(u));
		return good;
	}
	bool Float(float& d) override
	{
		good = good && CHECK(writer.Double(d));
		return good;
	}
	bool Float_CB(float d, BS_float_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		good = good && CHECK(writer.Double(d));
		return good;
	}
	bool Double(double& d) override
	{
		good = good && CHECK(writer.Double(d));
		return good;
	}
	bool Double_CB(double d, BS_double_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		good = good && CHECK(writer.Double(d));
		return good;
	}
	bool String(std::string& str) override
	{
		return StringZ_CB(str, BS_MAX_STRING_SIZE, NULL, NULL);
	}
	bool String_CB(std::string_view str, BS_string_cb cb, void* ud) override
	{
		return StringZ_CB(str, BS_MAX_STRING_SIZE, cb, ud);
	}
	bool StringZ(std::string& str, size_t max_size) override
	{
		return StringZ_CB(str, max_size, NULL, NULL);
	}
	bool StringZ_CB(std::string_view str, size_t max_size, BS_string_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		(void)max_size;
		ASSERT(max_size <= BS_MAX_STRING_SIZE);
		ASSERT(str.size() <= max_size);
		good = good && CHECK(writer.String(str.data(), str.size()));
		return good;
	}
	bool Data(std::string& str) override
	{
		return DataZ_CB(str, BS_MAX_STRING_SIZE, NULL, NULL);
	}
	bool Data_CB(std::string_view str, BS_string_cb cb, void* ud) override
	{
		return DataZ_CB(str, BS_MAX_STRING_SIZE, cb, ud);
	}
	bool DataZ(std::string& str, size_t max_size) override
	{
		return DataZ_CB(str, max_size, NULL, NULL);
	}
	bool DataZ_CB(std::string_view str, size_t max_size, BS_string_cb cb, void* ud) override
	{
		(void)cb;
		(void)ud;
		(void)max_size;
		ASSERT(max_size <= BS_MAX_STRING_SIZE);
		ASSERT(str.size() <= max_size);
		std::string tmp = base64_encode(str.data(), str.size());
		good = good && CHECK(writer.String(tmp.data(), tmp.size()));
		return good;
	}
	bool Key(std::string_view str) override
	{
		good = good && CHECK(writer.Key(str.data(), str.size()));
		return good;
	}

	bool StartObject() override
	{
		good = good && CHECK(writer.StartObject());
		return good;
	}
	bool EndObject() override
	{
		good = good && CHECK(writer.EndObject());
		return good;
	}
	bool StartArray() override
	{
		good = good && CHECK(writer.StartArray());
		return good;
	}
	bool EndArray() override
	{
		good = good && CHECK(writer.EndArray());
		return good;
	}
};
#endif