#include "util.h"

/*#include<set>
#include<sstream>
#include<map>
#include<variant>
#include<mariadb/mysql.h>
#include<cassert>*/

std::vector<size_t> range(size_t lim){
	std::vector<size_t> r;
	for(size_t i=0;i<lim;i++) r|=i;
	return r;
}

std::string join(std::string const& a,std::vector<std::string> const& b){
	std::stringstream ss;
	for(auto [last,elem]:mark_last(b)){
		ss<<elem;
		if(!last) ss<<a;
	}
	return ss.str();
}

bool prefix(std::string const& needle,std::string const& haystack){
	if(needle.size()>haystack.size()) return 0;
	for(auto i:range(needle.size())){
		if(needle[i]!=haystack[i]){
			return 0;
		}
	}
	return 1;
}

std::vector<std::string> split(char delim,std::string const& s){
	std::vector<std::string> r;
	std::stringstream ss;
	for(auto c:s){
		if(c==delim){
			r|=ss.str();
			ss.str("");
		}else{
			ss<<c;
		}
	}
	if(ss.str().size()){
		r|=ss.str();
	}
	return r;
}

float rand(const float*){
	return float(rand())/99;
}

double rand(const double*){
	return double(rand())/99;
}

bool rand(const bool*){ return rand()%2; }

std::string rand(const std::string*){ return (rand()%2)?"cow":"chicken"; }

bool operator==(std::vector<std::string> const& a,std::vector<const char*> const& b){
	if(a.size()!=b.size()) return 0;
	for(auto [a1,b1]:zip(a,b)){
		if(a1!=b1) return 0;
	}
	return 1;
}

bool operator!=(std::vector<std::string> const& a,std::vector<const char*> const& b){
	return !(a==b);
}

std::string tag(std::string const& name,std::string const& body){
	std::stringstream ss;
	ss<<"<"<<name<<">"<<body<<"</"<<name<<">";
	return ss.str();
}

std::string h1(std::string const& s){ return tag("h1",s); }

#define TAG(X) std::string X(std::string const& s){ return tag(""#X,s); }
TAG(title)
TAG(head)
TAG(html)
TAG(body)
TAG(p)
TAG(td)
TAG(th)
TAG(h2)
TAG(tr)
#undef TAG

using DB=MYSQL*;

void run_cmd(DB db,std::string const& cmd){
	auto q=mysql_query(db,cmd.c_str());
	if(q){
		std::cout<<"Fail:"<<mysql_error(db)<<"\n";
		mysql_close(db);
		exit(1);
	}
}

std::vector<std::vector<std::optional<std::string>>> query(DB db,std::string const& query){
	//this is obviously not the fastest way to do this.
	run_cmd(db,query);
	MYSQL_RES *result=mysql_store_result(db);
	if(result==NULL)nyi
	int fields=mysql_num_fields(result);
	//PRINT(fields);
	MYSQL_ROW row;
	std::vector<std::vector<std::optional<std::string>>> r;
	while((row=mysql_fetch_row(result))){
		std::vector<std::optional<std::string>> this_row;
		for(auto i:range(fields)){
			if(row[i]){
				//cout<<i<<": \""<<row[i]<<"\"\n";
				//cout<<"after\n";
				this_row|=std::optional{std::string(row[i])};
			}else{
				this_row|={};
				//cout<<"NULL!\n";
			}
		}
		r|=this_row;
	}
	mysql_free_result(result);
	return r;
}

std::vector<std::string> operator|=(std::vector<std::string> &a,const char *s){
    assert(s);
	a.push_back(s);
	return a;
}
