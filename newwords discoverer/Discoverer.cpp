#include "Discoverer.h"
#include <regex>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <future>
#include <condition_variable>
#include <boost/algorithm/string.hpp>     

namespace
{
	std::condition_variable g_cv_sentence;
	std::mutex g_mutex_sentence;
}
namespace new_words_discover {

void Discoverer::process()
{
	start_sentence_parser();
	//std::this_thread::sleep_for(std::chrono::milliseconds(10));
	std::cout << "proccessing file " << filename_ + "...\n";
	if (parse_file() < 0)
	{
		return;
	}
	std::cout << "done.\n";

	std::cout << "calculating firmness...\n";
	remove_words_by_firmness();
	std::cout << "done.\n";
	remove_words_by_freq_and_word_len();

	std::cout << "calculating degree of freedom...\n";
	remove_words_by_degree_of_freedom();
	std::cout << "done.\n";
	print();
}

void Discoverer::start_sentence_parser()
{
	sentence_parser_ = std::thread([this]
	{
		while (!file_parse_done_)
		{
			std::unique_lock<std::mutex> lck(g_mutex_sentence);
			g_cv_sentence.wait(lck, [this]
			{
				return !sentence_list_.empty();
			});
		
			std::wstring copy = sentence_list_.front();
			sentence_list_.pop();
			lck.unlock();
			parse_sentence(copy);			
		}
		
		while (!sentence_list_.empty())
		{
			const std::wstring& sentence = sentence_list_.front();
			parse_sentence(sentence);
			sentence_list_.pop();
		}
		
	});
}

int Discoverer::parse_file()
{
	std::wifstream corpus(filename_);
	if (!corpus.is_open())
	{
		std::cerr << "Failed to open " + filename_ + "!" << std::endl;
		return -1;
	}
	std::wstring paragraph;

	// Warning C4129 Unrecognized character escape sequence.
#pragma warning(disable:4129) 
	std::wregex re(L"\W+|[a-zA-Z0-9]+|\s+|\n+");
#pragma warning(default:4129)

	std::cout << "calculating word frequency...\n";
	while (std::getline(corpus, paragraph))
	{		
		if (paragraph.empty())
		{
			continue;
		}
		std::vector<std::wstring> para_vec;
		// Warning C4996 'std::copy::_Unchecked_iterators::_Deprecate': 
		// Call to 'std::copy' with parameters that may be unsafe.
#pragma warning(disable:4996) 
		boost::algorithm::split(para_vec, paragraph, boost::is_any_of(L"��������������������������������������"));
#pragma warning(default:4996) 

		for (auto& segment : para_vec)
		{
			std::wsregex_token_iterator it(segment.begin(), segment.end(), re, -1);
			std::wsregex_token_iterator end_it;
			while (it != end_it)
			{
				std::wstring&& cur_sentence=it->str();
				it++;
				boost::trim(cur_sentence);
				{
					std::lock_guard<std::mutex> lck(g_mutex_sentence);
					sentence_list_.emplace(cur_sentence);
				}
				g_cv_sentence.notify_one();
			}
		}
	}
	file_parse_done_ = true;
	sentence_parser_.join();
	std::cout << "done.\n";
	return 0;
}

void Discoverer::parse_sentence(const std::wstring & sentence)
{
	auto sen_length = sentence.size();
	if (sen_length > 0)
	{
		for (size_t word_len = 1; word_len <= std::min(thresholds_.max_word_len, sen_length); word_len++)
		{
			parse_word(sentence, word_len);
		}
	}
}

void Discoverer::parse_word(const std::wstring & sentence, size_t word_len)
{
	// Loop for the sentence to get all potential words.
	for (size_t j = 0; j + word_len <= sentence.size(); j++)
	{
		auto word = sentence.substr(j, word_len);

		wchar_t left_adja = 0;
		wchar_t right_adja = 0;
		if (j > 0)
		{
			// get the set of left adjacent Chinese character.
			left_adja = sentence[j - 1];
			std::get<1>(words_[word])[left_adja]++;
		}
		if (j + word_len < sentence.size())
		{
			// get the set of right adjacent Chinese character. 
			right_adja = sentence[j + word_len];
			std::get<2>(words_[word])[right_adja]++;
		}
		std::get<frequency_t>(words_[word])++;
		tot_frequency_++;
	}
}

void Discoverer::remove_words_by_firmness()
{
	// Calculate firmness concurrently.
	auto dist = std::distance(words_.begin(), words_.end());
	auto low = words_.begin();
	auto high = words_.begin();
	auto step = dist / 4;      // Seperate into 4 or 5 parts.
	std::advance(high, step);
	std::vector<std::future<void>> fu_vec;
	while (std::distance(high, words_.end()) >= 0)
	{
		// Go!
		fu_vec.push_back(std::async(std::launch::async, [this, low, high]
		{
			calculate_firmness(low, high);
		}));
		if (high == words_.end())
		{
			break;
		}
		std::advance(low, step);
		if (std::distance(high, words_.end()) >= step)
		{
			std::advance(high, step);
		}
		else
		{
			std::advance(high, std::distance(high, words_.end()));
		}
	}
	// Waiting for the future objects.
	for (auto& fu : fu_vec)
	{
		fu.get();
	}
	for (auto it = words_.begin(); it != words_.end();)
	{
		if (words_.size() > 1 &&
			std::get<firmness_t>(it->second) < thresholds_.firmness_thr)
		{
			it = words_.erase(it);
		}
		else
		{
			it++;
		}
	}
}

void Discoverer::calculate_firmness(std::pair<const std::wstring, word_t>& word)
{
	size_t p_word = std::get<frequency_t>(word.second);
	double min_firmness = std::numeric_limits<double>::max();
	for (size_t i = 0; i < word.first.size(); i++)
	{
		const auto& left_part = words_[word.first.substr(0, i + 1)];
		const auto& right_part = words_[word.first.substr(i + 1, word.first.size() - i - 1)];
		auto ans = static_cast<double>(p_word) * tot_frequency_ / (std::get<frequency_t>(left_part) * std::get<frequency_t>(right_part));
		if (ans < min_firmness)
		{
			min_firmness = ans;
		}
	}
	std::get<firmness_t>(word.second) = min_firmness;
}

void Discoverer::calculate_firmness(word_map_iter begin, word_map_iter end)
{
	for (auto it = begin; it != end; it++)
	{
		if (it->first.size() > 1)
		{
			calculate_firmness(*it);
		}
	}
}

double Discoverer::calculate_degree_of_freedom(const std::pair<std::wstring, word_t>& word)
{
	const auto& left_adjas = std::get<1>(word.second);
	const auto& right_adjas = std::get<2>(word.second);
	return std::min(entropy(left_adjas), entropy(right_adjas));
}

double Discoverer::entropy(const std::unordered_map<wchar_t, frequency_t>& adjacents)
{
	int tot_freq = 0;
	double ret = 0;
	std::for_each(adjacents.begin(), adjacents.end(), [&tot_freq](auto val)
	{
		tot_freq += val.second;
	});
	std::for_each(adjacents.begin(), adjacents.end(), [&tot_freq, &ret](auto& val)
	{
		ret += (-std::log2(static_cast<double>(val.second) / tot_freq) * (static_cast<double>(val.second) / tot_freq));
	});
	return ret;
}

void Discoverer::remove_words_by_freq_and_word_len()
{
	for (auto it = words_.begin(); it != words_.end();)
	{
		if (it->first.size() < 2 || std::get<frequency_t>(it->second) < thresholds_.freq_thr)
		{
			it = words_.erase(it);
		}
		else
		{
			it++;
		}
	}
}

void Discoverer::remove_words_by_degree_of_freedom()
{
	for (auto it = words_.begin(); it != words_.end();)
	{
		auto freedom = calculate_degree_of_freedom(*it);
		if (freedom < thresholds_.free_thr)
		{
			it = words_.erase(it);
		}
		else
		{
			it++;
		}
	}
}

void Discoverer::print()
{
	std::vector<std::pair<std::wstring, word_t>> sorted(words_.begin(), words_.end());
	std::sort(sorted.begin(), sorted.end(), [](auto& x, auto& y)
	{
		return std::get<frequency_t>(x.second) < std::get<frequency_t>(y.second);
	});

	// Generating output filename by input filename, remove suffix if any. 
	auto dot = filename_.find_last_of('.');
	std::string out_file;
	out_file = (dot != std::string::npos ? filename_.substr(0, dot) : filename_);
	out_file += "_out.txt";
	std::wofstream output(out_file);
	if (!output.is_open())
	{
		std::cerr << "Failed to open " + out_file + "!" << std::endl;
		return;
	}
	output << L"Total words: " << sorted.size() << std::endl;
	for (const auto& s : sorted)
	{
		output << s.first << L" " << std::get<frequency_t>(s.second) << std::endl;
	}
	std::cout << "The results are stored in " + out_file << std::endl;
}
}