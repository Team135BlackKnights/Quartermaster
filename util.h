#ifndef UTIL_H
#define UTIL_H

#include<set>
#include<sstream>
#include<map>
#include<variant>
#include<mysql/mysql.h>
#include<cassert>
#include<vector>
#include<iostream>

#define nyi { std::cout<<"NYI "<<__LINE__<<"\n"; exit(44); }
#define PRINT(X) { std::cout<<""#X<<":"<<(X)<<"\n"; }
#define INST(A,B) A B;

template<typename T>
std::set<T> to_set(std::vector<T> a){
	return std::set<T>{a.begin(),a.end()};
}

template<typename T>
std::ostream& operator<<(std::ostream& o,std::set<T> const& a){
	o<<"{ ";
	for(auto elem:a){
		o<<elem<<" ";
	}
	return o<<"}";
}

template<typename T>
std::vector<T>& operator|=(std::vector<T>& a,T t){
	a.push_back(t);
	return a;
}

template<typename T>
std::string as_string(T t){
	std::stringstream ss;
	ss<<t;
	return ss.str();
}

template<typename A,typename B>
std::ostream& operator<<(std::ostream& o,std::pair<A,B> const& a){
	return o<<"("<<a.first<<","<<a.second<<")";
}

template<typename K,typename V>
std::ostream& operator<<(std::ostream& o,std::map<K,V> const& a){
	o<<"{ ";
	for(auto p:a) o<<p<<" ";
	return o<<"}";
}

template<typename T>
std::ostream& operator<<(std::ostream& o,std::vector<T> const& a){
	o<<"[ ";
	for(auto elem:a) o<<elem<<" ";
	return o<<"]";
}

template<typename K,typename V>
std::map<K,V> without_key(std::map<K,V> a,K k){
	auto f=a.find(k);
	if(f==a.end()) return a;
	a.erase(f);
	return a;
}

template<typename A,typename B>
std::vector<std::pair<A,B>> zip(std::vector<A> const& a,std::vector<B> const& b){
	std::vector<std::pair<A,B>> r;
	auto a_at=begin(a);
	auto b_at=begin(b);
	auto a_end=end(a);
	auto b_end=end(b);
	while(a_at!=a_end && b_at!=b_end){
		r|=make_pair(*a_at,*b_at);
		a_at++;
		b_at++;
	}
	return r;
}

template<typename T>
std::ostream& operator<<(std::ostream& o,std::optional<T> const& a){
	if(a) return o<<*a;
	return o<<"NULL";
}

template<typename A,typename B>
std::vector<A> firsts(std::vector<std::pair<A,B>> a){
	return mapf([](auto x){ return x.first; },a);
}

template<typename A,typename B>
std::vector<B> seconds(std::vector<std::pair<A,B>> a){
	return mapf([](auto x){ return x.second; },a);
}

std::vector<size_t> range(size_t lim);

template<typename T>
std::vector<std::pair<bool,T>> mark_last(std::vector<T> a){
	std::vector<std::pair<bool,T>> r;
	for(auto i:range(a.size())){
		r|=make_pair(i==a.size()-1,a[i]);
	}
	return r;
}

template<typename T>
T choose(std::vector<T> a){
	assert(a.size());
	return a[rand()%a.size()];
}

template<typename Func,typename T>
auto mapf(Func f,std::vector<T> v){
	std::vector<decltype(f(v[0]))> r;
	for(auto elem:v){
		r|=f(elem);
	}
	return r;
}

template<typename T,typename ... Ts>
bool operator==(std::variant<Ts...> const& a,T const& t){
	return a==std::variant<Ts...>(t);
}

template<typename ...Ts>
std::ostream& operator<<(std::ostream& o,std::variant<Ts...> const& a){
	std::visit([&](auto && x){ o<<x; },a);
	return o;
}

template<typename T>
std::set<T>& operator|=(std::set<T>& a,T t){
	a.insert(t);
	return a;
}

template<typename K,typename V>
std::set<K> keys(std::map<K,V> const& a){
	std::set<K> r;
	for(auto [k,v]:a){
		(void)v;
		r|=k;
	}
	return r;
}

template<typename T>
std::set<T> operator-(std::set<T> a,std::set<T> b){
	for(auto elem:b){
		a.erase(elem);
	}
	return a;
}

template<typename K,typename V>
void diff(std::map<K,V> a,std::map<K,V> b){
	auto ka=keys(a);
	auto kb=keys(b);
	if(ka!=kb){
		PRINT(ka-kb);
		PRINT(kb-ka);
		nyi
	}
	for(auto k:ka){
		auto va=a[k];
		auto vb=b[k];
		if(va!=vb){
			PRINT(k);
			PRINT(va);
			PRINT(vb);
		}
	}
}

template<typename K,typename V>
std::vector< std::pair<bool,std::pair<const K,V>> > mark_last(std::map<K,V> const& a){
	std::vector< std::pair<bool,std::pair<const K,V>> > r;
	for(auto at=begin(a);at!=end(a);++at){
		auto n=at;
		n++;
		r|=std::make_pair(n==end(a),*at);
	}
	return r;
}

template<typename T>
std::vector<T> firsts(std::vector<std::vector<T>> a){
	return mapf([](auto x){ return x.at(0); },a);
}

template<typename T>
std::vector<T> operator+(std::vector<T> a,std::vector<T> b){
	std::vector<T> r=a;
	for(auto elem:b) r|=elem;
	return r;
}

template<typename ... Ts>
void diff(std::variant<Ts...> const& a,std::variant<Ts...> const& b){
	bool found=std::visit(
		[&](auto x){
			using T=decltype(x);
			if(std::holds_alternative<T>(b)){
				diff(x,std::get<T>(b));
				return 1;
			}
			return 0;
		},
		a
	);
	if(found) return;
	nyi
}

std::string join(std::string a,std::vector<std::string> b);
bool prefix(std::string needle,std::string haystack);
std::vector<std::string> split(char delim,std::string s);

float rand(const float*);
double rand(const double*);
bool rand(const bool*);
std::string rand(const std::string*);

bool operator==(std::vector<std::string> const& a,std::vector<const char*> const& b);
bool operator!=(std::vector<std::string> const& a,std::vector<const char*> const& b);

std::string tag(std::string name,std::string body);
std::string h1(std::string s);

#define X(A) std::string A(std::string s);
X(title)
X(head)
X(html)
X(body)
X(p)
X(td)
X(th)
X(h2)
#undef X

using DB=MYSQL*;

void run_cmd(DB db,std::string cmd);
std::vector<std::vector<std::optional<std::string>>> query(DB db,std::string query);

#endif
