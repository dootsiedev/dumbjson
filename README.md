# BS_Archive
This is a JSON-like API for a restrictive style of JSON that acts like binary serialization.\
The most important feature is the symetric read / write system using virtual functions,\
but if you use BS_json and BS_binary you can use templates to avoid virtual functions.\
Used when the file is exclusively generated by the program, a human should not write it.\
This was inspired by a example called "archiver" in the rapidjson examples.\
Limitations:\
-Variables must be ordered, no variable can be missing, no variable can be skipped over.\
-Keys, objects, arrays do nothing in the binary serializer.\
-An annoying limitation is that dynamic sized arrays requires you to specifiy\
 the size of an array ahead of the array (like a pascal string).\
Dynamic arrays require you to check the IsReader flag to properly work with dynamic arrays.\
Note:\
Performance is not the goal for the Binary format (it is faster, but not by 10x),\
the main goal is the size of the file and to prevent people from tinkering with it.\
\
Example:
```C++
// creates: {"i":N, "d":N}
void serialize_data_type(BS_Archive& ar, data_type& data)
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
	serialize_group_data root(output);

	// call Serialize (replace BS_FLAG_JSON with BS_FLAG_BINARY for binary)
	if(!BS_Read_Memory(root, file_data.c_str(), file_data.size(), BS_FLAG_JSON))
	{
		// error!
	}

	// use output
}
```
This wasn't made to be used as a library that you include into your project.\
Credit to rapidjson's "Archiver" in the examples directory, which inspired this, and it's better than this library in simplicity.\
This code depends on my very specific and probably poorly designed error handling system called "serr", which is just errno replaced with a single std::string, which makes errors look like a lazy dump of information, but I like it because it's simple.\
\
This isn't a high performance serialization library but it's an OK performance serialization library.\
Dumbjson was the first project (why this repo is called dumbjson, but BS_Archive is still pretty dumb) and it is a Rapidjson DOM api wrapper and it will print errors if you use it correctly, but I wouldn't use it because I don't like how difficult it is to use (it's even harder than just using rapidjson normally). I might revive this because Json is a key-value document, but BS_Archive is just a linear stream of data, no hashing.\
Kson is old (and buggy) version of BS_Archive. The benefit over BS_Archive is that it offers template functions so you can use C++11 lambdas for much easier custom callbacks, and it's faster because no virtual functions and templates (might be worth revival considering the fact you can implement both BS_Archive and kson at the same time, but the performance benefit is miniscule).\
The tests has turned into a mess, dumb_json and kson is mostly dead code, but I am keeping the code for reference.