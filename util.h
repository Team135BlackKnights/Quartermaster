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
#include<memory>
#include<optional>

#define nyi { std::cout<<"NYI "<<__FILE__<<":"<<__LINE__<<"\n"; exit(44); }
#define PRINT(X) { std::cout<<""#X<<":"<<(X)<<"\n"; }
#define INST(A,B) A B;

template<typename T>
std::set<T> to_set(std::vector<T> const& a){
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
	a.push_back(std::move(t));
	return a;
}

std::vector<std::string> operator|=(std::vector<std::string>&,const char *);

template<typename T>
std::string as_string(T const& t){
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
std::map<K,V> without_key(std::map<K,V> a,K const& k){
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
		r|=std::make_pair(*a_at,*b_at);
		a_at++;
		b_at++;
	}
	return r;
}

template<typename A,typename B,typename C>
std::vector<std::tuple<A,B,C>> zip(std::vector<A> const& a,std::vector<B> const& b,std::vector<C> const& c){
	std::vector<std::tuple<A,B,C>> r;
	auto a_at=begin(a);
	auto b_at=begin(b);
	auto c_at=begin(c);
	auto a_end=end(a);
	auto b_end=end(b);
	auto c_end=end(c);
	while(a_at!=a_end && b_at!=b_end && c_at!=c_end){
		r|=std::make_tuple(*a_at,*b_at,*c_at);
		a_at++;
		b_at++;
		c_at++;
	}
	return r;
}

template<typename T>
std::ostream& operator<<(std::ostream& o,std::optional<T> const& a){
	if(a) return o<<*a;
	return o<<"NULL";
}

template<typename A,typename B>
std::vector<A> firsts(std::vector<std::pair<A,B>> const& a){
	return mapf([](auto x){ return x.first; },a);
}

template<typename A,typename B>
std::vector<B> seconds(std::vector<std::pair<A,B>> const& a){
	return mapf([](auto x){ return x.second; },a);
}

std::vector<size_t> range(size_t lim);

template<typename T>
std::vector<std::pair<bool,T>> mark_last(std::vector<T> a){
	std::vector<std::pair<bool,T>> r;
	for(auto i:range(a.size())){
		r|=std::make_pair(i==a.size()-1,std::move(a[i]));
	}
	return r;
}

template<typename T>
T const& choose(std::vector<T> const& a){
	assert(a.size());
	return a[rand()%a.size()];
}

template<typename Func,typename T>
auto mapf(Func f,T const& v){
	std::vector<decltype(f(*std::begin(v)))> r;
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
	a.insert(std::move(t));
	return a;
}

template<typename K,typename V>
std::set<K> keys(std::map<K,V> const& a){
	std::set<K> r;
	for(auto& [k,v]:a){
		(void)v;
		r|=k;
	}
	return r;
}

template<typename T>
std::set<T> operator-(std::set<T> a,std::set<T> const& b){
	for(auto elem:b){
		a.erase(elem);
	}
	return a;
}

template<typename K,typename V>
void diff(std::map<K,V> const& a,std::map<K,V> const& b){
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
std::vector<T> firsts(std::vector<std::vector<T>> const& a){
	return mapf([](auto x){ return x.at(0); },a);
}

template<typename T>
std::vector<T> operator+(std::vector<T> a,std::vector<T> const& b){
	std::vector<T> r=std::move(a);
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
	std::cout<<"variant types differ:\n";
	PRINT(a);
	PRINT(b);
}

std::string join(std::string const&,std::vector<std::string> const&);
bool prefix(std::string const& needle,std::string const& haystack);
std::vector<std::string> split(char delim,std::string const&);

float rand(const float*);
double rand(const double*);
bool rand(const bool*);
std::string rand(const std::string*);

bool operator==(std::vector<std::string> const& a,std::vector<const char*> const& b);
bool operator!=(std::vector<std::string> const& a,std::vector<const char*> const& b);

std::string tag(std::string const& name,std::string const& body);
std::string h1(std::string const&);

#define TAGS(X)\
X(title)\
X(head)\
X(html)\
X(body)\
X(p)\
X(td)\
X(th)\
X(h2)\
X(h3)\
X(h4)\
X(tr)\
X(li)
#define X(A) std::string A(std::string const&);
TAGS(X)
#undef X

using DB=MYSQL*;

void run_cmd(DB db,std::string const& cmd);
std::vector<std::vector<std::optional<std::string>>> query(DB db,std::string const& query);

template<typename Func,typename T>
std::vector<T> filter(Func f,std::vector<T> const& v){
	std::vector<T> r;
	for(auto const& elem:v){
		if(f(elem)) r|=elem;
	}
	return r;
}

template<typename T>
std::vector<std::pair<size_t,T>> enumerate(std::vector<T> const& a){
	std::vector<std::pair<size_t,T>> r;
	for(size_t i=0;i<a.size();i++){
		r|=std::make_pair(i,a[i]);
	}
	return r;
}

template<typename T>
std::optional<T>& operator+=(std::optional<T>& a,std::optional<T> const& b){
	if(a){
		if(b){
			a=*a+*b;
		}
	}else{
		if(b) a=b;
	}
	return a;
}

template<typename T>
T sum(std::vector<T> const& v){
	T r{};
	for(auto elem:v){
		r+=elem;
	}
	return r;
}

template<typename ... Ts>
std::ostream& operator<<(std::ostream& o,std::tuple<Ts...> const& a){
	o<<"( ";
	std::apply([&](auto&&... x){ ((o<<x<<" "), ...); },a);
	return o<<")";
}

template<typename T>
void print_lines(T const& t){
	for(auto elem:t){
		std::cout<<elem<<"\n";
	}
}

template<typename T>
std::vector<T>& operator|=(std::vector<T>& a,std::vector<T> const& b){
	for(auto elem:b){
		a|=elem;
	}
	return a;
}

template<typename T>
std::string join(std::string delim,std::vector<T> v){
	std::stringstream ss;
	for(auto [last,elem]:mark_last(v)){
		ss<<elem;
		if(!last) ss<<delim;
	}
	return ss.str();
}

#define MAP(F,A) mapf([&](auto x){ return (F)(x); },(A))

template<typename K,typename V>
std::vector<V> values(std::map<K,V> const& a){
	std::vector<V> r;
	for(auto [_,v]:a){
		(void)_;
		r|=v;
	}
	return r;
}

template<typename T>
std::set<T> operator|(std::set<T> a,std::set<T> const& b){
	for(auto elem:b){
		a|=elem;
	}
	return a;
}

template<typename T>
std::vector<T> tail(std::vector<T> a){
	if(a.size()){
		a.erase(begin(a));
	}
	return a;
}

template<typename T>
bool all(std::vector<T> const& a){
	for(auto elem:a){
		if(!elem){
			return 0;
		}
	}
	return 1;
}

template<typename T>
std::vector<T> non_null(std::vector<std::optional<T>> v){
	std::vector<T> r;
	for(auto elem:v){
		if(elem){
			r|=*elem;
		}
	}
	return r;
}

template<typename T>
std::vector<T> sorted(std::vector<T> a){
	sort(begin(a),end(a));
	return a;
}

template<typename T>
std::vector<T>& operator|=(std::vector<T> &a,std::optional<T> b){
	if(b){
		a|=*b;
	}
	return a;
}

template<typename T>
std::optional<T> max(std::vector<T> const& a){
	if(a.empty()) return std::nullopt;
	T r=a[0];
	for(auto elem:a){
		r=std::max(r,elem);
	}
	return r;
}

template<typename T>
std::vector<T> flatten(std::vector<std::vector<T>> const& a){
	std::vector<T> r;
	for(auto elem:a){
		r|=elem;
	}
	return r;
}

template<typename T>
std::string join(std::vector<T> const& v){
	return join("",v);
}

template<typename A,typename B>
std::map<A,B> to_map(std::vector<std::tuple<A,B>> const& v){
	std::map<A,B> r;
	for(auto [a,b]:v){
		r[a]=b;
	}
	return r;
}

template<typename A,typename ... Ts>
std::map<A,std::tuple<A,Ts...>> to_map(std::vector<std::tuple<A,Ts...>> const& v){
	std::map<A,std::tuple<A,Ts...>> r;
	for(auto elem:v){
		r[std::get<0>(elem)]=elem;
	}
	return r;
}

template<typename T>
std::vector<T> to_vec(std::set<T> a){
	return std::vector<T>(begin(a),end(a));
}

template<typename T>
std::vector<T> to_vec(std::vector<T> a){
	return a;
}

using Table_name=std::string;

void insert(DB,Table_name const&,std::vector<std::pair<std::string,std::string>> const&);
void indent(size_t);

#endif
