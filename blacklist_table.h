#ifndef BLACKLIST_TABLE_H_
#define BLACKLIST_TABLE_H_

#include <string>
#include <vector>

#include "darts.h"

const int32_t kQueryTypeBlack=0x0001;
const int32_t kQueryTypeWhite=0x0002;

// 命中类型
const int32_t kHitTypeNull = 0x0000;
const int32_t kHitTypeBlack = 0x0001;
const int32_t kHitTypeWhite = 0x0002;

const int32_t kHitTitleBlack = 0x0001;
const int32_t kHitQueryBlack = 0x0002;
const int32_t kHitPeopleSet = 0x0004;
const int32_t kHitBlackSet = 0x0008;
const int32_t kHitCombineBlack = 0x0010;

const int32_t kMatchTypePrefix=0x0001;
const int32_t kMatchTypePostfix=0x0002;
const int32_t kMatchTypePart=0x0004;
const int32_t kMatchTypeWhole=0x0008;

// 配置文件中标识符号的标志位
const std::string FLAG_NO = "0";
const std::string FLAG_YES = "1";

// Blacklist Entry 黑名单条目
struct BlacklistEntry 
{
	int32_t query_type;//白名单还是黑名单white,black
	int32_t match_type;//匹配方式:prefix,postfix,part,whole
	size_t word_len; //该条目的长度

	BlacklistEntry() {}
	BlacklistEntry(
			int32_t init_query_type,
			int32_t init_match_type,
			size_t init_word_len
			)
	{
		query_type = init_query_type;
		match_type = init_match_type;
		word_len = init_word_len;
	}
};

// 每个命中的类别
struct HitEntry_ {
	int32_t query_type;  // 白名单还是黑名单white,black 可以按照自己的需求定义
	std::string query;	// 命中词

	HitEntry_() {}
	HitEntry_(
		int32_t _query_type, 
		std::string &_query) 
	{
		query_type = _query_type;
		query = _query;
	}
};


// 每个命中的类别
struct HitEntry {
	bool hitTitleBlack;
	bool hitQueryBlack;
	bool hitPeopleSet;
	bool hitBlackSet;
	std::vector<std::string> CombineMatchVec;	// a+b 精准组合命中的query

	HitEntry() {
		hitTitleBlack = false;
		hitQueryBlack = false;
		hitPeopleSet = false;
		hitBlackSet = false;
	}

	void clear() {
		hitTitleBlack = false;
		hitQueryBlack = false;
		hitPeopleSet = false;
		hitBlackSet = false;
		CombineMatchVec.clear();
	}
};





//辅助结构，用于建立索引使用
struct BlacklistEntryTmp
{
	std::string query;
	int32_t query_type;//
	int32_t match_type;//匹配方式:prefix,postfix,part,whole
	size_t word_len;

	bool operator < (const BlacklistEntryTmp& other) const
	{
		return query.compare(other.query) < 0;
	}

	BlacklistEntryTmp() {}
	BlacklistEntryTmp(
			const std::string& init_query,
			int32_t init_query_type,
			int32_t init_match_type,
			size_t init_word_len)
	{
		query = init_query;
		query_type = init_query_type;
		match_type = init_match_type;
		word_len = init_word_len;
	}
};


//黑名单 
class BlacklistTable 
{
public:
	BlacklistTable();
	~BlacklistTable();

	// 释放资源
	int32_t Release();

	// 对原始数据建立索引
	// 建立索引之前必须首先对EncodingConverter初始化
	// EncodingConverter::initializeInstance()
	int32_t MakeIndex(const char * file, 
			const char * index_file,
			const std::string source="all");

	// 读入索引
	int32_t LoadIndex(const char *index_file);

	// 清除索引
	int32_t ClearIndex();

	//判断是否命中白名单或者黑名单
	//return value:  kHitTypeBlack 命中黑名单
	//				 kHitTypeWhite 命中白名单
	//				 kHitTypeNull  hit nothing
	int32_t BlackWhiteHit(
			const std::string& query);

	void BlackWhiteMultiHit(
        const std::string& query,
		HitEntry &hintEntry);

	//得到对应的黑名单条目
//	BlacklistEntry* GetBlacklistEntry(
//			int32_t idx);

	//查找是否命中黑名单
	int32_t HitBlacklist(
			const std::string& query,
			int32_t query_type
			);

	// 在双数组上游走
	int32_t Traverse(
			const char *key, 
			size_t &node_pos, 
			size_t &key_pos, 
			size_t length = 0);

private:

	// 输出index文件
	int32_t OutputIndexFile(
			std::vector<BlacklistEntryTmp> &
				blacklist_vect, 
				const char * index_file
				);

	int32_t GetMatchType(
		std::string& match_type_str
		);
	int32_t GetQueryType(
			std::string& query_type_str
			);
	
	bool HitWholeQuery(
			const std::string& query,
			int32_t query_type
			);
	bool HitPrefixQuery(
			const std::string& query,
			int32_t query_type
			);
	bool HitPostfixQuery(
			const std::string& query,
			int32_t query_type
			);
	bool HitPartQuery(
			const std::string& query,
			int32_t query_type
			);
	int32_t CommonPrefixSearch(
			const std::string& query,
			std::vector<int>& hit_result,
			int32_t max_hit_num);

    Darts::DoubleArray m_da_blacklist; 

	int32_t m_num_blacklist_entry;//黑名单条目数
	BlacklistEntry* m_blacklist_entry_buf;

	static const int kMaxStrLen = 1024;
	static const int kMaxBlackQueryLen = 128;
	static const int kMaxHitNum = 1024;
	
}; //BlacklistTable

#endif
