#include <boost/program_options.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <locale>

#include "Discoverer.h"

using namespace boost::program_options;
using std::cout;
using std::endl;

void parse_vm(const options_description &opts, const variables_map &vm)
{
    if (vm.count("help")) {
        cout << opts << endl;
    }
}

int main(int argc, char *argv[])
{
    std::locale::global(std::locale(""));
    options_description opts("Usage");
    new_words_discover::Thresholds thrs;
    std::string filename;
    try {
        opts.add_options()("help,h", "produce help message")(
            "file,f", value<std::string>(&filename)->required(), "the file to process, required")(
            "freq", value<size_t>(&(thrs.freq_thr))->default_value(3), "frequcency")(
            "firm", value<double>(&(thrs.firmness_thr))->default_value(350.0), "frimness")(
            "df", value<double>(&(thrs.df_thr))->default_value(2.0), "degree of freedom")(
            "wordlen,l", value<size_t>(&(thrs.max_word_len))->default_value(4),
            "maximum word length");
        variables_map vm;
        store(parse_command_line(argc, argv, opts), vm);
        notify(vm);
        parse_vm(opts, vm);
    } catch (...) {
        cout << "invaild option(s): ";
        for (int i = 1; i < argc; i++) {
            cout << argv[i] << std::ends;
        }
        cout << endl << opts << endl;
        exit(1);
    }
    auto start = std::chrono::steady_clock::now();
    new_words_discover::Discoverer d(filename, thrs);
    d.process();
    auto end = std::chrono::steady_clock::now();
    using dd = std::chrono::duration<double>;
    cout << "Total time elapsed :" << std::chrono::duration_cast<dd>(end - start).count()
         << " seconds.\n";
    return 0;
}