#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <sstream>
#include <filesystem>

namespace std {
namespace fs = filesystem;
}

static void co(const std::set<char> &opt, const char *src, const char *dst)
{
	static_cast<void>(opt);
	static_cast<void>(src);
	static_cast<void>(dst);
}

static void dec(const std::set<char> &opt, const char *src, const char *dst)
{
	static_cast<void>(opt);
	static_cast<void>(src);
	static_cast<void>(dst);
}

static void tree(const std::set<char> &opt, const char *src)
{
	static_cast<void>(opt);
	static_cast<void>(src);
}

static inline bool streq(const char *a, const char *b)
{
	return std::strcmp(a, b) == 0;
}

static inline std::set<char> get_options(size_t &i, size_t ac, const char * const *av, const std::set<char> &auth)
{
	std::set<char> res;

	while (i < ac && av[i][0] == '-') {
		auto a = av[i];
		a++;
		while (auto c = *a) {
			if (auth.find(c) == auth.end()) {
				std::stringstream ss;
				ss << "Unknown option '" << c << "'";
				throw std::runtime_error(ss.str());
			}
			if (res.find(c) != res.end()) {
				std::stringstream ss;
				ss << "Duplicated option '" << c << "'";
				throw std::runtime_error(ss.str());
			}
			res.emplace(c);
			a++;
		}
		i++;
	}
	return res;
}

int main(int argc, const char * const *argv)
{
	auto av = argv + 1;
	argc--;
	size_t ac = argc;
	size_t i = 0;
	try {
		if (i >= ac)
			throw std::runtime_error("Expected at least one argument.");
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
		} else if (streq(av[i], "--faq")) {
			std::cout << "[NOTEC FILESYSTEM DESC]" << std::endl;
			std::cout << "Encoded file structure is strictly a set of files which size is at least 1." << std::endl;
			std::cout << "One of the files in the set is named 'INDEX.ntc'. It does list all the files" << std::endl;
			std::cout << "and directories existing within the encoded file structure. The file index is" << std::endl;
			std::cout << "not used for file lookup. It is only used for browsing the file structure." << std::endl;
			std::cout << "The encoded file set does not contain any directory. The encoded file set" << std::endl;
			std::cout << "can be moved in any directory and still represent the exact same inner." << std::endl;
			std::cout << "structure." << std::endl;
		} else if (streq(av[i], "co")) {
			i++;
			auto ops = get_options(i, ac, av, {'r', 'f', 'm', 'w'});
			if (ac - i != 2)
				throw std::runtime_error("Expected two last args which are not options");
			co(ops, av[i], av[i + 1]);
		} else if (streq(av[i], "dec")) {
			i++;
			auto ops = get_options(i, ac, av, {'f', 'w'});
			if (ac - i != 2)
				throw std::runtime_error("Expected two last args which are not options");
			dec(ops, av[i], av[i + 1]);
		} else if (streq(av[i], "tree")) {
			i++;
			auto ops = get_options(i, ac, av, {});
			if (ac - i != 1)
				throw std::runtime_error("Expected one last arg which is not options");
			tree(ops, av[i]);
		} else {
			std::stringstream ss;
			ss << "Unknown command '" << av[i] << "'";
			throw std::runtime_error(ss.str());
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "-h or --help for usage." << std::endl;
		return 1;
	}
	return 0;
}