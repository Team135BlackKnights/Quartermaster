#ifndef AUTH_H
#define AUTH_H

#include<string>

struct Auth{
	std::string host,user,pass,db;
};

Auth auth();

#endif
