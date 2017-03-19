#include "Discoverer.h"
#include <regex>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <boost/algorithm/string.hpp>     

using namespace new_words_discover;

void Discoverer::process()
{
	std::cout << "proccessing file...\n";
	if (parse_file() < 0)
	{
		return;
	}
	std::cout << "done.\n";

	std::cout << "calculating firmness...\n";
	remove_words_by_firmness();
	std::cout << "done.\n";
	
	remove_words_by_freq();
	remove_words_single();

	std::cout << "calculating degree of freedom...\n";
	remove_words_by_degree_of_freedom();
	std::cout << "done.\n";

	print();
}

int Discoverer::parse_file()
{
	std::wifstream corpus(filename_);
	if (!corpus.is_open())
	{
		std::wcerr << L"Failed to open " + filename_ + L"!" << std::endl;
		return -1;
	}
	std::wstring paragraph;
#pragma warning(disable:4129)
	std::wregex re(L"\W+|[a-zA-Z0-9]+|\s+|\n+");
#pragma warning(default:4129)
	std::cout << "calculating word frequency...\n";
	while (std::getline(corpus, paragraph))
	{
		std::vector<std::wstring> para_vec;
		boost::algorithm::split(para_vec, paragraph, boost::is_any_of(L"£¬¡££¿¡¶¡·£¡¡¢£¨£©¡­¡­£»£º¡°¡±¡®¡¯"));
		for (auto& para : para_vec)
		{
			std::wsregex_token_iterator it(para.begin(), para.end(), re, -1);
			std::wsregex_token_iterator end_it;
			while (it != end_it)
			{
				std::wstring sentence = *it++;
				if (sentence.size() > 0)
				{
					for (size_t i = 1; i <= thresholds_.max_word_len; i++)
					{
						parse_sentence(sentence, i);
					}
				}
			}		
		}
		paragraph.clear();
		
	}
	std::cout << "done.\n";
	return 0;
}

void Discoverer::parse_sentence(const std::wstring & sentence, size_t word_len)
{
	size_t i = word_len;
	for (size_t j = 0; j + i < sentence.size() ; j++)
	{
		auto word = sentence.substr(j, i);

		wchar_t left_adja = 0;
		wchar_t right_adja = 0;
		if (j > 0)
		{
			left_adja = sentence[j - 1];
		}
		if (j + i < sentence.size())
		{
			right_adja = sentence[j + i];
		}
		auto ret = words_.find(word);

		if (ret == words_.end())
		{
			left_adjacent_t lmap;
			right_adjacent_t rmap;
			if (left_adja != 0)
			{
				lmap.emplace(left_adja, 1);
			}
			if (right_adja != 0)
			{
				rmap.emplace(right_adja, 1);
			}
			word_t tmp{ 1, lmap, rmap };
			words_.emplace(std::move(word), std::move(tmp));
			tot_frequency++;
		}
		else
		{
			std::get<frequency_t>(ret->second)++;
			tot_frequency++;
			if (left_adja != 0)
			{
				auto& left = std::get<1>(ret->second);
				insert_adjacents(left, left_adja);
			}
			if (right_adja != 0)
			{
				auto& right = std::get<2>(ret->second);
				insert_adjacents(right, left_adja);
			}
		}

	}
}

void Discoverer::insert_adjacents(std::unordered_map<wchar_t, frequency_t>& dest, wchar_t data)
{
	if (dest.find(data) == dest.end())
	{
		dest.emplace(data, 1);
	}
	else
	{
		dest[data]++;
	}
}

void Discoverer::remove_words_by_firmness()
{
	std::unordered_map<std::wstring, double> firmness;

	for (const auto& word : words_)
	{
		if (word.first.size() > 1)
		{
			firmness.emplace(word.first, calculate_firmness(word));
		}	
	}
	for (auto it = words_.begin(); it != words_.end();)
	{
		if (words_.size() > 1 && firmness[it->first] < thresholds_.firmness_thr)
		{
			it = words_.erase(it);
		}
		else
		{
			it++;
		}
	}
}

double Discoverer::calculate_firmness(const std::pair<std::wstring, word_t>& word)
{
	size_t p_word = std::get<frequency_t>(word.second);
	double min_firmness = 0xFFFFFFFF;
	for (size_t i = 0; i < word.first.size(); i++)
	{
		const auto& left_part = words_[word.first.substr(0, i + 1)];
		const auto& right_part = words_[word.first.substr(i + 1, word.first.size() - i - 1)];
		auto ans = static_cast<double>(p_word) * tot_frequency / (std::get<frequency_t>(left_part) * std::get<frequency_t>(right_part));
		if (ans < min_firmness)
		{
			min_firmness = ans;
		}
	}
	return min_firmness;
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

void Discoverer::remove_words_by_freq()
{
	for (auto it = words_.begin(); it != words_.end();)
	{
		if (std::get<frequency_t>(it->second) < thresholds_.freq_thr)
		{
			it = words_.erase(it);
		}
		else
		{
			it++;
		}
	}
}


void Discoverer::remove_words_single()
{
	for (auto it = words_.begin(); it != words_.end();)
	{
		if (it->first.size() < 2)
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
	std::vector<std::pair<std::wstring, word_t>> sorted;
	sorted.assign(words_.begin(), words_.end());
	
	std::sort(sorted.begin(), sorted.end(), [](auto& x, auto& y)
	{
		return std::get<frequency_t>(x.second) < std::get<frequency_t>(y.second);
	});
	std::wstring out_file = L"out.txt";
	std::wofstream output(out_file);
	if (!output.is_open())
	{
		std::wcerr << L"Failed to open " + out_file + L"!" << std::endl;
		return;
	}
	output << L"total: " << sorted.size() << std::endl;
	for (const auto& s : sorted)
	{
		
		output << s.first << L" " << std::get<frequency_t>(s.second) << std::endl;
	}
}

