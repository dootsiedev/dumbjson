#include "global.h"
#include "tools/RWops.h"
#include "tools/dumbjson.h"

#include <limits>

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
		Unique_RWops file = RWops_FromMemory(file_memory, file_size, __FUNCTION__);
		if(!file) return -1;
		if(!json_state.write_file(file.get())) return -1;
		int get_file_size;
		if((get_file_size = file->tell()) == -1) return -1;
		file_size = get_file_size;
	}
	{
		JsonState json_state;

		// Unique_RWops file = RWops_OpenFS("path.json", "rb");
		Unique_RWops file = RWops_FromMemory_ReadOnly(file_memory, file_size, __FUNCTION__);
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
				const auto& rjobject = test_object.rjvalue.GetObject();

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
					   test_array.rjvalue.GetArray(), result, result + std::size(result)))
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
					   test_array.rjvalue.GetArray(), result, result + std::size(result)))
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
					   test_array.rjvalue.GetArray(), result, result + std::size(result)))
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

				const auto& rjarray = test_array.rjvalue.GetArray();

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
					   test_array.rjvalue.GetArray(), result, result + std::size(result)))
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

	data_type(int i_, double d_, std::string&& s_)
	: i(i_)
	, d(d_)
	, s(std::move(s_))
	{
	}

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
			if(str_mitr == object.MemberEnd()) { success = false; }
			else
			{
				s = std::string(str_mitr->value.GetString(), str_mitr->value.GetStringLength());
			}
		}
		return success;
	}

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
		Unique_RWops file = RWops_FromMemory(file_memory, file_size, __FUNCTION__);
		if(!file) return -1;
		if(!json_state.write_file(file.get())) return -1;
		int get_file_size;
		if((get_file_size = file->tell()) == -1) return -1;
		file_size = get_file_size;
	}
	{
		JsonState json_state;

		// Unique_RWops file = RWops_OpenFS("path.json", "rb");
		Unique_RWops file = RWops_FromMemory_ReadOnly(file_memory, file_size, __FUNCTION__);
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
				const auto& rjarray = test_array.rjvalue.GetArray();

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
					JsonIndexReader test_object(*vitr, i, json_state);

					data_type result(0, 0, "");

					if(!result.read(json_state, test_object.rjvalue.GetObject())) { return -1; }

					if(expected_array[i] != result)
					{
						json_state.PrintError("incorrect value");
						return -1;
					}
				}
			}
		}
	}
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
	"null":null}
)";
		size_t temp_size = strlen(temp_file);
		ASSERT(file_size > temp_size);

		Unique_RWops file = RWops_FromMemory_ReadOnly(temp_file, temp_size, __FUNCTION__);
		if(!file) return -1;

		// this will also check if root is an object.
		if(!json_state.open_file(file.get())) return -1;

		file.reset();

		{
			auto mitr = json_state.CheckMember(json_state.rjdoc.GetObject(), "null", rj::kNullType);
			if(mitr == json_state.rjdoc.MemberEnd()) return -1;
		}
	}

	{
		// I just write the file out as a way of testing.
		// actually testing would be better, but I feel like previous tests should work.
		Unique_RWops file = RWops_FromMemory(file_memory, file_size, __FUNCTION__);
		if(!file) return -1;
		if(!json_state.write_file(file.get())) return -1;

		int get_file_size;
		if((get_file_size = file->tell()) == -1) return -1;
		file_size = get_file_size;
	}

	return 0;
}

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
			Unique_RWops file = RWops_FromMemory_ReadOnly(temp_file, temp_size, __FUNCTION__);
			if(!file) return -1;

			// this will also check if root is an object.
			if(!json_state.open_file(file.get())) return -1;

			file.reset();
		}

		{
			// I just write the file out, for sanity.
			Unique_RWops file = RWops_FromMemory(file_memory, file_size, __FUNCTION__);
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
					const auto& rjobject = test_object.rjvalue.GetObject();

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
							int result[1];

							if(!json_state.GetArrayRange(
								   test_array.rjvalue.GetArray(),
								   result,
								   result + std::size(result)))
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
							double result[1];

							if(!json_state.GetArrayRange(
								   test_array.rjvalue.GetArray(),
								   result,
								   result + std::size(result)))
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

					const auto& rjarray = test_array.rjvalue.GetArray();

					size_t test_array_size = rjarray.Size();

					if(test_array_size != 1)
					{
						json_state.PrintError("array has incorrect size != 1");
						return -1;
					}

					// for(size_t i = 0; i < test_array_size; ++i)

					auto* vitr = json_state.CheckIndex(rjarray, 0, rj::kObjectType);
					if(vitr == NULL) return -1;
					JsonIndexReader test_object(*vitr, 0, json_state);

					const char* result;

					if(!json_state.GetMember(
						   test_object.rjvalue.GetObject(), key_array_string, result))
					{
						return -1;
					}

					if(strcmp(result, "a") != 0)
					{
						json_state.PrintError("incorrect value != \"a\"");
						return -1;
					}
				}
			}

			{
				// now start checking the errors.
				json_state.PrintError("PrintError!");
				ASSERT(!serr_get_error().empty());
				json_state.PrintError("PrintError, with %s!", "vargs");
				ASSERT(!serr_get_error().empty());
				json_state.PrintMemberError("nonexistant", "PrintMemberError Error!");
				ASSERT(!serr_get_error().empty());
				json_state.PrintMemberError(
					"nonexistant", "PrintMemberError Error with %s!", "vargs");
				ASSERT(!serr_get_error().empty());
				json_state.PrintIndexError(123, "PrintIndexError Error!");
				ASSERT(!serr_get_error().empty());
				json_state.PrintIndexError(123, "PrintIndexError Error with %s!", "vargs");
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
					const auto& rjobject = test_object.rjvalue.GetObject();

					// things that don't exist
					{
						slogf("\"%s\" does not exist\n", key_object);
						ASSERT(
							json_state.CheckMember(rjobject, key_object, rj::kObjectType) ==
							rjobject.MemberEnd());
						ASSERT(!serr_get_error().empty());
					}
					{
						slogf("\"%s\" does not exist\n", key_array);
						ASSERT(
							json_state.CheckMember(rjobject, key_array, rj::kArrayType) ==
							rjobject.MemberEnd());
						ASSERT(!serr_get_error().empty());
					}
					{
						slogf("\"%s\" does not exist\n", key_array_string);
						const char* result_original = "2";
						const char* result = result_original;
						ASSERT(!json_state.GetMember(rjobject, key_array_string, result));
						ASSERT(!serr_get_error().empty());
						ASSERT(result == result_original);
					}

					// int
					{
						slogf("\"%s\" not an object\n", key_object_int);
						ASSERT(
							json_state.CheckMember(rjobject, key_object_int, rj::kObjectType) ==
							rjobject.MemberEnd());
						ASSERT(!serr_get_error().empty());
					}
					{
						slogf("\"%s\" not an bool\n", key_object_int);
						bool result = true;
						ASSERT(!json_state.GetMember(rjobject, key_object_int, result));
						ASSERT(!serr_get_error().empty());
						ASSERT(result);
					}
					// array
					{
						slogf("\"%s\" not an object\n", key_object_array);
						ASSERT(
							json_state.CheckMember(rjobject, key_object_array, rj::kObjectType) ==
							rjobject.MemberEnd());
						ASSERT(!serr_get_error().empty());
					}
					{
						slogf("\"%s\" not an bool\n", key_object_array);
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
						const auto& rjarray = test_array.rjvalue.GetArray();
						{
							slogf("array \"%s\" not enough elements\n", key_object_array);
							int result[2] = {2, 3};
							ASSERT(!json_state.GetArrayRange(
								rjarray, result, result + std::size(result)));
							ASSERT(!serr_get_error().empty());
							ASSERT(result[0] == 2);
							ASSERT(result[1] == 3);
						}
						{
							slogf("array \"%s\" too many elements\n", key_object_array);
							const char* wtf[1];
							ASSERT(!json_state.GetArrayRange(rjarray, wtf, wtf));
							ASSERT(!serr_get_error().empty());
						}
						{
							slogf("array \"%s\" contains not object\n", key_object_array);
							ASSERT(json_state.CheckIndex(rjarray, 0, rj::kObjectType) == NULL);
							ASSERT(!serr_get_error().empty());
						}
						{
							slogf("array \"%s\" contains not string\n", key_object_array);
							const char* original_result = "2";
							const char* result[1] = {original_result};
							ASSERT(!json_state.GetArrayRange(
								rjarray, result, result + std::size(result)));
							ASSERT(!serr_get_error().empty());
							ASSERT(result[0] == original_result);
						}
						{
							slogf("array \"%s\" contains not array\n", key_object_array);
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

					const auto& rjarray = test_array.rjvalue.GetArray();

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
						slogf("array \"%s\" contains not bool\n", key_array);
						bool result[1] = {false};
						ASSERT(!json_state.GetArrayRange(rjarray, result, result + 1));
						ASSERT(!serr_get_error().empty());
						ASSERT(!result[0]);
					}
					{
						slogf("array \"%s\" contains not array\n", key_array);
						ASSERT(json_state.CheckIndex(rjarray, 0, rj::kArrayType) == NULL);
						ASSERT(!serr_get_error().empty());
					}
					{
						slogf("array \"%s\" contains not null\n", key_array);
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
							JsonIndexReader test_object2(*vitr, 0, json_state);
							const auto& rjobject2 = test_object2.rjvalue.GetObject();

							// things that don't exist
							{
								slogf("\"%s\" does not exist\n", key_object);
								ASSERT(
									json_state.CheckMember(
										rjobject2, key_object, rj::kObjectType) ==
									rjobject2.MemberEnd());
								ASSERT(!serr_get_error().empty());
							}
							{
								slogf("\"%s\" does not exist\n", key_array);
								ASSERT(
									json_state.CheckMember(rjobject2, key_array, rj::kArrayType) ==
									rjobject2.MemberEnd());
								ASSERT(!serr_get_error().empty());
							}
							// string
							{
								slogf("\"%s\" not bool\n", key_array_string);
								bool result = false;
								ASSERT(!json_state.GetMember(rjobject2, key_array_string, result));
								ASSERT(!serr_get_error().empty());
								ASSERT(!result);
							}
							{
								slogf("\"%s\" not array\n", key_array_string);
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

// possible todo's:
// the time benchmark is vague because it doesn't split the write/read time.
// PrintError could use some vargs (I originally didn't have vargs while writing this).
// This should probably be competely re-written because
// this was supposed to be an example, not so much a test suit.

int main()
{
	slogf("hello world\n");

	TIMER_U t1;
	TIMER_U t2;

	char file_memory[9999];
	size_t file_size = sizeof(file_memory);

	struct job_type
	{
		const char* name;
		int (*pfn)(char*, size_t&);
	} test_jobs[] = {
		{"test_object_1", test_object_1},
		{"test_array_1", test_array_1},
		{"test_array_of_objects_1", test_array_of_objects_1},
		{"test_read_1", test_read_1},
		{"test_error_1", test_error_1}};

	for(auto& job : test_jobs)
	{
		file_size = sizeof(file_memory);

		t1 = timer_now();
		if(job.pfn(file_memory, file_size) < 0)
		{
			ASSERT(serr_check_error());
			return -1;
		}
		t2 = timer_now();

		file_memory[file_size] = '\0';

		slogf("%s, ms: %f\n", job.name, timer_delta_ms(t1, t2));
		slogf("[[[\n%s\n]]]\n", file_memory);
	}

	slogf("done\n");
	return 0;
}
