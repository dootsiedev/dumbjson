#pragma once

#define RAPIDJSON_ASSERT ASSERT
#define RAPIDJSON_HAS_STDSTRING 1

//#define MYDEBUG

#include <rapidjson/document.h>
#include <rapidjson/fwd.h>
namespace rj = rapidjson;

// This API is explosive, which is a feature. If you use the API improperly, it will trigger an
// ASSERT. except it won't explode in certain cases, which kinda sucks, like if you forget to wraper
// a Reader.

class JsonMemberReader;
class JsonIndexReader;
class RWops;

extern const char* g_kTypeNames[];
extern const char* g_unspecified_filename_default;

const char* rj_string(const rj::Value& value);

union json_unwind_entry
{
	// if name is NULL, the next entry in the table is the index.
	const char* name;
	size_t index;
};

class JsonState : nocopy
{
  public:
	// note, it is expected to manually access this, same for rjvalue,
	// because I am not going to wrap every single function.
	// optimization: I can supply a memory buffer to avoid malloc
	// the memory buffer can also make clear() avoid reallocating.
	rj::Document rjdoc;

	// this is purely for diagnostics, only used to print the path.
	// not used while writing.
	// optimization: maybe make this use a SmallVector (arena allocator)?
	std::vector<json_unwind_entry> json_unwind_table;

	// the lifetime of this string exists outside the object (DANGER!!!).
	const char* file_info = NULL;

	// filename lifetime must persist, leaving info NULL will use RWops filename
	// check_object can be used to disable the root requiring "{}"
	bool open_file(RWops* file, const char* info = NULL, rj::Type expected = rj::kObjectType);
	bool open_string(const char* buffer, size_t buffer_size, const char* info = NULL,
					 rj::Type expected = rj::kObjectType);

	// open_string_insitu wouldn't be a bad idea, like for compressed files.
	// but I have this feeling that this won't give a 2x performance boost for SSD's...
	// bool open_string_insitu

	// leaving info NULL will use RWops filename.
	// note this will not modify file_info, use rename().
	bool write_file(RWops* file, const char* info = NULL);

	// you can use .GetString(); and .GetSize() on the buffer
	// you must also #include <rapidjson/StringBuffer.h>
	bool write_string(rj::StringBuffer& buffer, const char* info = NULL);

	// create() will clear and create an empty root object.
	// there is no name, because it's impossible (it should be) to create errors.
	void create(rj::Type type = rj::kObjectType);

	// filename lifetime must persist, leaving info NULL sets it to "<unspecified>"
	void rename(const char* info = NULL);

	// this will not create a root object, which means you cannot pass rjdoc into JsonObject.
	// the filename will stay the same (the filename only matters for reading anyways)
	void clear();

	// checks if the member is missing and the correct type, returns object.EndMember() if error.
	// used for arrays and objects because you can't deduce the types.
	// NOTE: I feel really dirty about "const rj::GenericXXX&", since it should be const right?
	template<class K, bool Const, class ValueT>
	rj::GenericMemberIterator<Const, typename ValueT::EncodingType, typename ValueT::AllocatorType>
		CheckMember(const rj::GenericObject<Const, ValueT>& object, K&& key, rj::Type type)
	{
		auto mitr = object.FindMember(key);
		if(mitr == object.MemberEnd())
		{
			internal_print_missing_member_error(__func__, rj::Value::StringRefType(key).s);
		}
		else
		{
			if(mitr->value.GetType() != type)
			{
				internal_print_member_convert_error(__func__, mitr, g_kTypeNames[type]);
				return object.MemberEnd();
			}
		}
		return mitr;
	}

	template<bool Const, class ValueT>
	ValueT* CheckIndex(const rj::GenericArray<Const, ValueT>& array, size_t index, rj::Type type)
	{
		if(index >= array.Size())
		{
			internal_print_array_size_error(__func__, index, array.Size());
		}
		else
		{
			auto found = array.Begin() + index;
			if(found->GetType() != type)
			{
				internal_print_index_convert_error(__func__, index, found, g_kTypeNames[type]);
			}
			else
			{
				return found;
			}
		}
		return NULL;
	}

	// will search for the member before setting it.
	template<class T, class K>
	void SetMember(const rj::Value::Object& object, K&& key, T&& value)
	{
		rj::Value::MemberIterator mitr = object.FindMember(key);
		if(mitr != object.MemberEnd())
		{
			mitr->value = value;
		}
		else
		{
			object.AddMember(key, value, rjdoc.GetAllocator());
		}
	}

	template<class T, class K>
	void AddMember(const rj::Value::Object& object, K&& key, T&& value)
	{
		object.AddMember(key, value, rjdoc.GetAllocator());
	}

	// rapidjson generics are terrible.
	template<class T, class K, bool Const, class ValueT>
	bool GetMember(const rj::GenericObject<Const, ValueT>& object, K&& key, T& value)
	{
		auto mitr = object.FindMember(key);
		if(mitr == object.MemberEnd())
		{
			internal_print_missing_member_error(__func__, rj::Value::StringRefType(key).s);
		}
		else
		{
			using UN_REF = typename std::remove_cv<T>::type;
			if(!rj::internal::TypeHelper<ValueT, UN_REF>::Is(mitr->value) &&
			   !(std::is_same<double, UN_REF>::value &&
				 (mitr->value.IsInt() || mitr->value.IsUint())))
			{
				internal_print_member_convert_error(__func__, mitr,
													rj_string(rj::Value().Set(value)));
			}
			else
			{
				value = rj::internal::TypeHelper<ValueT, UN_REF>::Get(mitr->value);
				return true;
			}
		}
		return false;
	}

	// SetIndex doesn't exist because it would just wrap rjvalue.GetArray()[]
	// you probably want rjvalue.GetArray().Reserve() + .PushBack().

	template<class T, bool Const, class ValueT>
	bool GetIndex(const rj::GenericArray<Const, ValueT>& array, size_t index, T& value)
	{
		if(index >= array.Size())
		{
			internal_print_array_size_error(__func__, index, array.Size());
		}
		else
		{
			auto found = array.Begin() + index;
			using UN_REF = typename std::remove_cv<T>::type;
			if(!rj::internal::TypeHelper<ValueT, UN_REF>::Is(*found) &&
			   !(std::is_same<double, UN_REF>::value && (found->IsInt() || found->IsUint())))
			{
				internal_print_index_convert_error(__func__, index, found,
												   rj_string(rj::Value().Set(value)));
			}
			else
			{
				value = rj::internal::TypeHelper<ValueT, UN_REF>::Get(*found);
				return true;
			}
		}
		return false;
	}

	// this will push back.
	// I could make a convenience wrapper that treats a member as an array,
	// but it wouldn't work for 2d arrays.
	template<class Iter, bool Const, class ValueT>
	rj::GenericArray<Const, ValueT> SetArrayRange(const rj::GenericArray<Const, ValueT>& array,
												  Iter start, Iter end)
	{
		array.Reserve(end - start, rjdoc.GetAllocator());
		while(start != end)
		{
			array.PushBack(*start++, rjdoc.GetAllocator());
		}
		return array;
	}

	// copy the whole array, must have the exact same size, must use random index iterators.
	//WARNING: if a conversion error occurred, the output will still be modified!
	//in the case you use defaults, you can use another array for reading, and set after success.
	template<class Iter, bool Const, class ValueT>
	bool GetArrayRange(const rj::GenericArray<Const, ValueT>& array, Iter start, Iter end)
	{
		size_t range_size = (end - start);
		if(array.Size() != range_size)
		{
			internal_print_array_size_error(__func__, array.Size(), range_size);
			return false;
		}
		// this depends on this size check.
		auto array_cur = array.Begin();
		for(Iter cur = start; cur != end; ++cur, ++array_cur)
		{
			using UN_REF = typename std::iterator_traits<Iter>::value_type;
			if(!rj::internal::TypeHelper<ValueT, UN_REF>::Is(*array_cur) &&
			   !(std::is_same<double, UN_REF>::value &&
				 (array_cur->IsInt() || array_cur->IsUint())))
			{
				internal_print_index_convert_error(__func__, (cur - start), array_cur,
												   rj_string(rj::Value().Set(*cur)));
				return false;
			}
			*cur = rj::internal::TypeHelper<ValueT, UN_REF>::Get(*array_cur);
		}
		return true;
	}

	// purely for user errors, ex: too many or too few elements in an array.
	void PrintError(const char* message, ...) __attribute__((format(printf, 1, 2)));
	void PrintMemberError(const char* key, const char* message, ...) __attribute__((format(printf, 2, 3)));
	void PrintIndexError(size_t index, const char* message, ...) __attribute__((format(printf, 2, 3)));

	// prints the path of the current Json(Member/Index)(Writer/Reader) stack.
	std::string dump_path();

	// rj::Value::ConstMemberIterator internal_find_member(const char* function, const
	// rj::Value::ConstObject &object, rj::Value::StringRefType key); rj::Value::ConstValueIterator
	// internal_find_index(const char* function, const rj::Value::Array &array, int index);

	void internal_print_missing_member_error(const char* function, const char* key);
	void internal_print_missing_index_error(const char* function, size_t index, size_t size);
	void internal_print_member_convert_error(const char* function,
											 const rj::Value::ConstMemberIterator& mitr,
											 const char* expected);
	void internal_print_index_convert_error(const char* function, size_t index,
											const rj::Value::ConstValueIterator& value,
											const char* expected);
	void internal_print_array_size_error(const char* function, size_t array_size,
										 size_t expected_size);
};

// This is to help with printing error messages while reading,
// For every JsonMemberReader, you are putting a name on the error message.
// You must use this with JsonState::CheckMember for missing + type errors.
// The destructor pops the name off the stack (for a proper error message),
// you can explicitly call finish() to pop the name off the stack.
class JsonMemberReader : nocopy
{
  public:
	rj::Value& rjvalue;
	// this is a pointer purely for the use as a finish flag.
	std::vector<json_unwind_entry>* json_unwind_table;
#ifdef MYDEBUG
	size_t stack_level;
#endif // MYDEBUG

	JsonMemberReader(rj::Value::MemberIterator& mitr, JsonState& json_state)
		: rjvalue(mitr->value), json_unwind_table(&json_state.json_unwind_table)
#ifdef MYDEBUG
		  ,
		  stack_level(json_state.json_unwind_table.size())
#endif // MYDEBUG
	{
		json_unwind_entry entry;
		entry.name = mitr->name.GetString();
		json_unwind_table->push_back(entry);
	}

	void finish()
	{
		if(json_unwind_table != NULL)
		{
			json_unwind_table->pop_back();
#ifdef MYDEBUG
			ASSERT(stack_level == json_unwind_table->size());
#endif // MYDEBUG
			json_unwind_table = NULL;
		}
	}

	~JsonMemberReader()
	{
		finish();
	}
};

// this is to help with printing error messages while reading,
// this is only for when you have an object within an array.
// read JsonMemberReader for more info.
// note you need to use JsonState::CheckIndex(...rj::kObjectType/kArrayType).
class JsonIndexReader : nocopy
{
  public:
	rj::Value& rjvalue;
	// this is a pointer purely for the use as a finish flag.
	std::vector<json_unwind_entry>* json_unwind_table;
#ifdef MYDEBUG
	size_t stack_level;
#endif // MYDEBUG

	JsonIndexReader(rj::Value& array, size_t index, JsonState& json_state)
		: rjvalue(array), json_unwind_table(&json_state.json_unwind_table)
#ifdef MYDEBUG
		  ,
		  stack_level(json_state.json_unwind_table.size())
#endif // MYDEBUG
	{
		json_unwind_entry entry;
		entry.name = NULL;
		json_unwind_table->push_back(entry);
		entry.index = index;
		json_unwind_table->push_back(entry);
	}

	void finish()
	{
		if(json_unwind_table != NULL)
		{
			json_unwind_table->pop_back();
			json_unwind_table->pop_back();
#ifdef MYDEBUG
			ASSERT(stack_level == json_unwind_table->size());
#endif // MYDEBUG
			json_unwind_table = NULL;
		}
	}

	~JsonIndexReader()
	{
		finish();
	}
};
