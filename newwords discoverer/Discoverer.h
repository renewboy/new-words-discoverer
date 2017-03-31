#ifndef __Discoverer_h__
#define __Discoverer_h__

#include <string>
#include <unordered_map>

namespace new_words_discover {

// The thresholds for the algorithm
struct Thresholds
{
	size_t freq_thr;
	double firmness_thr;
	double free_thr;
	size_t max_word_len;
};

// The set of left adjacent Chinese characters. <key, frequency>
using left_adjacent_t = std::unordered_map<wchar_t, size_t>;

// The set of right adjacent Chinese characters. <key, frequency>
using right_adjacent_t = std::unordered_map<wchar_t, size_t>;
using frequency_t = size_t;
using firmness_t = double;
using word_t = std::tuple<frequency_t, left_adjacent_t, right_adjacent_t, firmness_t>;

class Discoverer
{
public:
	Discoverer(std::string filename,Thresholds thds)
		:filename_(filename), thresholds_(thds){}
	void process();
private:
	int parse_file();
	void parse_sentence(const std::wstring& sentence, size_t word_len);
	void remove_words_by_firmness();
	void calculate_firmness(std::pair<const std::wstring, word_t>& word);
	double calculate_degree_of_freedom(const std::pair<std::wstring, word_t>& word);
	double entropy(const std::unordered_map<wchar_t, frequency_t>& adjacents);
	void remove_words_by_freq_and_word_len();
	void remove_words_by_degree_of_freedom();
	void print();
	std::unordered_map <std::wstring, word_t> words_;
	std::string filename_;
	Thresholds thresholds_; 
	size_t tot_frequency_ = 0;
};

}
#endif // !__Discoverer_h__
