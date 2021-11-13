# dumbjson
The tests have turned into a mess, this repo is just for reference.\
This wasn't made to be used as a library that you include into your project. The "dumb" in dumbjson stands for this being a terrible, verbose, and flimsy API, but it serves my needs.\
Credit to rapidjson's "Archiver" in the examples directory, which inspired this, and it's better than this library in simplicity.\
Dumbjson is a Rapidjson wrapper that uses my very specific error handling which is referred to as "serr", and whatever else I personally use.\
Kson is something extra that was thrown into here, it is completely different than Dumbjson as Dumbjson represents rapidjson's DOM API, and Kson represents the SAX api, it has a template read/write archive trick and it's able to serialize to binary.\
But Kson cannot replace Dumbjson because kson is not Json, it is equal to binary in terms of flexibility. You should not use Kson as a configuration file that could be modified by hand.\
The Kson api requires a newer version of rapidjson.\
BS_Archive (BS stands for "Binary Serializer") is a better version of Kson which uses virtual function interface (but it can be more annoying than kson in some ways). The virtual functions cause an overhead over kson, but it's not as slow as I expected (and the binary serializer is sped up if I disable json due to de-virtualization). This is better because the bloat from kson is not worth it.\
If you value performance and security and you don't care about plain text representation, I would recommend google protobuf.