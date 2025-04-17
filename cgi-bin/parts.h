#ifndef PARTS_H
#define PARTS_H

#include "queries.h"

std::optional<Request> parse_referer(const char*);
void inner(std::ostream&,New_user const&,DB);
struct DB_connection {
	DB db;

	DB_connection(DB db_) : db(db_) {}
	~DB_connection() {
		mysql_close(db);
	}

	std::vector<std::map<std::string, std::string>> query(const std::string& q) {
		std::vector<std::map<std::string, std::string>> results;

		if (mysql_query(db, q.c_str()) != 0) return results;

		MYSQL_RES* res = mysql_store_result(db);
		if (!res) return results;

		int num_fields = mysql_num_fields(res);
		MYSQL_ROW row;
		MYSQL_FIELD* fields = mysql_fetch_fields(res);

		while ((row = mysql_fetch_row(res))) {
			std::map<std::string, std::string> row_map;
			for (int i = 0; i < num_fields; ++i) {
				if (row[i]) {
					row_map[fields[i].name] = row[i];
				}
			}
			results.push_back(row_map);
		}
		mysql_free_result(res);
		return results;
	}
};
#endif
