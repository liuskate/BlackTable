/*************************************************
 *
 * Create time: 2012 Feb 14 07:02:38 PM
 * version 1.1
 *
*************************************************/
/* Last Modified 2016 3月 30 16时20分17秒 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

#include "blacklist_table.h"
#include "gary_string.h"
#include "Platform/encoding.h"
#include "Platform/encoding/support.h"
#include "Platform/encoding/EncodingConvertor.h"

using namespace std;
using namespace spaceGary;

string text_path;
string index_path;


void printHitResult(string &query, HitEntry &hintEntry) {
	cout << "query : " << query << endl;
	cout << "\thit title black: " << hintEntry.hitTitleBlack << endl;
	cout << "\thit query black: " << hintEntry.hitQueryBlack << endl;
	cout << "\thit people set: " << hintEntry.hitPeopleSet << endl;
	cout << "\thit black set: " << hintEntry.hitBlackSet << endl;
	cout << "\thit a+b black: " << (hintEntry.CombineMatchVec.size() > 0) << endl;
	
	for (int idx=0; idx<hintEntry.CombineMatchVec.size(); ++idx) {
		cout << "\t a+b query is: " << hintEntry.CombineMatchVec[idx] << endl;
	}


}

void test_blacklist(string &input, string &output)
{
	BlacklistTable blacklist_table;
	blacklist_table.MakeIndex(
				input.c_str(),
				output.c_str(),
				"sohu_wap"
				);
	blacklist_table.LoadIndex(output.c_str());
    HitEntry hintEntry;
	
	string query = "习近平近期与江泽民访问美国";
	blacklist_table.BlackWhiteMultiHit(query, hintEntry);
	printHitResult(query, hintEntry);

	hintEntry.clear();
	query = "胡锦涛与江泽民访问美国";
	blacklist_table.BlackWhiteMultiHit(query, hintEntry);
	printHitResult(query, hintEntry);

	hintEntry.clear();
	query = "意大利机长带着空姐去开飞机";
	blacklist_table.BlackWhiteMultiHit(query, hintEntry);
	printHitResult(query, hintEntry);
	

	hintEntry.clear();
	query = "重庆市委书记薄熙来和王立军翻脸";
	blacklist_table.BlackWhiteMultiHit(query, hintEntry);
	printHitResult(query, hintEntry);

}

int main(int argc, char ** argv)
{
	EncodingConvertor::initializeInstance();

	string input = "map_search_blacklist.new";
	string output = "map_search_blacklist.index";

	//test_makeindex(input, output);
	test_blacklist(input, output);


	return 0;
}
