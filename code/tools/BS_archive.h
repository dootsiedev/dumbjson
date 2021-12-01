#pragma once

// This is a JSON-like API for a restrictive style of JSON that acts like binary serialization.
// The most important feature is the symetric read / write system using virtual functions,
// but if you use BS_json and BS_binary you can use templates to avoid virtual functions.
// Used when the file is exclusively generated by the program, a human should not write it.
// This was inspired by a example called "archiver" in the rapidjson examples.
// Limitations:
// Variables must be ordered, no variable can be missing, no variable can be skipped over.
// Keys, objects, arrays do nothing in the binary serializer.
// An annoying limitation is that dynamic sized arrays requires you to specifiy
// the size of an array ahead of the array (like a pascal string).
// Dynamic arrays require you to check the IsReader flag to properly work with dynamic arrays.
// Note:
// Performance is not a goal for the Binary format but it is a minor benefit,
// the main benefit is in the size of the file,
// So you can have descriptive names of every variable without a worry about micro optimization.

// TODO:
//-Reading should give a path to the variable on an error like dumbjson.
//-all the ReadJson handlers will print a vague "expected [TYPE]" error, replace
// rj::BaseReaderHandler -using std::source_location (C++20) I could print the location of where the
// error occurred.

// example
#if 0
// creates: {"i":N, "d":N}
void serialize_data_type(BS_Serializer& ar, data_type& data)
{
	ar.StartObject();
	ar.Key("i");
	ar.Int32(data.i);
	ar.Key("d");
	ar.Double(data.d);
	ar.EndObject();
}

// the root serializer for an array of data_type.
// creates {"size":N, "array":[...]}
struct serialize_group_data : BS_Serializable
{
	std::vector<data_type>& group_data;

	explicit serialize_group_data(std::vector<data_type>& group_data_)
	: group_data(group_data_)
	{
	}

	void Serialize(BS_Archive& ar) override
	{
		ar.StartObject();

		// you must know the size of the array before reading it unlike normal json.
		// this right here is the worst part of BS_Archive.
		{
			const uint32_t min_size = 0;
			const uint32_t max_size = 2;

			// optional sanity checks for the writer.
			if(ar.IsWriter())
			{
				ASSERT(std::size(group_data) <= std::numeric_limits<uint32_t>::max());
				ASSERT(std::size(group_data) >= min_size);
				ASSERT(std::size(group_data) <= max_size);
			}

			// using uint64_t would be overkill.
			uint32_t size = std::size(group_data);

			// BS_min_max_state will include an error if the number is out of bounds.
			BS_min_max_state<uint32_t> size_cb_state{size, min_size, max_size};

			ar.Key("size");
			if(!ar.Uint32_CB(test_array_size, decltype(size_cb_state)::call, &size_cb_state))
			{
				// exit early
				// note that on the first callback or stream error,
				// all next operations will turn into NOPs and return false.
				return;
			}

			// the reader needs to allocate the entries to read into
			if(Archive::IsReader)
			{
				data.resize(size);
			}
		}

		ar.Key("array");
		ar.StartArray();
		for(data_type& entry : group_data)
		{
			if(!ar.Good())
			{
				// exit early
				return;
			}
			serialize_data_type(ar, entry);
		}
		ar.EndArray();
		ar.EndObject();
	}
};

// writer setup:
{
	// fill the input with data
	std::vector<data_type> input = {{1, 1.1}, {2, 2.2}};

	// this stores the result.
	BS_MemoryStream sb;

	// serialize_group_data inherits BS_Serializable
	serialize_group_data root(input);

	// call Serialize (replace BS_FLAG_JSON with BS_FLAG_BINARY for binary)
	if(!BS_Write_Memory(root, sb, BS_FLAG_JSON))
	{
		// error!
	}

	std::string file_data(sb.GetString(), sb.GetLength());
}

// reader setup:
{
	std::vector<data_type> output;

	// serialize_group_data inherits BS_Serializable
	serialize_group_data root(input);

	// call Serialize (replace BS_FLAG_JSON with BS_FLAG_BINARY for binary)
	if(!BS_Read_Memory(root, file_data.c_str(), file_data.size(), BS_FLAG_JSON))
	{
		// error!
	}

	// use output
}

or use BS_Serializable which you can use with:
-BS_Read_Memory
-BS_Write_Memory
-BS_Read_Stream
-BS_Write_Stream
for the possible faster compile / link times 
especially if you copy paste BS_JsonReader/Writer many times in different files.
but the problem is that your compiler might not optimize it as well.

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
	virtual bool Bool_CB(bool b, BS_bool_cb cb, void* ud) = 0;

	virtual bool Int8(int8_t& i) = 0;
	virtual bool Int8_CB(int8_t i, BS_int8_cb cb, void* ud) = 0;
	virtual bool Int16(int16_t& i) = 0;
	virtual bool Int16_CB(int16_t i, BS_int16_cb cb, void* ud) = 0;
	virtual bool Int32(int32_t& i) = 0;
	virtual bool Int32_CB(int32_t i, BS_int32_cb cb, void* ud) = 0;
	virtual bool Int64(int64_t& i) = 0;
	virtual bool Int64_CB(int64_t i, BS_int64_cb cb, void* ud) = 0;

	virtual bool Uint8(uint8_t& u) = 0;
	virtual bool Uint8_CB(uint8_t u, BS_uint8_cb cb, void* ud) = 0;
	virtual bool Uint16(uint16_t& u) = 0;
	virtual bool Uint16_CB(uint16_t u, BS_uint16_cb cb, void* ud) = 0;
	virtual bool Uint32(uint32_t& u) = 0;
	virtual bool Uint32_CB(uint32_t u, BS_uint32_cb cb, void* ud) = 0;
	virtual bool Uint64(uint64_t& u) = 0;
	virtual bool Uint64_CB(uint64_t u, BS_uint64_cb cb, void* ud) = 0;

	virtual bool Float(float& d) = 0;
	virtual bool Float_CB(float d, BS_float_cb cb, void* ud) = 0;
	virtual bool Double(double& d) = 0;
	virtual bool Double_CB(double d, BS_double_cb cb, void* ud) = 0;

	// the maxiumum size of a string is BS_MAX_STRING_SIZE (it's a uint16_t).
	virtual bool String(std::string& str) = 0;
	virtual bool String_CB(std::string_view str, BS_string_cb cb, void* ud) = 0;
	virtual bool StringZ(std::string& str, size_t max_size) = 0;
	virtual bool StringZ_CB(std::string_view str, size_t max_size, BS_string_cb cb, void* ud) = 0;

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