#include <cstring>
#include <iostream>

static inline bool streq(const char *a, const char *b)
{
	return std::strcmp(a, b) == 0;
}

int main(int argc, const char **av)
{
	av++;
	argc--;
	size_t ac = argc;
	size_t i = 0;
	if (i >= ac) {
		std::cerr << "Expected at least one argument." << std::endl;
		std::cerr << "-h or --help for usage." << std::endl;
		return 1;
	}
	if (streq(av[i], "-h") || streq(av[i], "--help")) {
		std::cout << "[NOTEC FILESYSTEM UTILITARY]" << std::endl;
		std::cout << "-What am I?" << std::endl;
		std::cout << "-Casio fx-9860G based systems handle up to 8 characters filenames, with up to 3" << std::endl;
		std::cout << " extra for extension. Filename charset is limited to 40 different characters." << std::endl;
		std::cout << " They cannot handle nested directories." << std::endl;
		std::cout << " NOTEC uses a hashed filepath technique to allow for ASCII-based filenames of" << std::endl;
		std::cout << " arbitrary length and directory nesting, at no filesystem extra lookup cost." << std::endl;
		std::cout << "=> Use --faq for more info." << std::endl;
		std::cout << "Lucky you, I can encode and decode these file structures." << std::endl;
		std::cout << std::endl;
		std::cout << "==> Mutation usage" << std::endl;
		std::cout << "fs [co|dec] opt[options...] src dst" << std::endl;
		std::cout << std::endl;
		std::cout << "-Encode a file structure:" << std::endl;
		std::cout << " fs co opt[-r, -f, -m, -w] src dst" << std::endl;
		std::cout << "\t-r: include src directory name at root" << std::endl;
		std::cout << "\t    Note: By default all filenames are relative to src." << std::endl;
		std::cout << "\t-f: completely overwrite any existing output directory" << std::endl;
		std::cout << "\t    Note: By default will print error & exit." << std::endl;
		std::cout << "\t-m: merge contents if output directory exists" << std::endl;
		std::cout << "\t    Note: By default will print error & exit." << std::endl;
		std::cout << "\t    Note: -f and -m are mutually exclusive." << std::endl;
		std::cout << "\t-w: ignore self encoding error" << std::endl;
		std::cout << "\tsrc: the path to the directory whose contents will be encoded" << std::endl;
		std::cout << "\tdst: the path to the output directory" << std::endl;
		std::cout << "\t     Note: behaves similarly to cp: creates folder if dst does not exist," << std::endl;
		std::cout << "\t     creates folder which name is src directory name is dst does exist." << std::endl;
		std::cout << std::endl;
		std::cout << "-Decode a file structure:" << std::endl;
		std::cout << " fs dec opt[-f, -w] src dst" << std::endl;
		std::cout << "\t-f: completely overwrite any existing output directory" << std::endl;
		std::cout << "\t    Note: By default will print error & exit." << std::endl;
		std::cout << "\t-w: ignore self decoding error" << std::endl;
		std::cout << "\tsrc: the path to the directory which contains encoded file set (with index)" << std::endl;
		std::cout << "\tdst: the path to the output directory" << std::endl;
		std::cout << "\t    Note: behaves similarly to cp: creates folder if dst does not exist," << std::endl;
		std::cout << "\t    creates folder which name is src directory name is dst does exist." << std::endl;
		std::cout << std::endl;
		std::cout << "==> Passive usage" << std::endl;
		std::cout << "-Display encoded file structure" << std::endl;
		std::cout << " fs tree src" << std::endl;
		std::cout << "\tsrc: the path to the directory which contains encoded file set (with index)" << std::endl;
		std::cout << "\t     to be displayed" << std::endl;
		std::cout << std::endl;
	} else if (streq(av[i], "--faq")) {
		std::cout << "[NOTEC FILESYSTEM DESC]" << std::endl;
		std::cout << "Encoded file structure is strictly a set of files which size is at least 1." << std::endl;
		std::cout << "One of the files in the set is named 'INDEX.ntc'. It does list all the files" << std::endl;
		std::cout << "and directories existing within the encoded file structure. The file index is" << std::endl;
		std::cout << "not used for file lookup. It is only used for browsing the file structure." << std::endl;
		std::cout << "The encoded file set does not contain any directory. The encoded file set" << std::endl;
		std::cout << "can be moved in any directory and still represent the exact same inner." << std::endl;
		std::cout << "structure." << std::endl;
	} else {
		std::cerr << "Unknown command '" << av[i] << "'" << std::endl;
		std::cerr << "-h or --help for usage." << std::endl;
		return 1;
	}
	return 0;
}