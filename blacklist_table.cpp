#include "blacklist_table.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include <cmath>

#include "gary_string.h"
#include "darts.h"
#include "Platform/bchar.h"
#include "Platform/encoding.h"
#include "Platform/encoding/support.h"
#include "Platform/encoding/EncodingConvertor.h"
//#include "glog/logging.h"

using namespace std;
	
#ifdef __DEBUG_ON_
#define debug_log fprintf
#else
#define debug_log if(0)
#endif
	
using namespace std;
using namespace spaceGary;

BlacklistTable::BlacklistTable()
{
	m_num_blacklist_entry = 0;
	m_blacklist_entry_buf = NULL;
}

BlacklistTable::~BlacklistTable() 
{
	if(m_blacklist_entry_buf != NULL)
	{
		delete []m_blacklist_entry_buf;
		m_blacklist_entry_buf = NULL;
	}

}

// 对原始数据建立索引
//输入file格式 query\t prefix|postfix
int32_t BlacklistTable::MakeIndex(
		const char *file, 
		const char *index_file,
		const string _scource)
{
	//从输入文件中将数据读出
	FILE * fp_in = fopen(file, "r");
	if (NULL == fp_in) 
	{
		cerr << "can't not open file:" << file <<endl;
		return -1;
	}

	vector<BlacklistEntryTmp> blacklist_tmp_vect;
	char line[10240];
	int32_t boost_num = 0;
	while (fgets(line, 10240, fp_in)) 
	{
		string str_line = line;
		StringTrim(str_line);
		vector<string> parts_vect;
		StringSplit(str_line, parts_vect, "\t");

		//去掉脏数据的情况
		int32_t num_parts = parts_vect.size();
		if (num_parts < 7) 
		{
			cerr << "pair format error:" << line;
			continue;
		}


		string source = parts_vect[0];
		string query = parts_vect[1];
		string fTitleBlack = parts_vect[2];
		string fQueryBlack = parts_vect[3];
		string fPeopleSet = parts_vect[4];
		string fBlackSet = parts_vect[5];
		string fCombineBlack = parts_vect[6];

		// 判断source 是否是需要的
		if (_scource!="all" && source != _scource) {
			cerr << "not require source, filter it [" << line << "]" << endl;
			continue;
		}

		StringTrim(query);
		if (query.length() > kMaxBlackQueryLen) 
		{
			cerr << "black query len max error:" << line;
			continue;
		}

		//半角转全角
		char buffer[kMaxStrLen];                                  
		memset(buffer, 0,  kMaxStrLen);
		int len = 
			EncodingConvertor::getInstance()->t2sgchar(
				query.c_str(), 
				(gchar_t*)buffer, 
				kMaxStrLen/2, true);
		if( len >= kMaxStrLen/2 || len < 0)
		{
			cerr<<"encoding convert error:"
				<<line<<"ret="<<len<<endl;
			continue;
		}
		buffer[len*2] = '\0';
		string gQuery(buffer);

		// 默认采用包含匹配 kMatchTypePart
		//int32_t match_type = GetMatchType(match_type_str);
		int32_t match_type = kMatchTypePart;
		// 每个query的黑名单类别
		int32_t query_type = 0;
		if (fTitleBlack == FLAG_YES) {
			query_type = query_type | kHitTitleBlack;
		} if (fQueryBlack == FLAG_YES) {
			query_type = query_type | kHitQueryBlack;
		} if (fPeopleSet == FLAG_YES) {
			query_type = query_type | kHitPeopleSet;
		} if (fBlackSet == FLAG_YES) {
			query_type = query_type | kHitBlackSet;
		} if (fCombineBlack == FLAG_YES) {
			query_type = query_type | kHitCombineBlack;
			if (num_parts > 7) {
				string combineBlackQuery = parts_vect[7];
				StringTrim(combineBlackQuery);
			}
		}
		
		BlacklistEntryTmp blacklist_entry_tmp(
				gQuery,
				query_type,
				match_type,
				gQuery.size());

		blacklist_tmp_vect.push_back(blacklist_entry_tmp);
	}

	//将数据进行转换并输出到index文件中
	if(OutputIndexFile(blacklist_tmp_vect, index_file) < 0)
	{
		fclose(fp_in);
		return -1;
	}

	fclose(fp_in);

	return 0;
}

//输出index文件
int32_t BlacklistTable::OutputIndexFile(
		std::vector<BlacklistEntryTmp> & blacklist_vect,
		const char * index_file) {

	// 对phrase结果进行排序
	sort(blacklist_vect.begin(), blacklist_vect.end());

	//进行数据转换
	vector<const char *> da_key_vect;
	vector<int32_t> da_value_vect;
	vector<char> char_vect;
	vector<BlacklistEntry> blacklist_entry_vect;
	int index = 0;

	for(vector<BlacklistEntryTmp>::iterator
			it = blacklist_vect.begin();
			it != blacklist_vect.end();
			++it)
	{
		string query = it->query;
		BlacklistEntry entry(
				it->query_type,
				it->match_type,
				it->word_len
				);
		blacklist_entry_vect.push_back(entry);
		da_key_vect.push_back(query.c_str());
		da_value_vect.push_back(index);
		++index;
	}

	if(m_da_blacklist.build(da_key_vect.size(),
				&da_key_vect[0], 0, 
				&da_value_vect[0], NULL) != 0)
	{
		fprintf(stderr, "build da error!\n");
		return -1;
	}

	// 写入输出文件
	FILE * fp_index = fopen(index_file, "wb");
	if (NULL == fp_index) 
	{
		fprintf(
				stderr, "Can not open file [%s]!\n",
				index_file);
		return -1;
	}

	// 保留file size字段
	int64_t index_file_size = 0;
	fwrite(&index_file_size, 
			sizeof(index_file_size), 1, fp_index);

	// 写入da size
	int64_t da_size =
		m_da_blacklist.size() * m_da_blacklist.unit_size();
	fwrite(&da_size, sizeof(da_size), 1, fp_index);

    // 写入源的da
	fclose(fp_index);

	if (m_da_blacklist.save(index_file, "ab") != 0) 
	{
		fprintf(stderr, "save index error!\n");
		return -1;
	}

	// 写入blacklist entry option
	fp_index = fopen(index_file, "ab");
	m_num_blacklist_entry = blacklist_entry_vect.size();
	fwrite(&m_num_blacklist_entry, sizeof(m_num_blacklist_entry), 
			1, fp_index);
	fwrite(&(blacklist_entry_vect[0]), 
			sizeof(blacklist_entry_vect[0]),
			blacklist_entry_vect.size(), fp_index);

	// 写入文件总长度
	fflush(fp_index);
	fp_index = fopen(index_file, "r+b");
	struct stat file_info;
	if (fstat(fileno(fp_index), &file_info) == -1) 
	{
		fprintf(stderr, 
				"can not get index file info from file [%s]!\n", index_file);
		return -1;
	}
	index_file_size = file_info.st_size;
	fwrite(&index_file_size, 
			sizeof(index_file_size), 1, fp_index);

	fclose(fp_index);

	return 0;
}

// 读入索引
int32_t BlacklistTable::LoadIndex(
		const char *index_file)
{
	FILE *fp_in = fopen(index_file, "rb");
	if (NULL == fp_in) 
	{
		fprintf(stderr, 
				"can not open file [%s]!\n", index_file);
		return -1;
	}

	int64_t index_file_size = 0;
	fread(&index_file_size, 
			sizeof(index_file_size), 1, fp_in);
    
	struct stat file_info;
	if (fstat(fileno(fp_in), &file_info) == -1) 
	{
		fprintf(stderr, 
				"can not get file info from file [%s]!\n", 
				index_file);
		return -1;
	}

	if(index_file_size != file_info.st_size) 
	{
		fprintf(stderr, 
				"index file [%s] size error!, size = %lu, true size = %lu!\n",
			index_file, index_file_size, file_info.st_size); 
		return -1;
	}
    
	// 清除原先的index
	ClearIndex();

    //读出 da
	int64_t da_size = 0;
	if (fread(&da_size, sizeof(da_size), 1, fp_in) != 1) 
	{
		fprintf(stderr, "read da size error!\n");
		return -1;
	}
    fclose(fp_in);

	if(m_da_blacklist.open(index_file, "rb",
				sizeof(index_file_size) + sizeof(da_size),
				da_size) != 0) 
	{
		fprintf(stderr, 
				"read da error in file [%s]!\n", index_file);
		return -1;
	}

	fp_in = fopen(index_file, "rb");
	fseek(fp_in, 
			sizeof(index_file_size) + sizeof(da_size) + 
			da_size, SEEK_SET);
    fprintf(stderr, "Load DA OK!\n");

	// read blacklist entry option
	if(fread(&m_num_blacklist_entry, 
				sizeof(m_num_blacklist_entry), 1, fp_in) != 1) 
	{
		fprintf(stderr,
				"can not read m_num_blacklist_entry!\n");
		return -1;
	}

	m_blacklist_entry_buf = new BlacklistEntry[
		m_num_blacklist_entry];
	if(NULL == m_blacklist_entry_buf) 
	{
		fprintf(stderr, 
				"alloc m_num_blacklist_entry_buf error!\n");
		return -1;
	}

	if(fread(m_blacklist_entry_buf, 
				sizeof(m_blacklist_entry_buf[0]), 
				m_num_blacklist_entry, fp_in) != 
			(int64_t)m_num_blacklist_entry) 
	{
		fprintf(stderr, "read m_trans_option_buf error!\n");
		return -1;
	}
	fprintf(stderr, "[Blacklist Load]Load Blacklist Entry OK!\n");

	fclose(fp_in);
	fp_in = NULL;

	return 0;
}


// 清除索引
int32_t BlacklistTable::ClearIndex() 
{
	m_da_blacklist.clear();

	return 0;
}



//BlacklistEntry * BlacklistTable::GetBlacklistEntry(
//		int32_t idx) 
//{
//
//	return m_blacklist_entry_buf + idx;
//}


// 在双数组上游走
int32_t BlacklistTable::Traverse(
		const char * key, size_t & node_pos, 
		size_t &key_pos, size_t length) 
{
	return 
		m_da_blacklist.traverse(key, node_pos, key_pos, length);

}

int32_t BlacklistTable::GetQueryType(
		string& query_type_str)
{
	StringTrim(query_type_str);
	vector<string> parts_vec;
	StringSplit(query_type_str,parts_vec,"|");
	int ret = 0;
	for (int i=0; i<parts_vec.size(); ++i) {
		if(parts_vec[i] == "black") {
			ret |= kQueryTypeBlack;
		}
		else if(parts_vec[i] == "white") {
			ret |= kQueryTypeWhite;
		}
	}
	return ret;
}

int32_t BlacklistTable::GetMatchType(
		string& match_type_str)
{
	StringTrim(match_type_str);
	vector<string> parts_vec;

	StringSplit(match_type_str,parts_vec,"|");

	int ret = 0;
	for(int i = 0; i < parts_vec.size(); ++i)
	{
		if(parts_vec[i] == "prefix")
		{
			ret |= kMatchTypePrefix;
		}
		else if(parts_vec[i] == "suffix")
		{	
			ret |= kMatchTypePostfix;
		}
		else if(parts_vec[i] == "part")
		{
			ret |= kMatchTypePart;
		}
		else if(parts_vec[i] == "whole")
		{
			ret |= kMatchTypeWhole;
		}
		else
		{
			cerr<<"[Blacklist Make]"<<
				"get match type error:"<<
				parts_vec[i]<<endl;
		}

	}
	return ret;
}

int32_t BlacklistTable::BlackWhiteHit(
		const std::string& query
		)
{
	if(query.size() <= 0)
		return kHitTypeNull;
	for(size_t i = 0; i < query.size()/2; ++i)	
	{
		string tmp_query = query.substr(i*2);
		vector<int> hit;
		CommonPrefixSearch(
				tmp_query.c_str(),hit,kMaxHitNum);

		for(size_t j = 0; j < hit.size(); j++)
		{
			BlacklistEntry black_entry = 
				m_blacklist_entry_buf[hit[j]];
			//prefix || part || whole || postfix
			if(0 == i)
			{
				if((black_entry.match_type & kMatchTypePart)!=0 || (black_entry.match_type & kMatchTypePrefix) != 0)
				{
					return black_entry.query_type;
				}
				else if((black_entry.match_type & kMatchTypeWhole) != 0)
				{
					//全命中
					if(black_entry.word_len == query.size())
					{
						return black_entry.query_type;
					}
				}
			}
			//part || suffix
			else
			{
				if( (black_entry.match_type & kMatchTypePart) != 0)
				{
					//cout << i << "\t" << black_entry.word_len/2 << "\t" << query.substr(i*2, black_entry.word_len) << endl;
					return black_entry.query_type;
				}
				else if( (black_entry.match_type & kMatchTypePostfix) != 0)
				{
					if(i*2 + black_entry.word_len == query.size())
					{
						return black_entry.query_type;
					}
				}
			}
		}

	}

	return kHitTypeNull;
}


void BlacklistTable::BlackWhiteMultiHit(
		const std::string& query,
		HitEntry &hintEntry
		)
{
	if(query.size() <= 0) {
		return;
	}
	for(size_t i = 0; i < query.size()/2; ++i)	
	{
		string tmp_query = query.substr(i*2);
		vector<int> hit;
		CommonPrefixSearch(
				tmp_query.c_str(),hit,kMaxHitNum);
		for(size_t j = 0; j < hit.size(); j++)
		{
			BlacklistEntry black_entry = 
				m_blacklist_entry_buf[hit[j]];
			string hitQuery = query.substr(i*2, black_entry.word_len);
			cout << "hit " << query << "\t" <<hitQuery << "\t" << black_entry.query_type << endl;

			if (black_entry.query_type & kHitTitleBlack) {
				hintEntry.hitTitleBlack = true;
			} if (black_entry.query_type & kHitQueryBlack) {
				hintEntry.hitQueryBlack = true;
			} if (black_entry.query_type & kHitPeopleSet) {
				hintEntry.hitPeopleSet = true;
			} if (black_entry.query_type & kHitBlackSet) {
				hintEntry.hitBlackSet = true;
			} if (black_entry.query_type & kHitCombineBlack) {
				//string hitQuery = query.substr(i*2, black_entry.word_len);
				hintEntry.CombineMatchVec.push_back(hitQuery);
			}
		}
	}
}




int32_t BlacklistTable::CommonPrefixSearch(
		const std::string& query,
		vector<int>& hit_result,
		int32_t max_hit_num)
{
	Darts::DoubleArray::result_type* result_value =
		new Darts::DoubleArray::result_type[max_hit_num];
	if(result_value == NULL)
	{
		fprintf(stderr,"alloc result value buffer error!,alloc size is [%d]\n",max_hit_num);
		return -1;
	}

	hit_result.clear();
	size_t ret_value = m_da_blacklist.commonPrefixSearch(
			query.c_str(),result_value,max_hit_num);

	for(size_t i = 0; i < ret_value; i++)
	{
		hit_result.push_back(result_value[i]);
	}

	delete []result_value;

	return hit_result.size();
}

int32_t BlacklistTable::HitBlacklist(
		const string& query,
		int32_t query_type)
{
	if( HitWholeQuery(query,query_type) )
	{
		return kMatchTypeWhole;
	}
	else if( HitPrefixQuery(query,query_type) )
	{
		return kMatchTypePrefix;
	}
	else if( HitPostfixQuery(query,query_type) )
	{
		return kMatchTypePostfix;
	}
	else if( HitPartQuery(query,query_type) )
	{
		return kMatchTypePart;
	}
	else
		return 0;
}

bool BlacklistTable::HitPrefixQuery(
		const string& query,
		int32_t query_type)
{
	for(int i=1; i < query.length()/2; ++i)
	{
		string tmp = query.substr(0,i*2);
		Darts::DoubleArray::result_type ret_value;
		m_da_blacklist.exactMatchSearch(
				tmp.c_str(),ret_value);
		
		if(ret_value > -1)
		{
			BlacklistEntry entry = 
				m_blacklist_entry_buf[ret_value];
			if(	(int32_t)(entry.match_type & kMatchTypePrefix) > 0)
			{
				//cerr<<"[Blacklist Table][query]"<<query
				//	<<" hit prefix blacklist:"<<tmp
				//	<<endl;
					return true;
			}
		}

	}
	return false;
}

bool BlacklistTable::HitPostfixQuery(
		const string& query,
		int32_t query_type)
{
	int len = query.length()/2;
	for(int i=len-1 ; i > 0; --i )
	{
		string tmp = query.substr(i*2);

		//cerr<<tmp<<endl;
		
		Darts::DoubleArray::result_type ret_value;
		m_da_blacklist.exactMatchSearch(
				tmp.c_str(),ret_value);

		
		if(ret_value > -1)
		{
			BlacklistEntry entry = 
				m_blacklist_entry_buf[ret_value];

			if((int32_t)(entry.match_type & kMatchTypePostfix) > 0)

			{
				//cerr<<"[Blacklist Table][query]"<<query
				//	<<" hit postfix blacklist:"<<tmp
				//	<<endl;
					return true;
			}
		}

	}
	return false;

}

bool BlacklistTable::HitPartQuery(
		const string& query,
		int32_t query_type)
{
	int len = query.length()/2;

	for(int i = 0; i < len; ++i)
	{
		for(int j = 1; j <= len-i; ++j)
		{
			string tmp = query.substr(i*2,j*2);
			//cerr<<tmp<<std::endl;
			Darts::DoubleArray::result_type ret_value;

			m_da_blacklist.exactMatchSearch(
					tmp.c_str(),ret_value);

			if(ret_value > -1)
			{
				BlacklistEntry entry =
					m_blacklist_entry_buf[ret_value];
				if((int32_t)(entry.match_type & kMatchTypePart) > 0 )
				{
					//cerr<<"[Blacklist Table][query]"<<query
					//	<<" hit part blacklist:"<<tmp
					//	<<endl;
					return true;
				}
			}
		}
	}

	return false;

}


bool BlacklistTable::HitWholeQuery(
		const string& query,
		int32_t query_type)
{
	Darts::DoubleArray::result_type ret_value;
	m_da_blacklist.exactMatchSearch(
			query.c_str(),ret_value);

	if(ret_value > -1)
	{
		BlacklistEntry entry = m_blacklist_entry_buf[ret_value];
		
		if((int32_t)(entry.match_type & kMatchTypeWhole) > 0)
		{
			//cerr<<"[Blacklist Table][query]"<<query<<
			//	" hit black list whole query:"<<query
			//	<<endl;
			return true;
		}
	}
	return false;
}

