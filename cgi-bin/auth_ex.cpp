#include "auth.h"

//This is an example of what "auth.cpp" should look like
//All of the pieces should be replaced with the login info for 
//your mysql server, and then this file should be copied to
//"auth.cpp"

Auth auth(){
	return Auth{
		"<hostname of server>",
		"<username>",
		"<password>",
		"<database name>"
	};
}
