#include "Discoverer.h"
#include <iostream>
#include <fstream>
#include <locale>
#include <regex>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>

using namespace boost::program_options;
using namespace std;

void parse_vm(const options_description &opts, const variables_map &vm)
{
	if (vm.find("filename") == vm.end()) {
		cout << "invaild options." << endl;
		cout << opts << endl;
		exit(1);
	}

	if (vm.count("help")) 
	{
		cout << opts << endl;
	}
}

int main(int argc, char* argv[]) 
{
	locale::global(std::locale(""));
	options_description opts("Usage: options_description [options]");
	new_words_discover::Thresholds thds;
	wstring filename;
	try
	{		
		opts.add_options()
			("help,h", "produce help message")
			("filename,f", wvalue<wstring>(&filename), "the file to process")
			("freq", value<size_t>(&(thds.freq_thr))->default_value(3), "frequcency")
			("firm", value<double>(&(thds.firmness_thr))->default_value(500.0), "frimness")
			("free", value<double>(&(thds.free_thr))->default_value(2.0), "degree of freedom")
			("wordlen,l", value<size_t>(&(thds.max_word_len))->default_value(5), "maximum word length");
		variables_map vm;
		store(parse_command_line(argc, argv, opts), vm);
		notify(vm);

		parse_vm(opts, vm);
	}
	catch (...)
	{
		cout << opts << endl;
		exit(1);
	}

	new_words_discover::Discoverer d(filename, thds);
	d.process();

	return 0;
}