#pragma once

// This is a JSON API for a restrictive style of JSON that supports binary serialization.
// I want to call this KSON to distinguish itself as NOT json, because it's more like binary.
// The most important feature is the symetric read / write system using templates.
// Used when the file is exclusively generated by the program, a human should not write it.
// This was inspired by a example called "archiver" in the rapidjson examples.
// Limitations:
// Variables must be ordered, no variable can be missing, no variable can be skipped over.
// Keys, objects, arrays do nothing in the binary serializer.
// An annoying limitation is that dynamic sized arrays requires you to specifiy
// the size of an array ahead of the array (like a pascal string).
// Dynamic arrays require you to check the IsReader flag to properly work with dynamic arrays.
// Note:
// Performance is not a goal for the Binary format, because rapidjson is surprisingly fast,
// But the benefit is the lack of care needed with the length of key strings in JSON,
// So you can have descriptive names of every variable without a worry about micro optimization.

// TODO:
//-I am noticing that binary bloat is a problem (especially for JSON), anything I can do?
//-all the handlers will print a vague "expected [TYPE]" error, replace rj::BaseReaderHandler
//-should I trigger the read callback while writing for X_CB calls to check for errors?
//-using std::source_location (C++20) I could print the location of where the error occurred.
//-I could include a "key" label in errors that only apply to the next read
// maybe also keep a stack of array/objects?
//-Add a macro definition that enables future debugging features.
//-Maybe a Reserve function (only affects writing memory files)?

// example
#if 0
//shared simple serialization:
//creates: {"i":N, "d":N}
void serialize_data_type(BS_Serializer& ar, data_type& data)
{
	ar.StartObject();
	ar.Key("i");
	ar.Int32(data.i);
	ar.Key("d");
	ar.Double(data.d);
	ar.EndObject();
}

//shared dynamic arrays:
//creates {"size":N, "array":[...]}
void serialize_data_group(BS_Serializer& ar, std::vector<data_type>& group_data)
{
	ar.StartObject();
	ar.Key("size");
	uint16_t size = std::size(group_data);
	const uint16_t min_size = 0;
	const uint16_t max_size = 2;

	//you must know the size of the array before reading it unlike normal json.
	BS_min_max_state<uint16_t> size_cb_state{size, min_size, max_size};
	if(!ar.Uint16_CB(test_array_size, BS_min_max_cb<uint16_t>, &size_cb_state))
	{
		//exit early because the erronious value is being accessed.
		return;
	}

	//the reader needs to allocate the entries to read into
	if(Archive::IsReader)
	{
		data.resize(size);
	}

	ar.Key("array");
	ar.StartArray();
	for(data_type& entry : group_data)
	{
		serialize_data_type(ar, entry);
	}
	ar.EndArray();
	ar.EndObject();
}

//writer setup:
{
	//fill the input with data
	std::vector<data_type> input = {{1,1.1}, {2,2.2}};

	//this stores the result.
	rj::StringBuffer sb;

	//this is implements BS_Serializer
	BS_JsonWriter ar(sb);
	//you can switch with BS_BinaryWriter

	serialize_data_group(ar, input);

	if(!ar.Finish())
	{
		//Error! Failed to write json:
	}

	std::string file_data(sb.GetString(), sb.GetLength());
}

//reader setup:
{
	std::vector<data_type> output;

	BS_MemoryStream sb(file_data.c_str(), file_data.size());

	//this is implements BS_Serializer
	BS_JsonReader ar(sb);
	//you can switch with BS_BinaryReader

	serialize_data_group(&ar, input);

	if(!ar.Finish())
	{
		//Error! Failed to read json:
	}
	
	//use output
}

#endif

#define BS_MAX_STRING_SIZE std::numeric_limits<uint16_t>::max()

typedef bool (*BS_bool_cb)(bool, void*);

typedef bool (*BS_int8_cb)(int8_t, void*);
typedef bool (*BS_int16_cb)(int16_t, void*);
typedef bool (*BS_int32_cb)(int32_t, void*);
typedef bool (*BS_int64_cb)(int64_t, void*);

typedef bool (*BS_uint8_cb)(uint8_t, void*);
typedef bool (*BS_uint16_cb)(uint16_t, void*);
typedef bool (*BS_uint32_cb)(uint32_t, void*);
typedef bool (*BS_uint64_cb)(uint64_t, void*);

typedef bool (*BS_float_cb)(float, void*);
typedef bool (*BS_double_cb)(double, void*);

typedef bool (*BS_string_cb)(const char*, size_t, void*);

class BS_Archive
{
public:
	virtual ~BS_Archive() = default;

	virtual bool IsReader() const = 0;
	virtual bool IsWriter() const = 0;

	virtual bool Good() const = 0;

	// pass NULL for <unspecified>
	virtual bool Finish(const char* info) = 0;

	virtual bool Null() = 0; // json only
	virtual bool Bool(bool& b) = 0;
	virtual bool Bool_CB(bool& b, BS_bool_cb cb, void* ud) = 0;

	virtual bool Int8(int8_t& i) = 0;
	virtual bool Int8_CB(int8_t& i, BS_int8_cb cb, void* ud) = 0;
	virtual bool Int16(int16_t& i) = 0;
	virtual bool Int16_CB(int16_t& i, BS_int16_cb cb, void* ud) = 0;
	virtual bool Int32(int32_t& i) = 0;
	virtual bool Int32_CB(int32_t& i, BS_int32_cb cb, void* ud) = 0;
	virtual bool Int64(int64_t& i) = 0;
	virtual bool Int64_CB(int64_t& i, BS_int64_cb cb, void* ud) = 0;

	virtual bool Uint8(uint8_t& u) = 0;
	virtual bool Uint8_CB(uint8_t& u, BS_uint8_cb cb, void* ud) = 0;
	virtual bool Uint16(uint16_t& u) = 0;
	virtual bool Uint16_CB(uint16_t& u, BS_uint16_cb cb, void* ud) = 0;
	virtual bool Uint32(uint32_t& u) = 0;
	virtual bool Uint32_CB(uint32_t& u, BS_uint32_cb cb, void* ud) = 0;
	virtual bool Uint64(uint64_t& u) = 0;
	virtual bool Uint64_CB(uint64_t& u, BS_uint64_cb cb, void* ud) = 0;

	virtual bool Float(float& d) = 0;
	virtual bool Float_CB(float& d, BS_float_cb cb, void* ud) = 0;
	virtual bool Double(double& d) = 0;
	virtual bool Double_CB(double& d, BS_double_cb cb, void* ud) = 0;

	virtual bool String(std::string& str) = 0;
	virtual bool String_CB(std::string& str, BS_string_cb cb, void* ud) = 0;
	virtual bool StringZ(std::string& str, size_t max_size) = 0;
	virtual bool StringZ_CB(std::string& str, size_t max_size, BS_string_cb cb, void* ud) = 0;

	// can't use string because json cannot store binary (0-255)
	virtual bool Data(std::string& str) = 0;
	virtual bool Data_CB(std::string& str, BS_string_cb cb, void* ud) = 0;
	virtual bool DataZ(std::string& str, size_t max_size) = 0;
	virtual bool DataZ_CB(std::string& str, size_t max_size, BS_string_cb cb, void* ud) = 0;

	// json only
	virtual bool Key(std::string_view str) = 0;
	virtual bool StartObject() = 0;
	virtual bool EndObject() = 0;
	virtual bool StartArray() = 0;
	virtual bool EndArray() = 0;
};

template<class T>
struct BS_min_max_state
{
	T& value;
	T minimum;
	T maximum;

	static bool call(T t, void* ud)
	{
		BS_min_max_state* state = reinterpret_cast<BS_min_max_state*>(ud);

		if(t < state->minimum)
		{
			// printf + generics don't mix up well, better use a stringstream.
			std::ostringstream oss;
			oss << "number too small, min: " << state->minimum << ", result: " << t;
			serrf("%s\n", oss.str().c_str());
			return false;
		}
		if(t > state->maximum)
		{
			std::ostringstream oss;
			oss << "number too large, max: " << state->maximum << ", result: " << t;
			serrf("%s\n", oss.str().c_str());
			return false;
		}

		state->value = t;
		return true;
	}
};

// garbage used in both json and binary.
// it's ugly to be here, but oh well.
template<class T>
bool internal_read_simple_cb(T t_, void* ud)
{
	T* t = reinterpret_cast<T*>(ud);
	*t = t_;
	return true;
}

inline bool internal_read_string_cb(const char* str_, size_t length_, void* ud)
{
	std::string* str = reinterpret_cast<std::string*>(ud);
	*str = std::string(str_, length_);
	return true;
}