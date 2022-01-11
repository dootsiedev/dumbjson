#include "global.h"
#include "tools/BS_stream.h"
//#include "tools/kson_serializer.h"
//#include "tools/dumb_json.h"
#include "3rd_party/utf8/core.hpp"

#include "tools/RWops.h"


#include <SDL2/SDL.h>

#if 0

static int test_object_1(char* file_memory, size_t& file_size)
{
	const char* key_object = "test_object";

	const char* key_double = "some_double";
	const double expected_double = 123;

	const char* key_int = "some_int";
	const int expected_int = 456;

	const char* key_bool = "some_bool";
	const bool expected_bool = false;

	const char* key_string = "some_string";
	const std::string expected_string = "string";

	const char* key_cstring = "some_cstring";
	const char* expected_cstring = "cstring";
	{
		JsonState json_state;
		// this will set the root object to be empty.
		// rj::kObjectType is optional and will asumme an object if blank.
		json_state.create(rj::kObjectType);
		{
			{
				auto test_object = rj::Value(rj::kObjectType);
				{
					const auto& rjobject = test_object.GetObject();
					// add to the test object
					json_state.AddMember(rjobject, rj::StringRef(key_double), expected_double);
					json_state.AddMember(rjobject, rj::StringRef(key_int), expected_int);
					json_state.AddMember(rjobject, rj::StringRef(key_bool), expected_bool);
					json_state.AddMember(
						rjobject, rj::StringRef(key_string), rj::StringRef(expected_string));
					json_state.AddMember(
						rjobject, rj::StringRef(key_cstring), rj::StringRef(expected_cstring));
				}

				// move to the root (test_object is Null after this)
				json_state.AddMember(
					json_state.rjdoc.GetObject(), rj::StringRef(key_object), test_object);
			}
		}

		// writing
		// Unique_RWops file = RWops_OpenFS("path.json", "wb");
		Unique_RWops file = RWops_FromMemory(file_memory, file_size, __func__);
		if(!file) return -1;
		if(!json_state.write_file(file.get())) return -1;
		int get_file_size;
		if((get_file_size = file->tell()) == -1) return -1;
		file_size = get_file_size;
	}
	{
		JsonState json_state;

		// Unique_RWops file = RWops_OpenFS("path.json", "rb");
		Unique_RWops file = RWops_FromMemory_ReadOnly(file_memory, file_size, __func__);
		if(!file) return -1;

		// this will also check if root is an object.
		if(!json_state.open_file(file.get())) return -1;

		file.reset();

		{
			// root has a nested object.
			{
				// get an object, check if it exists and if it's the correct type.
				auto mitr = json_state.CheckMember(
					json_state.rjdoc.GetObject(), key_object, rj::kObjectType);
				if(mitr == json_state.rjdoc.MemberEnd()) return -1;

				// put it in the wrapper for good errors,
				// call .finish() if you can't call the destructor before moving to the next object.
				JsonMemberReader test_object(mitr, json_state);
				const auto& rjobject = mitr->value.GetObject();

				{
					double result;
					if(!json_state.GetMember(rjobject, key_double, result)) return -1;
					if(result != expected_double)
					{
						json_state.PrintMemberError(key_double, "incorrect value");
						return -1;
					}
				}
				{
					int result;
					if(!json_state.GetMember(rjobject, key_int, result)) return -1;
					if(result != expected_int)
					{
						json_state.PrintMemberError(key_int, "incorrect value");
						return -1;
					}
				}
				{
					bool result;
					if(!json_state.GetMember(rjobject, key_bool, result)) return -1;
					if(result != expected_bool)
					{
						json_state.PrintMemberError(key_bool, "incorrect value");
						return -1;
					}
				}
				{
					// this is how you get a string with the length.
					// you could just use the more simple C-string approach.
					auto str_mitr = json_state.CheckMember(rjobject, key_string, rj::kStringType);
					if(str_mitr == rjobject.MemberEnd()) return -1;
					if(rj::StringRef(expected_string) != str_mitr->value)
					{
						json_state.PrintMemberError(key_string, "incorrect value");
						return -1;
					}
				}
				{
					const char* result;
					if(!json_state.GetMember(rjobject, key_cstring, result)) return -1;
					if(strcmp(result, expected_cstring) != 0)
					{
						json_state.PrintMemberError(key_cstring, "incorrect value");
						return -1;
					}
				}
			}
		}
	}
	return 0;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static int test_array_1(char* file_memory, size_t& file_size)
{
	const char* key_double = "some_double";
	const double expected_double[] = {
		0.111,
		222,
		0,
		444.444,
		std::numeric_limits<double>::max(),
		std::numeric_limits<double>::min()};

	const char* key_int = "some_int";
	const int expected_int[] = {
		444, -555, 0, 666, std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};

	const char* key_bool = "some_bool";
	const bool expected_bool[] = {false, true, false, true};

	const char* key_string = "some_string";
	const std::string expected_string[] = {"aaaaa", "BBBBB", "", "\n"};

	const char* key_cstring = "some_cstring";
	const char* expected_cstring[] = {"aaaaa", "BBBBB", "", "\n"};
	{
		JsonState json_state;
		// this will set the root object to be empty.
		json_state.create();
		{
			const auto& rjroot = json_state.rjdoc.GetObject();
			{
				auto test_array = rj::Value(rj::kArrayType);
				json_state.SetArrayRange(
					test_array.GetArray(),
					expected_double,
					expected_double + std::size(expected_double));
				json_state.AddMember(rjroot, rj::StringRef(key_double), test_array);
			}
			{
				auto test_array = rj::Value(rj::kArrayType);
				json_state.SetArrayRange(
					test_array.GetArray(), expected_int, expected_int + std::size(expected_int));
				json_state.AddMember(rjroot, rj::StringRef(key_int), test_array);
			}
			{
				auto test_array = rj::Value(rj::kArrayType);
				json_state.SetArrayRange(
					test_array.GetArray(), expected_bool, expected_bool + std::size(expected_bool));
				json_state.AddMember(rjroot, rj::StringRef(key_bool), test_array);
			}
			{
				auto test_array = rj::Value(rj::kArrayType);
				{
					// this alternative way is required for strings.
					const auto& rjarray = test_array.GetArray();
					for(const auto& val : expected_string)
					{
						rjarray.PushBack(rj::StringRef(val), json_state.rjdoc.GetAllocator());
					}
				}
				json_state.AddMember(rjroot, rj::StringRef(key_string), test_array);
			}
			{
				auto test_array = rj::Value(rj::kArrayType);
				{
					const auto& rjarray = test_array.GetArray();
					for(const auto& val : expected_cstring)
					{
						rjarray.PushBack(rj::StringRef(val), json_state.rjdoc.GetAllocator());
					}
				}
				json_state.AddMember(rjroot, rj::StringRef(key_cstring), test_array);
			}
		}

		// writing
		// Unique_RWops file = RWops_OpenFS("path.json", "wb");
		Unique_RWops file = RWops_FromMemory(file_memory, file_size, "json write");
		if(!file) return -1;
		if(!json_state.write_file(file.get())) return -1;
		int get_file_size;
		if((get_file_size = file->tell()) == -1) return -1;
		file_size = get_file_size;
	}
	{
		JsonState json_state;

		// Unique_RWops file = RWops_OpenFS("path.json", "rb");
		Unique_RWops file = RWops_FromMemory_ReadOnly(file_memory, file_size, "json read");
		if(!file) return -1;

		// this will also check if root is an object.
		if(!json_state.open_file(file.get())) return -1;
		file.reset();
		{
			const auto& rjroot = json_state.rjdoc.GetObject();
			{
				auto mitr = json_state.CheckMember(rjroot, key_double, rj::kArrayType);
				if(mitr == rjroot.MemberEnd()) return -1;
				JsonMemberReader test_array(mitr, json_state);

				double result[std::size(expected_double)];

				if(!json_state.GetArrayRange(
					   mitr->value.GetArray(), result, result + std::size(result)))
					return -1;

				for(size_t i = 0; i < std::size(result); ++i)
				{
					if(result[i] != expected_double[i])
					{
						json_state.PrintIndexError(i, "incorrect value");
						return -1;
					}
				}
			}
			{
				auto mitr = json_state.CheckMember(rjroot, key_int, rj::kArrayType);
				if(mitr == rjroot.MemberEnd()) return -1;
				JsonMemberReader test_array(mitr, json_state);

				int result[std::size(expected_int)];

				if(!json_state.GetArrayRange(
					   mitr->value.GetArray(), result, result + std::size(result)))
					return -1;

				for(size_t i = 0; i < std::size(result); ++i)
				{
					if(result[i] != expected_int[i])
					{
						json_state.PrintIndexError(i, "incorrect value");
						return -1;
					}
				}
			}
			{
				auto mitr = json_state.CheckMember(rjroot, key_bool, rj::kArrayType);
				if(mitr == rjroot.MemberEnd()) return -1;
				JsonMemberReader test_array(mitr, json_state);

				bool result[std::size(expected_bool)];

				if(!json_state.GetArrayRange(
					   mitr->value.GetArray(), result, result + std::size(result)))
					return -1;

				for(size_t i = 0; i < std::size(result); ++i)
				{
					if(result[i] != expected_bool[i])
					{
						json_state.PrintIndexError(i, "incorrect value");
						return -1;
					}
				}
			}
			{
				// note that you can use GetArrayRange for const char*,
				// but this is the only way how to get the length of the string.
				auto mitr = json_state.CheckMember(rjroot, key_string, rj::kArrayType);
				if(mitr == rjroot.MemberEnd()) return -1;
				JsonMemberReader test_array(mitr, json_state);

				const auto& rjarray = mitr->value.GetArray();

				if(rjarray.Size() != std::size(expected_string))
				{
					json_state.PrintError("unexpected array size");
					return -1;
				}

				for(size_t i = 0; i < std::size(expected_string); ++i)
				{
					auto* rjstring = json_state.CheckIndex(rjarray, i, rj::kStringType);
					if(rjstring == NULL) return -1;

					if(rj::StringRef(expected_string[i]) != *rjstring)
					{
						json_state.PrintIndexError(i, "incorrect value");
						return -1;
					}
				}
			}
			{
				auto mitr = json_state.CheckMember(rjroot, key_cstring, rj::kArrayType);
				if(mitr == rjroot.MemberEnd()) return -1;
				JsonMemberReader test_array(mitr, json_state);

				const char* result[std::size(expected_cstring)];

				if(!json_state.GetArrayRange(
					   mitr->value.GetArray(), result, result + std::size(result)))
					return -1;

				for(size_t i = 0; i < std::size(result); ++i)
				{
					if(strcmp(result[i], expected_cstring[i]) != 0)
					{
						json_state.PrintIndexError(i, "incorrect value");
						return -1;
					}
				}
			}
		}
	}
	return 0;
}

static const char* data_key_i = "i";
static const char* data_key_d = "d";
static const char* data_key_s = "s";
struct data_type
{
	int i;
	double d;
	std::string s;

	rj::Value write(JsonState& json_state) const
	{
		rj::Value object(rj::kObjectType);
		const auto& rjobject = object.GetObject();
		json_state.AddMember(rjobject, rj::StringRef(data_key_i), i);
		json_state.AddMember(rjobject, rj::StringRef(data_key_d), d);
		json_state.AddMember(rjobject, rj::StringRef(data_key_s), rj::StringRef(s));
		return object;
	}

	bool read(JsonState& json_state, const rj::Value::Object& object)
	{
		// for batches of members, I prefer showing all the missing values, instead of failing fast.
		bool success = true;
		success = success && json_state.GetMember(object, data_key_i, i);
		success = success && json_state.GetMember(object, data_key_d, d);
		{
			// strings... (much more simple to just convert a c-string to std::string)
			auto str_mitr = json_state.CheckMember(object, data_key_s, rj::kStringType);
			if(str_mitr == object.MemberEnd())
			{
				success = false;
			}
			else
			{
				s = std::string(str_mitr->value.GetString(), str_mitr->value.GetStringLength());
			}
		}
		return success;
	}

	template<class Config>
	bool serialize(Config&& ar);

	template<class Config>
	bool serialize(Config&& ar) const
	{
		// A hack, but if we are writing, there should be no problem.
		static_assert(Config::IsWriter);
		return const_cast<data_type*>(this)->serialize(ar);
	}

	template<class Archive>
	bool kson_serialize(Archive& ar);

	bool Serialize(BS_Archive& ar);

	bool operator==(const data_type& rhs) const
	{
		return i == rhs.i && d == rhs.d && s == rhs.s;
	}
	bool operator!=(const data_type& rhs) const
	{
		return !(*this == rhs);
	}
};

static int test_array_of_objects_1(char* file_memory, size_t& file_size)
{
	const char* key_array = "test_array";

	const data_type expected_array[] = {{1, 1.1, "aaa"}, {2, 2.2, "bbb"}, {3, 3.3, "ccc"}};

	{
		JsonState json_state;
		json_state.create();
		{
			{
				auto test_array = rj::Value(rj::kArrayType);
				const auto& rjarray = test_array.GetArray();
				for(const data_type& test_data : expected_array)
				{
					rjarray.PushBack(test_data.write(json_state), json_state.rjdoc.GetAllocator());
				}
				json_state.AddMember(
					json_state.rjdoc.GetObject(), rj::StringRef(key_array), test_array);
			}
		}

		// writing
		// Unique_RWops file = RWops_OpenFS("path.json", "wb");
		Unique_RWops file = RWops_FromMemory(file_memory, file_size, __func__);
		if(!file) return -1;
		if(!json_state.write_file(file.get())) return -1;
		int get_file_size;
		if((get_file_size = file->tell()) == -1) return -1;
		file_size = get_file_size;
	}
	{
		JsonState json_state;

		// Unique_RWops file = RWops_OpenFS("path.json", "rb");
		Unique_RWops file = RWops_FromMemory_ReadOnly(file_memory, file_size, __func__);
		if(!file) return -1;

		// this will also check if root is an object.
		if(!json_state.open_file(file.get())) return -1;

		file.reset();

		{
			const auto& rjroot = json_state.rjdoc.GetObject();
			{
				// get an object, check if it exists and if it's the correct type.
				auto mitr = json_state.CheckMember(rjroot, key_array, rj::kArrayType);
				if(mitr == rjroot.MemberEnd()) return -1;

				JsonMemberReader test_array(mitr, json_state);
				const auto& rjarray = mitr->value.GetArray();

				size_t test_array_size = rjarray.Size();

				if(test_array_size != std::size(expected_array))
				{
					json_state.PrintError("array has incorrect size");
					return -1;
				}

				for(size_t i = 0; i < test_array_size; ++i)
				{
					auto* vitr = json_state.CheckIndex(rjarray, i, rj::kObjectType);
					if(vitr == NULL) return -1;
					JsonIndexReader test_object(i, json_state);

					data_type result;

					if(!result.read(json_state, vitr->GetObject()))
					{
						return -1;
					}

					if(expected_array[i] != result)
					{
						// NOTE: this is bad because I have no idea which value is wrong.
						json_state.PrintError("incorrect value");
						return -1;
					}
				}
			}
		}
	}
	return 0;
}

template<class Config>
bool data_type::serialize(Config&& ar)
{
	bool success = true;
	success = success && ar.Member(rj::StringRef(data_key_i), i);
	success = success && ar.Member(rj::StringRef(data_key_d), d);

	success = success && ar.MemberString(rj::StringRef(data_key_s), s);
#if 0
	size_t sz = s.size();
	success = success && ar.MemberStringRef(rj::StringRef(data_key_s), s.c_str(), &sz);
	const char* text = "text";
	success = success && ar.MemberStringRef(rj::StringRef(data_key_s), text);
#endif

	return success;
}

static int test_array_of_objects_2(char* file_memory, size_t& file_size)
{
	const char* key_array = "test_array";

	const data_type expected_array[] = {{1, 1.1, "aaa"}, {2, 2.2, "bbb"}, {3, 3.3, "ccc"}};

	{
		JsonState json_state;
		json_state.create();
		{
			{
				auto test_array = rj::Value(rj::kArrayType);
				const auto& rjarray = test_array.GetArray();
				for(const data_type& test_data : expected_array)
				{
					rj::Value object(rj::kObjectType);
					const auto& rjobject = object.GetObject();
					test_data.serialize(JsonConfigWriter(rjobject, json_state));
					rjarray.PushBack(object, json_state.rjdoc.GetAllocator());
				}
				json_state.AddMember(
					json_state.rjdoc.GetObject(), rj::StringRef(key_array), test_array);
			}
		}

		// writing
		// Unique_RWops file = RWops_OpenFS("path.json", "wb");
		Unique_RWops file = RWops_FromMemory(file_memory, file_size, __func__);
		if(!file) return -1;
		if(!json_state.write_file(file.get())) return -1;
		int get_file_size;
		if((get_file_size = file->tell()) == -1) return -1;
		file_size = get_file_size;
	}
	{
		JsonState json_state;

		// this will also check if root is an object.
		if(!json_state.open_string(file_memory, file_size)) return -1;

#if 0
		// Unique_RWops file = RWops_OpenFS("path.json", "rb");
		Unique_RWops file = RWops_FromMemory_ReadOnly(file_memory, file_size, __func__);
		if(!file) return -1;

		// this will also check if root is an object.
		if(!json_state.open_file(file.get())) return -1;

		file.reset();
#endif
		{
			const auto& rjroot = json_state.rjdoc.GetObject();
			{
				// get an object, check if it exists and if it's the correct type.
				auto mitr = json_state.CheckMember(rjroot, key_array, rj::kArrayType);
				if(mitr == rjroot.MemberEnd()) return -1;

				JsonMemberReader test_array(mitr, json_state);
				const auto& rjarray = mitr->value.GetArray();

				size_t test_array_size = rjarray.Size();

				if(test_array_size != std::size(expected_array))
				{
					json_state.PrintError("array has incorrect size");
					return -1;
				}

				for(size_t i = 0; i < test_array_size; ++i)
				{
					auto* vitr = json_state.CheckIndex(rjarray, i, rj::kObjectType);
					if(vitr == NULL) return -1;
					JsonIndexReader test_object(i, json_state);

					data_type result;
					const auto& rjobject = vitr->GetObject();
					if(!result.serialize(JsonConfigReader(rjobject, json_state)))
					{
						return -1;
					}

					if(expected_array[i] != result)
					{
						// NOTE: this is bad because I have no idea which value is wrong.
						json_state.PrintError("incorrect value");
						return -1;
					}
				}
			}
		}
	}
	return 0;
}

template<class Archive>
bool data_type::kson_serialize(Archive& ar)
{
	if(!ar.StartObject())
	{
		// exit early
		return false;
	}
	ar.Key("i");
	ar.Int(i);
	ar.Key("d");
	ar.Double(d);

	ar.Key("s");
	// ar.String(s);
	size_t test = 3;

	struct custom_string_wrapper
	{
		std::string& s;
		explicit custom_string_wrapper(std::string& s_)
		: s(s_)
		{
		}
		bool operator()(const char* str, size_t size)
		{
			if(!utf8::is_valid(str, str + size))
			{
				// you can use \xa0\xa1 to check
				serr("invalid utf8\n");
				return false;
			}
			s = std::string(str, size);
			return true;
		}
	};
	ar.String_CB(custom_string_wrapper{s}, s, test);
	ar.EndObject();
	return true;
}
template<class Archive>
bool kson_array_of_objects(Archive& ar, std::vector<data_type>& data)
{
	if(!ar.StartObject())
	{
		// exit early
		return false;
	}

	// this is a drawback of using kson,
	// which is the requirement of manually sized arrays.
	ar.Key("size");
	ASSERT(std::size(data) <= std::numeric_limits<uint16_t>::max());
	uint16_t test_array_size = static_cast<uint16_t>(std::size(data));

	if(!ar.Uint16_CB(kson_min_max_cb<uint16_t>{test_array_size, 3, 3}, test_array_size))
	{
		// exit early
		return false;
	}

	if(Archive::IsReader)
	{
		data.resize(test_array_size);
	}

	ar.Key("array");
	ar.StartArray();
	for(data_type& entry : data)
	{
		if(!entry.kson_serialize(ar))
		{
			return false;
		}
	}
	ar.EndArray();

	ar.EndObject();
	return true;
}

static const data_type kson_expected_array[] = {
	{-4, 1.1, "aaa"}, {2, 2.234567890123456789, "bb\n"}, {3, -1, ""}};

static int test_kson_json_stream(char* file_memory, size_t& file_size)
{
	{
		// copy the contents in.
		std::vector<data_type> dynamic_array(
			kson_expected_array, kson_expected_array + std::size(kson_expected_array));

		Unique_RWops file = RWops_FromMemory(file_memory, file_size, __func__);
		if(!file) return -1;

		if(!kson_write_json_stream(
			   [&dynamic_array](auto& ar) -> bool {
				   return kson_array_of_objects(ar, dynamic_array);
			   },
			   file.get()))
		{
			return -1;
		}

		int cursor = file->tell();
		if(cursor < 0)
		{
			return -1;
		}
		file_size = cursor;
		file_memory[file_size] = '\0';
	}
	{
		std::vector<data_type> result;

		Unique_RWops file = RWops_FromMemory_ReadOnly(file_memory, file_size, __func__);
		if(!file) return -1;
		if(!kson_read_json_stream(
			   [&result](auto& ar) -> bool { return kson_array_of_objects(ar, result); },
			   file.get()))
		{
			return -1;
		}

		if(result.size() != std::size(kson_expected_array))
		{
			serrf(
				"mismatching array, expected: %zu, result: %zu\n",
				std::size(kson_expected_array),
				result.size());
			return -1;
		}
		for(size_t i = 0; i < std::size(kson_expected_array); ++i)
		{
			if(result.at(i) != kson_expected_array[i])
			{
				serrf("mismatching entry at: %zu\n", i);
				return -1;
			}
		}
	}

	return 0;
}

static int test_kson_json_memory(char* file_memory, size_t& file_size)
{
	{
		// copy the contents in.
		std::vector<data_type> dynamic_array(
			kson_expected_array, kson_expected_array + std::size(kson_expected_array));

		KsonStringBuffer sb;

		if(!kson_write_json_memory(
			   [&dynamic_array](auto& ar) -> bool {
				   return kson_array_of_objects(ar, dynamic_array);
			   },
			   sb,
			   __func__))
		{
			return -1;
		}

		file_size = (sb.GetLength() > file_size) ? file_size : sb.GetLength();
		memcpy(file_memory, sb.GetString(), file_size);
		file_memory[file_size] = '\0';
	}
	{
		std::vector<data_type> result;

		if(!kson_read_json_memory(
			   [&result](auto& ar) -> bool { return kson_array_of_objects(ar, result); },
			   file_memory,
			   file_size,
			   __func__))
		{
			return -1;
		}

		if(result.size() != std::size(kson_expected_array))
		{
			serrf(
				"mismatching array, expected: %zu, result: %zu\n",
				std::size(kson_expected_array),
				result.size());
			return -1;
		}
		for(size_t i = 0; i < std::size(kson_expected_array); ++i)
		{
			if(result.at(i) != kson_expected_array[i])
			{
				serrf("mismatching entry at: %zu\n", i);
				return -1;
			}
		}
	}

	return 0;
}

static int test_kson_binary_stream(char* file_memory, size_t& file_size)
{
	size_t old_file_size = file_size;
	{
		rj::StringBuffer sb;
		// copy the contents in.
		std::vector<data_type> dynamic_array(
			kson_expected_array, kson_expected_array + std::size(kson_expected_array));

		Unique_RWops file = RWops_FromMemory(file_memory, file_size, __func__);
		if(!file) return -1;

		if(!kson_write_binary_stream(
			   [&dynamic_array](auto& ar) -> bool {
				   return kson_array_of_objects(ar, dynamic_array);
			   },
			   file.get()))
		{
			return -1;
		}

		int cursor = file->tell();
		if(cursor < 0)
		{
			return -1;
		}
		file_size = cursor;
		file_memory[file_size] = '\0';
	}
	{
		std::vector<data_type> result;

		Unique_RWops file = RWops_FromMemory_ReadOnly(file_memory, file_size, __func__);
		if(!file) return -1;
		if(!kson_read_binary_stream(
			   [&result](auto& ar) -> bool { return kson_array_of_objects(ar, result); },
			   file.get()))
		{
			return -1;
		}

		if(result.size() != std::size(kson_expected_array))
		{
			serrf(
				"mismatching array, expected: %zu, result: %zu\n",
				std::size(kson_expected_array),
				result.size());
			return -1;
		}
		for(size_t i = 0; i < std::size(kson_expected_array); ++i)
		{
			if(result.at(i) != kson_expected_array[i])
			{
				serrf("mismatching entry at: %zu\n", i);
				return -1;
			}
		}
	}
#if 1
	std::string tmp = base64_encode(file_memory, file_size);
	file_size = (tmp.size() > old_file_size) ? old_file_size : tmp.size();
	memcpy(file_memory, tmp.c_str(), file_size);
	file_memory[file_size] = '\0';
#else
	(void)old_file_size;
#endif

	return 0;
}

static int test_kson_binary_memory(char* file_memory, size_t& file_size)
{
	size_t old_file_size = file_size;
	{
		KsonStringBuffer sb;
		// copy the contents in.
		std::vector<data_type> dynamic_array(
			kson_expected_array, kson_expected_array + std::size(kson_expected_array));

		if(!kson_write_binary_memory(
			   [&dynamic_array](auto& ar) -> bool {
				   return kson_array_of_objects(ar, dynamic_array);
			   },
			   sb,
			   __func__))
		{
			return -1;
		}

		file_size = (sb.GetLength() > file_size ? file_size : sb.GetLength());
		memcpy(file_memory, sb.GetString(), file_size);
		file_memory[file_size] = '\0';
	}
	{
		std::vector<data_type> result;

		if(!kson_read_binary_memory(
			   [&result](auto& ar) -> bool { return kson_array_of_objects(ar, result); },
			   file_memory,
			   file_size,
			   __func__))
		{
			return -1;
		}

		if(result.size() != std::size(kson_expected_array))
		{
			serrf(
				"mismatching array, expected: %zu, result: %zu\n",
				std::size(kson_expected_array),
				result.size());
			return -1;
		}
		for(size_t i = 0; i < std::size(kson_expected_array); ++i)
		{
			if(result.at(i) != kson_expected_array[i])
			{
				serrf("mismatching entry at: %zu\n", i);
				return -1;
			}
		}
	}

#if 1
	std::string tmp = base64_encode(file_memory, file_size);
	file_size = (tmp.size() > old_file_size) ? old_file_size : tmp.size();
	memcpy(file_memory, tmp.c_str(), file_size);
	file_memory[file_size] = '\0';
#else
	(void)old_file_size;
#endif

	return 0;
}

static int test_read_1(char* file_memory, size_t& file_size)
{
	// I am not doing anything meaningful here,
	// I used this to give garbage and to make sure the error is good.
	JsonState json_state;

	{
		char temp_file[] = R"(
//this is a comment
{
	/*another one!*/
   "null":null})";
		size_t temp_size = strlen(temp_file);
		ASSERT(file_size > temp_size);
		/*
		Unique_RWops file = RWops_FromMemory_ReadOnly(temp_file, temp_size, __func__);
		if(!file) return -1;

		// this will also check if root is an object.
		if(!json_state.open_file(file.get())) return -1;

		file.reset();
		*/

		if(!json_state.open_string(temp_file, temp_size, __func__)) return -1;

		{
			auto mitr = json_state.CheckMember(json_state.rjdoc.GetObject(), "null", rj::kNullType);
			if(mitr == json_state.rjdoc.MemberEnd()) return -1;
		}
	}

	{
		// I just write the file out as a way of testing.
		// actually testing would be better, but I feel like previous tests should work.
		Unique_RWops file = RWops_FromMemory(file_memory, file_size, __func__);
		if(!file) return -1;
		if(!json_state.write_file(file.get())) return -1;

		int get_file_size;
		if((get_file_size = file->tell()) == -1) return -1;
		file_size = get_file_size;
	}

	return 0;
}

struct serialize_kson
{
	template<class T>
	bool operator()(T& ar)
	{
		ar.StartObject();
		ar.Key("null");
		ar.Null();
		ar.EndObject();
		return true;
	}
};

static int test_read_kson(char* file_memory, size_t& file_size)
{
	char temp_file[] = R"(
//this is a comment
{
	/*another one!*/
   "null":null
   })";
	size_t temp_size = strlen(temp_file);
	ASSERT(file_size > temp_size);

	file_size = (temp_size > file_size) ? file_size : temp_size;
	memcpy(file_memory, temp_file, file_size);
	file_memory[file_size] = '\0';

#if 0
	if(!kson_read_json_memory(
		   [](auto& ar) -> bool {
			   ar.StartObject();
			   ar.Key("null");
			   ar.Null();
			   return false;
			   ar.EndObject();
			   return true;
		   },
		   file_memory,
		   file_size,
		   __func__))
#endif
	if(!kson_read_json_memory(serialize_kson{}, file_memory, file_size, __func__))
	{
		return -1;
	}

	return 0;
}
#endif

// disabled because of the warnings on release, and wall of text
#if 0
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static int test_error_1(char* file_memory, size_t& file_size)
{
	// just test to see if the correct error is made.

	JsonState json_state;

	{
		char temp_file[] = R"(
{
	"node":
	{
		"i":1,
		"arr":[1]
	},
	"array_node":
	[
		{
			"s":"a"
		}
	]
})";
		size_t temp_size = strlen(temp_file);
		ASSERT(file_size > temp_size);

		{
			Unique_RWops file = RWops_FromMemory_ReadOnly(temp_file, temp_size, __func__);
			if(!file) return -1;

			// this will also check if root is an object.
			if(!json_state.open_file(file.get())) return -1;

			file.reset();
		}

		{
			// I just write the file out, for sanity.
			Unique_RWops file = RWops_FromMemory(file_memory, file_size, __func__);
			if(!file) return -1;
			if(!json_state.write_file(file.get())) return -1;

			int get_file_size;
			if((get_file_size = file->tell()) == -1) return -1;
			file_size = get_file_size;
		}

		{
			const char* key_object = "node";
			const char* key_object_int = "i";
			const char* key_object_array = "arr";

			const char* key_array = "array_node";
			const char* key_array_string = "s";

			{
				// check that all the values are correct, for sanity
				const auto& rjroot = json_state.rjdoc.GetObject();
				{
					// get an object, check if it exists and if it's the correct type.
					auto mitr = json_state.CheckMember(rjroot, key_object, rj::kObjectType);
					if(mitr == rjroot.MemberEnd()) return -1;

					// put it in the wrapper for good errors.
					JsonMemberReader test_object(mitr, json_state);
					const auto& rjobject = mitr->value.GetObject();

					{
						int result;
						if(!json_state.GetMember(rjobject, key_object_int, result)) return -1;
						if(result != 1)
						{
							json_state.PrintMemberError(key_object_int, "incorrect value 1");
							return -1;
						}
					}
					{
						// technically ints should be doubles too.
						double result;
						if(!json_state.GetMember(rjobject, key_object_int, result)) return -1;
						if(result != 1.0)
						{
							json_state.PrintMemberError(key_object_int, "incorrect value 1");
							return -1;
						}
					}
					{
						// auto
						mitr = json_state.CheckMember(rjobject, key_object_array, rj::kArrayType);
						if(mitr == rjobject.MemberEnd()) return -1;
						JsonMemberReader test_array(mitr, json_state);

						{
							int result[1] = {0};

							if(!json_state.GetArrayRange(
								   mitr->value.GetArray(), result, result + std::size(result)))
							{
								return -1;
							}
							if(result[0] != 1)
							{
								json_state.PrintIndexError(0, "incorrect value != 1");
								return -1;
							}
						}
						{
							// technically ints should be doubles too.
							double result[1] = {0.0};

							if(!json_state.GetArrayRange(
								   mitr->value.GetArray(), result, result + std::size(result)))
							{
								return -1;
							}
							if(result[0] != 1.0)
							{
								json_state.PrintIndexError(0, "incorrect value != 1");
								return -1;
							}
						}
					}
				}
				{
					auto mitr = json_state.CheckMember(rjroot, key_array, rj::kArrayType);
					if(mitr == rjroot.MemberEnd()) return -1;
					JsonMemberReader test_array(mitr, json_state);

					const auto& rjarray = mitr->value.GetArray();

					size_t test_array_size = rjarray.Size();

					if(test_array_size != 1)
					{
						json_state.PrintError("array has incorrect size != 1");
						return -1;
					}

					// for(size_t i = 0; i < test_array_size; ++i)

					auto* vitr = json_state.CheckIndex(rjarray, 0, rj::kObjectType);
					if(vitr == NULL) return -1;
					JsonIndexReader test_object(0, json_state);

					const char* result;

					if(!json_state.GetMember(vitr->GetObject(), key_array_string, result))
					{
						return -1;
					}

					if(strcmp(result, "a") != 0)
					{
						json_state.PrintMemberError(key_array_string, "incorrect value != \"a\"");
						return -1;
					}
				}
			}

			{
				// now start checking the errors.
				json_state.PrintError("PrintError!");
				ASSERT(!serr_get_error().empty());
				json_state.FormatError("FormatError, with %s!", "vargs");
				ASSERT(!serr_get_error().empty());

				json_state.PrintMemberError("nonexistant", "PrintMemberError Error!");
				ASSERT(!serr_get_error().empty());
				json_state.FormatMemberError(
					"nonexistant", "FormatMemberError Error with %s!", "vargs");
				ASSERT(!serr_get_error().empty());

				json_state.PrintIndexError(123, "PrintIndexError Error!");
				ASSERT(!serr_get_error().empty());
				json_state.FormatIndexError(123, "FormatIndexError Error with %s!", "vargs");
				ASSERT(!serr_get_error().empty());

				const auto& rjroot = json_state.rjdoc.GetObject();

				// things that don't exist
				{
					slogf("\"%s\" does not exist\n", key_object_int);
					int result = 2;
					ASSERT(!json_state.GetMember(rjroot, key_object_int, result));
					ASSERT(!serr_get_error().empty());
					ASSERT(result == 2);
				}
				{
					slogf("\"%s\" does not exist\n", key_object_array);
					ASSERT(
						json_state.CheckMember(rjroot, key_object_array, rj::kArrayType) ==
						rjroot.MemberEnd());
					ASSERT(!serr_get_error().empty());
				}
				{
					slogf("\"%s\" does not exist\n", key_array_string);
					const char* result_original = "2";
					const char* result = result_original;
					ASSERT(!json_state.GetMember(rjroot, key_array_string, result));
					ASSERT(!serr_get_error().empty());
					ASSERT(result == result_original);
				}
				// object
				{
					slogf("\"%s\" not an int\n", key_object);
					int result = 2;
					ASSERT(!json_state.GetMember(rjroot, key_object, result));
					ASSERT(!serr_get_error().empty());
					ASSERT(result == 2);
				}
				{
					slogf("\"%s\" not an array\n", key_object);
					ASSERT(
						json_state.CheckMember(rjroot, key_object, rj::kArrayType) ==
						rjroot.MemberEnd());
					ASSERT(!serr_get_error().empty());
				}
				{
					// get an object, check if it exists and if it's the correct type.
					auto mitr = json_state.CheckMember(rjroot, key_object, rj::kObjectType);
					if(mitr == rjroot.MemberEnd()) return -1;

					// put it in the wrapper for good errors.
					JsonMemberReader test_object(mitr, json_state);
					const auto& rjobject = mitr->value.GetObject();

					// things that don't exist
					{
						slogf("\"%s.%s\" does not exist\n", key_object, key_object);
						ASSERT(
							json_state.CheckMember(rjobject, key_object, rj::kObjectType) ==
							rjobject.MemberEnd());
						ASSERT(!serr_get_error().empty());
					}
					{
						slogf("\"%s.%s\" does not exist\n", key_object, key_array);
						ASSERT(
							json_state.CheckMember(rjobject, key_array, rj::kArrayType) ==
							rjobject.MemberEnd());
						ASSERT(!serr_get_error().empty());
					}
					{
						slogf("\"%s.%s\" does not exist\n", key_object, key_array_string);
						const char* result_original = "2";
						const char* result = result_original;
						ASSERT(!json_state.GetMember(rjobject, key_array_string, result));
						ASSERT(!serr_get_error().empty());
						ASSERT(result == result_original);
					}

					// int
					{
						slogf("\"%s.%s\" not an object\n", key_object, key_object_int);
						ASSERT(
							json_state.CheckMember(rjobject, key_object_int, rj::kObjectType) ==
							rjobject.MemberEnd());
						ASSERT(!serr_get_error().empty());
					}
					{
						slogf("\"%s.%s\" not an bool\n", key_object, key_object_int);
						bool result = true;
						ASSERT(!json_state.GetMember(rjobject, key_object_int, result));
						ASSERT(!serr_get_error().empty());
						ASSERT(result);
					}
					// array
					{
						slogf("\"%s.%s\" not an object\n", key_object, key_object_array);
						ASSERT(
							json_state.CheckMember(rjobject, key_object_array, rj::kObjectType) ==
							rjobject.MemberEnd());
						ASSERT(!serr_get_error().empty());
					}
					{
						slogf("\"%s.%s\" not an bool\n", key_object, key_object_array);
						bool result = true;
						ASSERT(!json_state.GetMember(rjobject, key_object_array, result));
						ASSERT(!serr_get_error().empty());
						ASSERT(result);
					}
					{
						// auto
						mitr = json_state.CheckMember(rjobject, key_object_array, rj::kArrayType);
						if(mitr == rjobject.MemberEnd()) return -1;
						JsonMemberReader test_array(mitr, json_state);
						const auto& rjarray = mitr->value.GetArray();
						{
							slogf(
								"array \"%s.%s\" not enough elements\n",
								key_object,
								key_object_array);
							int result[2] = {2, 3};
							ASSERT(!json_state.GetArrayRange(
								rjarray, result, result + std::size(result)));
							ASSERT(!serr_get_error().empty());
							ASSERT(result[0] == 2);
							ASSERT(result[1] == 3);
						}
						{
							slogf(
								"array \"%s.%s\" too many elements\n",
								key_object,
								key_object_array);
							ASSERT(!json_state.GetArrayRange(
								rjarray, (const char**)(NULL), (const char**)(NULL)));
							ASSERT(!serr_get_error().empty());
						}
						{
							slogf(
								"array \"%s.%s[0]\" contains not object\n",
								key_object,
								key_object_array);
							ASSERT(json_state.CheckIndex(rjarray, 0, rj::kObjectType) == NULL);
							ASSERT(!serr_get_error().empty());
						}
						{
							slogf(
								"array \"%s.%s[0]\" contains not string\n",
								key_object,
								key_object_array);
							const char* original_result = "2";
							const char* result[1] = {original_result};
							ASSERT(!json_state.GetArrayRange(
								rjarray, result, result + std::size(result)));
							ASSERT(!serr_get_error().empty());
							ASSERT(result[0] == original_result);
						}
						{
							slogf(
								"array \"%s.%s[0]\" contains not array\n",
								key_object,
								key_object_array);
							ASSERT(json_state.CheckIndex(rjarray, 0, rj::kArrayType) == NULL);
							ASSERT(!serr_get_error().empty());
						}
					}
				}
				{
					// array of objects
					auto mitr = json_state.CheckMember(rjroot, key_array, rj::kArrayType);
					if(mitr == rjroot.MemberEnd()) return -1;
					JsonMemberReader test_array(mitr, json_state);

					const auto& rjarray = mitr->value.GetArray();

					{
						slogf("array \"%s\" not enough elements\n", key_array);
						int result[2] = {2, 3};
						ASSERT(
							!json_state.GetArrayRange(rjarray, result, result + std::size(result)));
						ASSERT(!serr_get_error().empty());
						ASSERT(result[0] == 2);
						ASSERT(result[1] == 3);
					}
					{
						slogf("array \"%s\" too many elements\n", key_array);
						const char* wtf[1];
						ASSERT(!json_state.GetArrayRange(rjarray, wtf, wtf));
						ASSERT(!serr_get_error().empty());
					}
					{
						slogf("array \"%s[0]\" contains not bool\n", key_array);
						bool result[1] = {false};
						ASSERT(!json_state.GetArrayRange(rjarray, result, result + 1));
						ASSERT(!serr_get_error().empty());
						ASSERT(!result[0]);
					}
					{
						slogf("array \"%s[0]\" contains not array\n", key_array);
						ASSERT(json_state.CheckIndex(rjarray, 0, rj::kArrayType) == NULL);
						ASSERT(!serr_get_error().empty());
					}
					{
						slogf("array \"%s[0]\" contains not null\n", key_array);
						ASSERT(json_state.CheckIndex(rjarray, 0, rj::kNullType) == NULL);
						ASSERT(!serr_get_error().empty());
					}
					{
						size_t test_array_size = rjarray.Size();

						if(test_array_size != 1)
						{
							json_state.PrintError("array has incorrect size != 1");
							return -1;
						}
						// for(size_t i = 0; i < test_array_size; ++i)

						{
							auto* vitr = json_state.CheckIndex(rjarray, 0, rj::kObjectType);
							if(vitr == NULL) return -1;
							JsonIndexReader test_object2(0, json_state);
							const auto& rjobject2 = vitr->GetObject();

							// things that don't exist
							{
								slogf("\"%s[0].%s\" does not exist\n", key_array, key_object);
								ASSERT(
									json_state.CheckMember(
										rjobject2, key_object, rj::kObjectType) ==
									rjobject2.MemberEnd());
								ASSERT(!serr_get_error().empty());
							}
							{
								slogf("\"%s[0].%s\" does not exist\n", key_array, key_array);
								ASSERT(
									json_state.CheckMember(rjobject2, key_array, rj::kArrayType) ==
									rjobject2.MemberEnd());
								ASSERT(!serr_get_error().empty());
							}
							// string
							{
								slogf("\"%s[0].%s\" not bool\n", key_array, key_array_string);
								bool result = false;
								ASSERT(!json_state.GetMember(rjobject2, key_array_string, result));
								ASSERT(!serr_get_error().empty());
								ASSERT(!result);
							}
							{
								slogf("\"%s[0].%s\" not array\n", key_array, key_array_string);
								ASSERT(
									json_state.CheckMember(
										rjobject2, key_array_string, rj::kArrayType) ==
									rjobject2.MemberEnd());
								ASSERT(!serr_get_error().empty());
							}
						}
					}
				}
			}
		}
	}

	return 0;
}
#endif
/*struct serialize_kson : public BS_Serializable
{
	bool Serialize(BS_Archive* ar) override
	{
		ar->StartObject();
		ar->Key("null");
		ar->Null();
		ar->EndObject();
		return ar->Finish(NULL);
	}
};*/

struct bs_data
{
	bool b;

	int8_t i8;
	int16_t i16;
	int32_t i32;
	int64_t i64;

	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;

	float f;
	double d;

	std::string str;

	// void Serialize(BS_Archive& ar)
	template<class Archive>
	void Serialize(Archive& ar)
	{
		ar.StartObject();
		ar.Key("b");
		ar.Bool(b);

		ar.Key("i8");
		ar.Int8(i8);
		ar.Key("i16");
		ar.Int16(i16);
		ar.Key("i32");
		ar.Int32(i32);
		ar.Key("i64");
		ar.Int64(i64);

		ar.Key("u8");
		ar.Uint8(u8);
		ar.Key("u16");
		ar.Uint16(u16);
		ar.Key("u32");
		ar.Uint32(u32);
		ar.Key("u64");
		ar.Uint64(u64);

		ar.Key("f");
		ar.Float(f);
		ar.Key("d");
		ar.Double(d);

		ar.Key("str");
		ar.String(str);

		ar.EndObject();
	}

	// C++20 can avoid this with the default compare operator
	bool operator==(const bs_data& rhs) const
	{
		return b == rhs.b && i8 == rhs.i8 && i16 == rhs.i16 && i32 == rhs.i32 && i64 == rhs.i64 &&
			   u8 == rhs.u8 && u16 == rhs.u16 && u32 == rhs.u32 && u64 == rhs.u64 && d == rhs.d &&
			   f == rhs.f && str == rhs.str;
	}
	bool operator!=(const bs_data& rhs) const
	{
		return !(*this == rhs);
	}
};

// not part of bs_data because of constructor problems from unique_ptr.
struct bs_custom
{
	std::unique_ptr<char[]> custom;
	size_t custom_len = 0;
	std::string utf8;
	explicit bs_custom() = default;
	explicit bs_custom(std::string_view raw_, std::string utf8_)
	: utf8(std::move(utf8_))
	{
		raw_cb(raw_.data(), raw_.size());
	}

	bs_custom(const bs_custom& rhs)
	: utf8(rhs.utf8)
	{
		raw_cb(rhs.custom.get(), rhs.custom_len);
	}

	static bool shim_raw_cb(const char* data, size_t length, void* ud)
	{
		return reinterpret_cast<bs_custom*>(ud)->raw_cb(data, length);
	}
	bool raw_cb(const char* data, size_t length)
	{
		custom.reset(new char[length]);
		memcpy(custom.get(), data, length);
		custom_len = length;
		return true;
	}

	static bool shim_utf8_cb(const char* data, size_t length, void* ud)
	{
		return reinterpret_cast<bs_custom*>(ud)->utf8_cb(data, length);
	}
	bool utf8_cb(const char* data, size_t length)
	{
		if(!utf8::is_valid(data, data + length))
		{
			// you can use \xa0\xa1 to check
			serr("invalid utf8\n");
			return false;
		}
		utf8 = std::string(data, length);
		return true;
	}

	void Serialize(BS_Archive& ar)
	{
		ar.StartObject();

		ar.Key("raw");
		ar.String_CB(std::string_view(custom.get(), custom_len), shim_raw_cb, this);

		ar.Key("utf8");
		ar.String_CB(utf8, shim_utf8_cb, this);

		ar.EndObject();
	}

	bool operator==(const bs_custom& rhs) const
	{
		return custom_len == rhs.custom_len &&
			   memcmp(custom.get(), rhs.custom.get(), custom_len) == 0 && utf8 == rhs.utf8;
	}

	bool operator!=(const bs_custom& rhs) const
	{
		return !(*this == rhs);
	}
};

class TestSerialize : public BS_Serializable
{
public:
	std::vector<bs_data>& group_data;
	std::vector<bs_custom>& group_custom;

	explicit TestSerialize(std::vector<bs_data>& data_, std::vector<bs_custom>& custom_)
	: group_data(data_)
	, group_custom(custom_)
	{
	}

	void Serialize(BS_Archive& ar) override
	{
		T_Serialize(ar);
	}

	template<class Archive>
	void T_Serialize(Archive& ar)
	{
		ar.StartObject();

		// this is a drawback of using the BS_archive,
		// which is the requirement of manually storing the size of arrays.
		{
			const uint32_t min_size = 0;
			const uint32_t max_size = 4;

			// sanity check for writer.
			if(ar.IsWriter())
			{
				ASSERT(std::size(group_data) <= std::numeric_limits<uint32_t>::max());
				ASSERT(std::size(group_data) >= min_size);
				ASSERT(std::size(group_data) <= max_size);
			}

			// the value is overwritten if IsReader
			uint32_t test_array_size = std::size(group_data);

			BS_min_max_state<uint32_t> size_cb_state{test_array_size, min_size, max_size};

			ar.Key("size");
			if(!ar.Uint32_CB(test_array_size, decltype(size_cb_state)::call, &size_cb_state))
			{
				// exit early
				return;
			}

			if(ar.IsReader())
			{
				group_data.resize(test_array_size);
			}
		}

		ar.Key("data");
		ar.StartArray();
		for(bs_data& entry : group_data)
		{
			if(!ar.Good())
			{
				// exit early
				return;
			}
			entry.Serialize(ar);
		}
		ar.EndArray();

		{
			// sanity check for writer.
			if(ar.IsWriter())
			{
				ASSERT(std::size(group_custom) <= std::numeric_limits<uint32_t>::max());
			}

			uint32_t custom_array_size = std::size(group_custom);

			ar.Key("custom_size");
			if(!ar.Uint32(custom_array_size))
			{
				// exit early
				return;
			}

			if(ar.IsReader())
			{
				group_custom.resize(custom_array_size);
			}
		}

		ar.Key("custom");
		ar.StartArray();
		for(bs_custom& entry : group_custom)
		{
			if(!ar.Good())
			{
				// exit early
				return;
			}
			entry.Serialize(ar);
		}
		ar.EndArray();

		ar.EndObject();
	}

	bool check(
		const bs_data* data_begin,
		const bs_data* data_end,
		const bs_custom* custom_begin,
		const bs_custom* custom_end)
	{
		size_t data_size = data_end - data_begin;
		if(group_data.size() != data_size)
		{
			serrf(
				"data mismatching array size, expected: %zu, result: %zu\n",
				data_size,
				group_data.size());
			return false;
		}
		for(size_t i = 0; i < data_size; ++i)
		{
			if(group_data.at(i) != data_begin[i])
			{
				serrf("data mismatching entry at: %zu\n", i);
				return false;
			}
		}

		size_t custom_size = custom_end - custom_begin;
		if(group_custom.size() != custom_size)
		{
			serrf(
				"custom mismatching array size, expected: %zu, result: %zu\n",
				custom_size,
				group_data.size());
			return false;
		}
		for(size_t i = 0; i < custom_size; ++i)
		{
			if(group_custom.at(i) != custom_begin[i])
			{
				serrf("custom mismatching entry at: %zu\n", i);
				return false;
			}
		}
		return true;
	}
};

// allow null terminaters in strings.
using namespace std::string_literals;

static const bs_data bs_expected_array[] = {
	{false, 0, 0, 0, 0, 0, 0, 0, 0, 0.00, 0.00, ""},
	{true, 1, 2, 3, 4, 5, 6, 7, 8, 9.9f, 10.10, "1234"},
	{false,
	 std::numeric_limits<int8_t>::min(),
	 std::numeric_limits<int16_t>::min(),
	 std::numeric_limits<int32_t>::min(),
	 std::numeric_limits<int64_t>::min(),
	 std::numeric_limits<uint8_t>::min(),
	 std::numeric_limits<uint16_t>::min(),
	 std::numeric_limits<uint32_t>::min(),
	 std::numeric_limits<uint64_t>::min(),
	 std::numeric_limits<float>::min(),
	 std::numeric_limits<double>::min(),
	 std::string(100, '\0')},
	{true,
	 std::numeric_limits<int8_t>::max(),
	 std::numeric_limits<int16_t>::max(),
	 std::numeric_limits<int32_t>::max(),
	 std::numeric_limits<int64_t>::max(),
	 std::numeric_limits<uint8_t>::max(),
	 std::numeric_limits<uint16_t>::max(),
	 std::numeric_limits<uint32_t>::max(),
	 std::numeric_limits<uint64_t>::max(),
	 std::numeric_limits<float>::max(),
	 std::numeric_limits<double>::max(),
	 std::string(100, '\xFF')}};

static const bs_custom bs_expected_custom[] = {
	bs_custom{"data", "data"},
	bs_custom{"", ""},
	bs_custom{std::string(100, 'a'), std::string(100, 'a')},
	bs_custom{"\r\n\x01", "\0\r\n\x01"s}};

static int g_bs_flag = 0;

static int test_BS_1(char* file_memory, size_t& file_size)
{
	// sadly the StringBuffer is much slower than the file stream because
	// this has a non absolute memory complexity (unfair comparison)
	BS_StringBuffer sb;
	{
		// copy the contents in.
		std::vector<bs_data> input(
			bs_expected_array, bs_expected_array + std::size(bs_expected_array));
		std::vector<bs_custom> input_custom(
			bs_expected_custom, bs_expected_custom + std::size(bs_expected_custom));
		TestSerialize test(input, input_custom);

		if(!BS_Write_Memory(test, sb, g_bs_flag, __func__))
		{
			return -1;
		}

		file_size = (sb.GetLength() > file_size) ? file_size : sb.GetLength();
		memcpy(file_memory, sb.GetString(), file_size);
		file_memory[file_size] = '\0';
	}
	{
		std::vector<bs_data> result;
		std::vector<bs_custom> result_custom;
		TestSerialize test(result, result_custom);

		if(!BS_Read_Memory(test, sb.GetString(), sb.GetLength(), g_bs_flag, __func__))
		{
			return -1;
		}

		if(!test.check(
			   bs_expected_array,
			   bs_expected_array + std::size(bs_expected_array),
			   bs_expected_custom,
			   bs_expected_custom + std::size(bs_expected_custom)))
		{
			return -1;
		}
	}
	return 0;
}

static int test_BS_2(char* file_memory, size_t& file_size)
{
	{
		Unique_RWops file = RWops_FromMemory(file_memory, file_size, __func__);
		if(!file) return -1;

		// copy the contents in.
		std::vector<bs_data> input(
			bs_expected_array, bs_expected_array + std::size(bs_expected_array));
		std::vector<bs_custom> input_custom(
			bs_expected_custom, bs_expected_custom + std::size(bs_expected_custom));
		TestSerialize test(input, input_custom);

		if(!BS_Write_Stream(test, file.get(), g_bs_flag))
		{
			return -1;
		}

		int get_file_size;
		if((get_file_size = file->tell()) == -1) return -1;
		file_size = get_file_size;
	}
	{
		Unique_RWops file = RWops_FromMemory_ReadOnly(file_memory, file_size, __func__);
		if(!file) return -1;

		std::vector<bs_data> result;
		std::vector<bs_custom> result_custom;
		TestSerialize test(result, result_custom);

		if(!BS_Read_Stream(test, file.get(), g_bs_flag))
		{
			return -1;
		}

		if(!test.check(
			   bs_expected_array,
			   bs_expected_array + std::size(bs_expected_array),
			   bs_expected_custom,
			   bs_expected_custom + std::size(bs_expected_custom)))
		{
			return -1;
		}
	}
	return 0;
}

#include "tools/BS_binary.h"
#include "tools/BS_json.h"

static int test_BS_3(char* file_memory, size_t& file_size)
{
	{
		// copy the contents in.
		std::vector<bs_data> input(
			bs_expected_array, bs_expected_array + std::size(bs_expected_array));
		std::vector<bs_custom> input_custom(
			bs_expected_custom, bs_expected_custom + std::size(bs_expected_custom));
		TestSerialize test(input, input_custom);

		BS_StringBuffer sb;

#ifndef DISABLE_BS_JSON
		if(g_bs_flag == BS_FLAG_JSON)
		{
			// instead of the pretty printer you can use the compact writer.
			BS_JsonWriter<decltype(sb), rj::Writer<decltype(sb)>> ar(sb);
			test.T_Serialize(ar);
			if(!ar.Finish(__func__))
			{
				return -1;
			}
		}
#endif
		if(g_bs_flag == BS_FLAG_BINARY)
		{
			BS_BinaryWriter ar(sb);
			test.T_Serialize(ar);
			if(!ar.Finish(__func__))
			{
				return -1;
			}
		}

		file_size = (sb.GetLength() > file_size) ? file_size : sb.GetLength();
		memcpy(file_memory, sb.GetString(), file_size);
		file_memory[file_size] = '\0';
	}
	{
		std::vector<bs_data> result;
		std::vector<bs_custom> result_custom;
		TestSerialize test(result, result_custom);

		BS_MemoryStream sb(file_memory, file_memory + file_size);

#ifndef DISABLE_BS_JSON
		if(g_bs_flag == BS_FLAG_JSON)
		{
			BS_JsonReader ar(sb);
			test.T_Serialize(ar);
			if(!ar.Finish(__func__))
			{
				return -1;
			}
		}
#endif
		if(g_bs_flag == BS_FLAG_BINARY)
		{
			BS_BinaryReader ar(sb);
			test.T_Serialize(ar);
			if(!ar.Finish(__func__))
			{
				return -1;
			}
		}

		if(!test.check(
			   bs_expected_array,
			   bs_expected_array + std::size(bs_expected_array),
			   bs_expected_custom,
			   bs_expected_custom + std::size(bs_expected_custom)))
		{
			return -1;
		}
	}
	return 0;
}

static int test_BS_4(char* file_memory, size_t& file_size)
{
	{
		Unique_RWops file = RWops_FromMemory(file_memory, file_size, __func__);
		if(!file) return -1;

		// copy the contents in.
		std::vector<bs_data> input(
			bs_expected_array, bs_expected_array + std::size(bs_expected_array));
		std::vector<bs_custom> input_custom(
			bs_expected_custom, bs_expected_custom + std::size(bs_expected_custom));
		TestSerialize test(input, input_custom);

		char buffer[1000];
		BS_WriteStream sb(file.get(), buffer, sizeof(buffer));

#ifndef DISABLE_BS_JSON
		if(g_bs_flag == BS_FLAG_JSON)
		{
			// instead of the pretty printer you can use the compact writer.
			BS_JsonWriter<decltype(sb), rj::Writer<decltype(sb)>> ar(sb);
			test.T_Serialize(ar);
			if(!ar.Finish(__func__))
			{
				return -1;
			}
		}
#endif
		if(g_bs_flag == BS_FLAG_BINARY)
		{
			BS_BinaryWriter ar(sb);
			test.T_Serialize(ar);
			if(!ar.Finish(__func__))
			{
				return -1;
			}
		}

		int get_file_size;
		if((get_file_size = file->tell()) == -1) return -1;
		file_size = get_file_size;
	}
	{
		Unique_RWops file = RWops_FromMemory_ReadOnly(file_memory, file_size, __func__);
		if(!file) return -1;

		std::vector<bs_data> result;
		std::vector<bs_custom> result_custom;
		TestSerialize test(result, result_custom);

		char buffer[1000];
		BS_ReadStream sb(file.get(), buffer, sizeof(buffer));

#ifndef DISABLE_BS_JSON
		if(g_bs_flag == BS_FLAG_JSON)
		{
			BS_JsonReader ar(sb);
			test.T_Serialize(ar);
			if(!ar.Finish(__func__))
			{
				return -1;
			}
		}
#endif
		if(g_bs_flag == BS_FLAG_BINARY)
		{
			BS_BinaryReader ar(sb);
			test.T_Serialize(ar);
			if(!ar.Finish(__func__))
			{
				return -1;
			}
		}

		if(!test.check(
			   bs_expected_array,
			   bs_expected_array + std::size(bs_expected_array),
			   bs_expected_custom,
			   bs_expected_custom + std::size(bs_expected_custom)))
		{
			return -1;
		}
	}
	return 0;
}

// possible todo's:
// the time benchmark is vague because it doesn't split the write/read time.
// This should probably be competely re-written because
// this was supposed to be an example, not so much a test suit.

// do a test pass for json and binary
static bool test_pass()
{
	TIMER_U t1;
	TIMER_U t2;

	char file_memory[9999];
	size_t file_size = sizeof(file_memory);

	struct job_type
	{
		const char* name;
		int (*pfn)(char*, size_t&);
	} test_jobs[] = {
#if 0
		{"test_object_1", test_object_1},
		{"test_array_1", test_array_1},
		{"test_array_of_objects_1", test_array_of_objects_1},
		{"test_array_of_objects_2", test_array_of_objects_2},
		
		{"test_kson_json_stream", test_kson_json_stream},
		{"test_kson_json_memory", test_kson_json_memory},
		{"test_kson_binary_stream", test_kson_binary_stream},
		{"test_kson_binary_memory", test_kson_binary_memory},
		{"test_read_kson", test_read_kson},
		{"test_read_1", test_read_1},
#endif

		{"test_BS_1", test_BS_1},
		{"test_BS_2", test_BS_2},
		{"test_BS_3", test_BS_3},
		{"test_BS_4", test_BS_4}
	};

	for(auto& job : test_jobs)
	{
		file_size = sizeof(file_memory) - 1;

		t1 = timer_now();
		int res = job.pfn(file_memory, file_size);
		t2 = timer_now();

		file_memory[file_size] = '\0';

		slogf("%s, ms: %f\n", job.name, timer_delta_ms(t1, t2));
		slogf("size: %zu\n", file_size);
		slogf("[[[\n%s\n]]]\n", file_memory);
		if(res < 0)
		{
			slogf("test failed: %s\n", job.name);
			ASSERT(serr_check_error());
			return false;
		}
	}
	return true;
}
int main(int, char**)
{
	if(SDL_Init(0) != 0)
	{
		serrf("Error initializing SDL:  %s\n", SDL_GetError());
		return -1;
	}
	slogf("hello world\n");

#ifndef DISABLE_BS_JSON
	g_bs_flag = BS_FLAG_JSON;
	slog("===START JSON===\n");
	if(!test_pass())
	{
		return -1;
	}
#endif

	slog("===START BINARY===\n");
	g_bs_flag = BS_FLAG_BINARY;
	if(!test_pass())
	{
		return -1;
	}

	SDL_Quit();

	slog("done\n");
	return 0;
}
