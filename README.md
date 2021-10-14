# dumbjson
Dumbjson is a Rapidjson wrapper that uses my very specific error handling which is referred to as "serr", and whatever else I personally use.\
This wasn't made to be used as a library that you include into your project. The "dumb" in dumbjson stands for this being a terrible, verbose, and flimsy API, but it serves my needs.\
Kson is something extra that was thrown into here, it is completely different than Dumbjson as Dumbjson represents rapidjson's DOM API, and Kson represents the SAX api, it has a template read/write archive trick and it's able to serialize to binary.\
But Kson cannot replace Dumbjson because kson is not Json, it is equal to binary in terms of flexibility. You should not use Kson as a configuration file that could be modified by hand.\
The Kson api requires a newer version of rapidjson, and ATM on my distro rapidjson is as old as 2015. I recommend you to just modify the header path of rapidjson and copy all the headers into your project tree.\
Shout out to rapidjson's "Archiver" in the examples directory, which inspired me to create Kson, and the weird "JsonConfig" wrapper for Dumbjson that I almost forgot about.