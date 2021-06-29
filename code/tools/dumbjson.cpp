#include "../global.h"
#include "dumbjson.h"

#include "RWops.h"

#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>

//TODO (dootsie): make the way how PrintError and Get/Check functions print messages more similar.

const char* g_kTypeNames[] = {"Null", "False", "True", "Object", "Array", "String", "Number"};

const char* g_unspecified_filename_default = "<unspecified>";

const char* rj_string(const rj::Value& value)
{
	switch(value.GetType())
	{
	case rj::kTrueType:
	case rj::kFalseType: return "Bool";
	case rj::kNullType:
	case rj::kStringType:
	case rj::kObjectType:
	case rj::kArrayType: return g_kTypeNames[value.GetType()];

	case rj::kNumberType:
		if(value.IsDouble())
		{
			return "Double";
		}
		else if(value.IsInt())
		{
			return "Int";
		}
		else if(value.IsUint())
		{
			return "Uint";
		}
		else
		{
			return "64bit number probably";
		}
	}
	return "Unknown";
}

// This is 99% copy pasted from rapidjson::FileReadStream, but modified for use by RWops.

// Tencent is pleased to support the open source community by making RapidJSON available.
//
// Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip.
//
// Licensed under the MIT License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
class RWops_JsonReadStream
{
  public:
	typedef char Ch; //!< Character type (byte).

	//! Constructor.
	/*!
		\param fp File pointer opened for read.
		\param buffer user-supplied buffer.
		\param bufferSize size of buffer in bytes. Must >=4 bytes.
	*/
	RWops_JsonReadStream(RWops* fp, char* buffer, size_t bufferSize)
	: fp_(fp)
	, buffer_(buffer)
	, bufferSize_(bufferSize)
	, bufferLast_(0)
	, current_(buffer_)
	, readCount_(0)
	, count_(0)
	, eof_(false)
	{
		ASSERT(fp_ != 0);
		ASSERT(bufferSize >= 4);
		Read();
	}

	Ch Peek() const
	{
		return *current_;
	}
	Ch Take()
	{
		Ch c = *current_;
		Read();
		return c;
	}
	size_t Tell() const
	{
		return count_ + (current_ - buffer_);
	}

	// Not implemented
	void Put(Ch)
	{
		ASSERT(false);
	}
	void Flush()
	{
		ASSERT(false);
	}
	Ch* PutBegin()
	{
		ASSERT(false);
		return 0;
	}
	size_t PutEnd(Ch*)
	{
		ASSERT(false);
		return 0;
	}

	// For encoding detection only.
	const Ch* Peek4() const
	{
		return (current_ + 4 - static_cast<int>(!eof_) <= bufferLast_) ? current_ : 0;
	}

  private:
	void Read()
	{
		if(current_ < bufferLast_)
			++current_;
		else if(!eof_)
		{
			count_ += readCount_;
			readCount_ = fp_->read(buffer_, 1, bufferSize_);
			bufferLast_ = buffer_ + readCount_ - 1;
			current_ = buffer_;

			if(readCount_ < bufferSize_)
			{
				buffer_[readCount_] = '\0';
				++bufferLast_;
				eof_ = true;
			}
		}
	}

	RWops* fp_;
	Ch* buffer_;
	size_t bufferSize_;
	Ch* bufferLast_;
	Ch* current_;
	size_t readCount_;
	size_t count_; //!< Number of characters read
	bool eof_;
};

// this is copy pasted from rapidjson::FileWriteStream

class RWops_JsonWriteStream
{
  public:
	typedef char Ch; //!< Character type. Only support char.

	RWops_JsonWriteStream(RWops* fp, char* buffer, size_t bufferSize)
	: fp_(fp)
	, buffer_(buffer)
	, bufferEnd_(buffer + bufferSize)
	, current_(buffer_)
	{
		ASSERT(fp_ != 0);
	}

	void Put(char c)
	{
		if(current_ >= bufferEnd_) Flush();

		*current_++ = c;
	}

	void PutN(char c, size_t n)
	{
		size_t avail = (bufferEnd_ - current_);
		while(n > avail)
		{
			std::memset(current_, c, avail);
			current_ += avail;
			Flush();
			n -= avail;
			avail = static_cast<size_t>(bufferEnd_ - current_);
		}

		if(n > 0)
		{
			std::memset(current_, c, n);
			current_ += n;
		}
	}

	void Flush()
	{
		if(current_ != buffer_)
		{
			size_t result = fp_->write(buffer_, 1, static_cast<size_t>(current_ - buffer_));
			if(result < static_cast<size_t>(current_ - buffer_))
			{
				// failure deliberately ignored at this time
				// added to avoid warn_unused_result build errors
			}
			current_ = buffer_;
		}
	}

	// Not implemented
	char Peek() const
	{
		ASSERT(false);
		return 0;
	}
	char Take()
	{
		ASSERT(false);
		return 0;
	}
	size_t Tell() const
	{
		ASSERT(false);
		return 0;
	}
	char* PutBegin()
	{
		ASSERT(false);
		return 0;
	}
	size_t PutEnd(char*)
	{
		ASSERT(false);
		return 0;
	}

  private:
	RWops* fp_;
	char* buffer_;
	char* bufferEnd_;
	char* current_;
};

RAPIDJSON_NAMESPACE_BEGIN
//! Implement specialized version of PutN() with memset() for better performance.
template<>
inline void PutN(RWops_JsonWriteStream& stream, char c, size_t n)
{
	stream.PutN(c, n);
}
RAPIDJSON_NAMESPACE_END

bool JsonState::open_file(RWops* file, const char* info, rj::Type expected)
{
	ASSERT(file != NULL);
	ASSERT(info != NULL || file->stream_info != NULL);
	ASSERT(file->good());

	clear();

	file_info = (info == NULL ? file->stream_info : info);

	char buffer[1024];
	RWops_JsonReadStream isw(file, buffer, sizeof(buffer));

	// rj::kParseCommentsFlag, rj::UTF8<> is to allow comments
	if(rjdoc.ParseStream<rj::kParseCommentsFlag, rj::UTF8<>>(isw).HasParseError())
	{
		size_t offset = rjdoc.GetErrorOffset();
		serrf(
			"Failed to parse json: `%s`\n"
			"Message: %s\n"
			"Offset: %zu\n",
			file_info,
			GetParseError_En(rjdoc.GetParseError()),
			offset);

		if(!file->good())
		{
			serrf("bad file stream\n");
			return false;
		}

		// reset the file cursor.
		if(!CHECK(file->seek(0, SEEK_SET) != -1))
		{
			return false;
		}

		// TODO (dootsie): I really want to move this out into another function,
		// but I also have a very similar situation in open_string
		// which needs a function as well, but I can't reduce copy-paste easily...
		// and as much as it's tempting to just copy the file memory into a buffer
		// and use the same code as open_string, that would be not ideal.

		// find the offending line:
		size_t line_n = 0; // should be 1 indexed
		size_t file_position = 0;
		size_t start_position = 0;

		size_t bytes_read;
		while((bytes_read = file->read(buffer, 1, sizeof(buffer))) > 0)
		{
			char* last = buffer + bytes_read;
			char* pos = buffer;
			do
			{
				char* last_pos = pos;
				pos = std::find(pos, last, '\n');

				// go over the newline for the next std::find
				pos = std::min(pos + 1, last);

				file_position += (pos - last_pos);

				if(offset < file_position)
				{
					serrf(
						"Line: %zu\n"
						"Col: %zu\n",
						line_n + 1,
						(offset - start_position) + 1);
					size_t line_length = file_position - start_position;

					line_length = std::min(line_length, sizeof(buffer) - 1);

					if(offset - start_position > sizeof(buffer) - 1)
					{
						serrf("offset too far to print line.\n");
						return false;
					}

					if(!CHECK(file->seek(static_cast<int>(start_position), SEEK_SET) != -1))
					{
						return false;
					}
					if(!CHECK(file->read(buffer, 1, line_length) == line_length))
					{
						return false;
					}
					// clear any control values (mainly for windows \r\n)
					char* line_end =
						std::remove_if(buffer, buffer + line_length, [](unsigned char _1) {
							return _1 != '\n' && _1 != '\t' && (_1 < 32 || _1 == 127);
						});

					*line_end = '\0';
					// print the line.
					serrf(">>>%s\n", buffer);
					// I could print a tiny arrow to the column, but it doesn't work good with utf8
					return false;
				}

				if(pos != last)
				{
					++line_n;
					start_position = file_position;
				}
			} while(pos != last);
		}
		ASSERT(false && "unreachable?");
		return false;
	}

	if(rjdoc.GetType() != expected)
	{
		serrf(
			"JsonState::%s Error in `%s`: Failed to convert Root [%s], expected [%s] \n",
			__func__,
			file_info,
			rj_string(rjdoc),
			g_kTypeNames[expected]);
		return false;
	}
	return true;
}

bool JsonState::open_string(
	const char* buffer, size_t buffer_size, const char* info, rj::Type expected)
{
	ASSERT(buffer != NULL);
	ASSERT(buffer_size != 0);

	clear();

	file_info = (info == NULL ? g_unspecified_filename_default : info);

	if(rjdoc.Parse<rj::kParseCommentsFlag, rj::UTF8<>>(buffer, buffer_size).HasParseError())
	{
		size_t offset = rjdoc.GetErrorOffset();
		serrf(
			"Failed to parse json: `%s`\n"
			"Message: %s\n"
			"Offset: %zu\n",
			file_info,
			GetParseError_En(rjdoc.GetParseError()),
			offset);

		// find the offending line:
		size_t line_n = 1; // should be 1 indexed
		size_t line_start_position = 0;
		const char* last = buffer + buffer_size;
		const char* pos = buffer;

		do
		{
			pos = std::find(pos, last, '\n');
			size_t file_position = (pos - buffer);
			++pos; // go over the newline for the next std::find
			if(offset <= file_position)
			{
				serrf(
					"Line: %zu\n"
					"Col: %zu\n",
					line_n,
					(offset - line_start_position));

				// file_position should point to the location of '\n', which means it will be
				// trimmed off.
				size_t line_length = file_position - line_start_position;

				// copy because I need to modify the contents (null terminator).
				// I also don't want an absolutely gigantic line printed.
				char copy_buffer[1024];

				// needs to be less than the buffer (+ null terminator)
				line_length = std::min(line_length, sizeof(copy_buffer) - 1);

				if(offset - line_start_position > line_length)
				{
					serrf("offset too far to print line.\n");
					return false;
				}

				const char* line_start = buffer + line_start_position;
				std::copy_n(line_start, line_length, copy_buffer);

				// clear any control values (mainly for windows \r\n)
				char* line_end =
					std::remove_if(copy_buffer, copy_buffer + line_length, [](unsigned char _1) {
						return _1 != '\n' && _1 != '\t' && (_1 < 32 || _1 == 127);
					});

				*line_end = '\0';
				// print the line.
				serrf(">>>%s\n", copy_buffer);
				// I could print a tiny arrow to the column, but it doesn't work good with utf8
				return false;
			}
			++line_n;
			line_start_position = file_position + 1;
		} while(pos != last);
		ASSERT(false && "unreachable?");
	}

	if(rjdoc.GetType() != expected)
	{
		serrf(
			"JsonState::%s Error in `%s`: Failed to convert Root [%s], expected [%s] \n",
			__func__,
			file_info,
			rj_string(rjdoc),
			g_kTypeNames[expected]);
		return false;
	}
	return true;
}

bool JsonState::write_file(RWops* file, const char* info)
{
	ASSERT(file != NULL);
	ASSERT(info != NULL || file->stream_info != NULL);
	ASSERT(file->good());

	info = (info == NULL ? file->stream_info : info);

	char buffer[1024];
	RWops_JsonWriteStream osw(file, buffer, sizeof(buffer));
	rj::PrettyWriter<RWops_JsonWriteStream> writer(osw);
	if(!rjdoc.Accept(writer) || !file->good())
	{
		serrf("Failed to write json: `%s`\n", info);
		return false;
	}
	return true;
}

bool JsonState::write_string(rj::StringBuffer& buffer, const char* info)
{
	ASSERT(info != NULL || file_info != NULL);

	info = (info == NULL ? file_info : info);

	rj::PrettyWriter<rj::StringBuffer> writer(buffer);
	if(!rjdoc.Accept(writer))
	{
		serrf("Failed to write json: `%s`\n", info);
		return false;
	}
	return true;
}

void JsonState::create(rj::Type type)
{
	clear();
	file_info = NULL;
	static_cast<rj::Value&>(rjdoc) = rj::Value(type);
}

void JsonState::rename(const char* info)
{
	file_info = (info == NULL ? g_unspecified_filename_default : info);
}

void JsonState::clear()
{
	rjdoc.SetNull();
	rjdoc.GetAllocator().Clear();
	json_unwind_table.clear();
}

void JsonState::PrintError(const char* message, ...)
{
	va_list args;
	va_start(args, message);
	std::unique_ptr<char[]> buffer = unique_vasprintf(NULL, message, args);
	va_end(args);

	serrf("Error: at `%s` in `%s`: %s\n", dump_path().c_str(), file_info, buffer.get());
}
void JsonState::PrintMemberError(const char* key, const char* message, ...)
{
	va_list args;
	va_start(args, message);
	std::unique_ptr<char[]> buffer = unique_vasprintf(NULL, message, args);
	va_end(args);

	if(json_unwind_table.empty())
	{
		// root will include a period before the key...
		serrf("Error: at `%s` in `%s`: %s\n", key, file_info, buffer.get());
	}
	else
	{
		serrf("Error: at `%s.%s` in `%s`: %s\n", dump_path().c_str(), key, file_info, buffer.get());
	}
}
void JsonState::PrintIndexError(size_t index, const char* message, ...)
{
	va_list args;
	va_start(args, message);
	std::unique_ptr<char[]> buffer = unique_vasprintf(NULL, message, args);
	va_end(args);

	serrf("Error: at `%s[%zu]` in `%s`: %s\n", dump_path().c_str(), index, file_info, buffer.get());
}

// prints the path of the current Json(Member/Index)(Writer/Reader) stack.
std::string JsonState::dump_path()
{
	// not efficent by any means, but errors aren't efficent.
	std::string path;
	for(auto it = json_unwind_table.begin(); it != json_unwind_table.end(); ++it)
	{
		if(it->name != NULL)
		{
			// if at the start, ignore the root.
			if(it != json_unwind_table.begin())
			{
				path += '.';
			}
			path.append(it->name);
		}
		else
		{
			++it;
			ASSERT(it != json_unwind_table.end() && "bad stack");
			path += '[';
			path += std::to_string(it->index);
			path += ']';
		}
	}
	return path;
}

void JsonState::internal_print_missing_member_error(const char* function, const char* key)
{
	if(json_unwind_table.empty())
	{
		// root will include a period before the key...
		serrf("JsonState::%s Error in `%s`: No such node \"%s\"\n", function, file_info, key);
	}
	else
	{
		serrf(
			"JsonState::%s Error in `%s`: No such node \"%s.%s\"\n",
			function,
			file_info,
			dump_path().c_str(),
			key);
	}
}
void JsonState::internal_print_missing_index_error(const char* function, size_t index, size_t size)
{
	serrf(
		"JsonState::%s Error in `%s`: index out of bounds \"%s[%zu]\" size=%zu\n",
		function,
		file_info,
		dump_path().c_str(),
		index,
		size);
}

void JsonState::internal_print_member_convert_error(
	const char* function, const rj::Value::ConstMemberIterator& mitr, const char* expected)
{
	if(json_unwind_table.empty())
	{
		// root will include a period before the key...
		serrf(
			"JsonState::%s Error in `%s`: Failed to convert key: \"%s\", value: %c%s%c, expected: [%s]\n",
			function,
			file_info,
			mitr->name.GetString(),
			(mitr->value.IsString() ? '\"' : '['),
			(mitr->value.IsString() ? mitr->value.GetString() : rj_string(mitr->value)),
			(mitr->value.IsString() ? '\"' : ']'),
			expected);
	}
	else
	{
		serrf(
			"JsonState::%s Error in `%s`: Failed to convert key: \"%s.%s\", value: %c%s%c, expected: [%s]\n",
			function,
			file_info,
			dump_path().c_str(),
			mitr->name.GetString(),
			(mitr->value.IsString() ? '\"' : '['),
			(mitr->value.IsString() ? mitr->value.GetString() : rj_string(mitr->value)),
			(mitr->value.IsString() ? '\"' : ']'),
			expected);
	}
}
void JsonState::internal_print_index_convert_error(
	const char* function,
	size_t index,
	const rj::Value::ConstValueIterator& value,
	const char* expected)
{
	serrf(
		"JsonState::%s Error in `%s`: Failed to convert index: \"%s[%zu]\", value: %c%s%c, expected: [%s]\n",
		function,
		file_info,
		dump_path().c_str(),
		index,
		(value->IsString() ? '\"' : '['),
		(value->IsString() ? value->GetString() : rj_string(*value)),
		(value->IsString() ? '\"' : ']'),
		expected);
}
void JsonState::internal_print_array_size_error(
	const char* function, size_t array_size, size_t expected_size)
{
	serrf(
		"JsonState::%s Error at `%s` in `%s`: incorrect array size: %zu, expected: %zu\n",
		function,
		file_info,
		dump_path().c_str(),
		array_size,
		expected_size);
}
