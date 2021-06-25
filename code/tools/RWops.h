#pragma once

class RWops
{
  public:
	const char* stream_info = "<unspecified>";
	// follows the same error specifications as stdio functions.
	// if read returns 0, check serr_check_error() or good()
	virtual size_t read(void* ptr, size_t size, size_t nmemb) = 0;
	virtual size_t write(const void* ptr, size_t size, size_t nmemb) = 0;
	// unlike SDL_RWops, this will not return tell(), 0 == success.
	virtual int seek(int offset, int whence) = 0;
	virtual int tell() = 0;

	//if false, it should mean that an error has been printed.
	virtual bool good() = 0;

	// the one annoying quirk is that the destructor won't return an error,
	// but you should still check serr.
	// if an error already occured,
	virtual ~RWops() = default;
};

typedef std::unique_ptr<RWops> Unique_RWops;

Unique_RWops RWops_OpenFS(const char* path, const char* mode = NULL);
Unique_RWops RWops_FromFP(FILE* fp, const char* name = NULL);
Unique_RWops RWops_FromFP_NoClose(FILE* fp, const char* name = NULL);

// if you are writing to the file the size will not be treated as a capacity,
// so if you read, EOF will not change (workaround: get size with tell(), and reopen)
Unique_RWops RWops_FromMemory(char* memory, size_t size, const char* name = NULL);
Unique_RWops RWops_FromMemory_ReadOnly(char* memory, size_t size, const char* name = NULL);
