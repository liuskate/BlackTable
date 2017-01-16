/*******************************************************

* 文件名： gary_string.h
* 功  能： 针对string的封装
* 版  本： version 1.0 2010-3-31
* 修  改：

********************************************************/

#ifndef __GARY_STRING_H_
#define __GARY_STRING_H_

#include <string>
#include <vector>
#include <string.h>
#include <string.h>
#include <stdlib.h>

//using namespace std;

namespace spaceGary
{

//split
inline
bool StringSplit(std::string strItem, std::vector<std::string> &vectRes, const char* splitWord) {
    vectRes.clear();

	std::string::size_type posOnePart, posFindBegin = 0;
	unsigned int nSplitWordLen = strlen(splitWord);
	posOnePart = strItem.find(splitWord);
	
	while (std::string::npos != posOnePart) {
		
		//不能将splitword， 加入到结果中
		if (posOnePart != posFindBegin) {
			vectRes.push_back(strItem.substr(posFindBegin, posOnePart-posFindBegin));
		}
		posFindBegin = posOnePart + nSplitWordLen;
		posOnePart = strItem.find(splitWord, posFindBegin);
	}

	//防止后面出现splitword的情况
	if (posFindBegin < strItem.length()) {
		vectRes.push_back(strItem.substr(posFindBegin));
	}
    
	return true;
};

inline
bool DeleteSpace(
		const char * src,
		char * dest, unsigned int max_len)
{
	if(NULL == src || strlen(src) > max_len)
	{
		return false;
	}
	int pos_dest = 0;
	const char * p = src;
	bool is_first = true;
	while(*p)
	{
		if(*p < 0)
		{
			if(*(p+1) == '\0')
			{
				return false;
			}
			dest[pos_dest++] = *p;
			dest[pos_dest++] = *(p+1);
			p+=2;
			if(is_first == true)
			{
				is_first = false;
			}
		}
		else if(*p == ' ')
		{
			if(is_first == true)
			{
			}
			else if(*(p+1) == '\0')
			{

			}
			else if(*(p+1) == ' ')
			{
			}
			else
			{
				dest[pos_dest++] = *p;
			}
			p++;
		}
		else
		{
			dest[pos_dest++] = *p;
			p++;
			if(is_first == true)
			{
				is_first = false;
			}
		}
	}
	dest[pos_dest] = '\0';
	return true;
}

//seg split
inline
bool SegSplit(std::string strItem, std::vector<std::string> &vectRes, const char* splitWord) {
   
	vectRes.clear();

	std::string::size_type posOnePart, posFindBegin = 0;
	unsigned int nSplitWordLen = strlen(splitWord);
	posOnePart = strItem.find(splitWord);
	bool is_pre_seg = false;

	while (std::string::npos != posOnePart) {
		
		//处理分隔符和原词一致， 并联系出现的情况
		if  (posOnePart == posFindBegin) {
			if (is_pre_seg) {
				vectRes.push_back(splitWord);
			}
			is_pre_seg = true;
		}
		else {
			vectRes.push_back(strItem.substr(posFindBegin, posOnePart-posFindBegin));
			is_pre_seg = false;
		}

		posFindBegin = posOnePart + nSplitWordLen;
		posOnePart = strItem.find(splitWord, posFindBegin);
	}

	//防止后面出现splitword的情况
	if (posFindBegin < strItem.length()) {
		vectRes.push_back(strItem.substr(posFindBegin));
	}
    
	return true;
};

//trim
inline
bool StringTrim(std::string &strItem) {

 
	const char *pItem = strItem.c_str();
	unsigned int nItemLen = strItem.length();
    if (0 ==  nItemLen) {
		return true;
	}
	int left = 0;
	while (pItem[left] == ' ' || pItem[left] == '\t' || pItem[left] == '\r' || pItem[left] == '\n') {
		left++;
	}

	int right = nItemLen-1;
	while (right >=0 && (pItem[right] == ' ' || pItem[right] == '\t' || pItem[right] == '\r' || pItem[right] == '\n')) {
		right--;
	}

	if (left > right) {
		strItem = "";
	}
	else {
		strItem = strItem.substr(left, right-left+1);
	}

	return true;
}

inline 
int StringJoin(const std::vector<std::string>& parts,const char* splitword,std::string& result)
{
	if(parts.size() < 1)
		return 0;

	result = "";
	result += parts[0];

	for(size_t i = 1; i < parts.size(); ++i)
	{
		result += splitword + parts[i];
	}

	return 0;
}

inline
int GetWordNum(const char *src) {

	int num = 0;

	const char *p = src;
	while (*p) {
		if (*p == ' ') {
			p++;
		}
		while (*p) {
			if (*p < 0) {
				if (*(p + 1) == '\0') {
					return num + 1;
				}
				p += 2;
			}
			else if (*p == ' ') {
				p++;
				num++;
				break;
			}
			else {
				p++;
			}
		}
	}

	return num + 1;
}

inline 
const char* GetTheWord(char *src) {
	
	char *p = src;
	if (*p == ' ') {
		p++;
	}
	while (*p && *p != ' ') {
		if (*p < 0) {
			p += 2;
		}
		else {
			p++;
		}
	}

	*p = 0;

	return src;

}

inline
const char *GetEngWord(const char *query, int &len_eng, int &num_eng_word) {

	int cur_num_eng_word = 0;
	int cur_len_eng = 0;
	len_eng = 0;

	const char *p = query;
	while (*p) {
		if (*p == ' ') {
			
			len_eng = cur_len_eng;
			cur_len_eng++;
			p++;
			cur_num_eng_word++;
			if (cur_num_eng_word >= num_eng_word) {
				break;
			}
			
		}
		else if (*p >= 'a' && *p <= 'z') {
			cur_len_eng++;
			p++;
		}
		else {
			break;
		}
	}

	num_eng_word = cur_num_eng_word;
	if (0 == len_eng) {
		return NULL;
	}

	return query;
	
}

};


#endif


