# !/bin/bash

g++ blacklist_table.cpp test_blacklist_table.cpp -o black_test -lpthread -lencoding 
g++ blacklist_table.cpp blacklist_table_make_index.cpp -o make_index -lpthread -lencoding 
