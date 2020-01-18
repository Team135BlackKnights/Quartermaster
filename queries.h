#ifndef QUERIES_H
#define QUERIES_H

#include<memory>
#include "data_types.h"

#define SUBSYSTEM_DATA(X)\
	X(Valid,valid)\
	X(std::string,name)\
	X(Subsystem_prefix,prefix)\
	X(std::optional<Subsystem_id>,parent)\
	X(Part_number,part_number)\
	X(Hours,time)\
	X(Assembly_state,state)\
	X(std::optional<DNI>,dni)\
	X(Priority,priority)\
	X(std::optional<Weight>,weight_override)
	
struct Subsystem_data{
	SUBSYSTEM_DATA(INST)
};

#define SUBSYSTEM_ROW(X)\
	X(Subsystem_id,id)\

#define SUBSYSTEM_INFO_ROW(X)\
	X(Id,id)\
	X(Subsystem_id,subsystem_id)\
	X(User,edit_user)\
	X(Datetime,edit_date)\
	SUBSYSTEM_DATA(X)\

#define PART_ROW(X)\
	X(Part_id,id)\

#define PART_DATA_INNER(X)\
	X(Subsystem_id,subsystem)\
	X(std::string,name)\
	X(Part_number,part_number)\
	X(Part_state,part_state)\
	X(std::optional<std::string>,length)\
	X(std::optional<std::string>,width)\
	X(std::optional<std::string>,thickness)\
	X(std::optional<Material>,material)\
	X(unsigned,qty)\
	X(std::optional<Hours>,time)\
	X(std::optional<Date>,manufacture_date)\
	X(std::optional<User>,who_manufacture)\
	X(std::optional<Machine>,machine)\
	X(std::optional<std::string>,place)\
	X(std::optional<std::string>,bent)\
	X(std::optional<Bend_type>,bend_type)\
	X(std::optional<URL>,drawing_link)\
	X(std::optional<URL>,cam_link)\
	X(std::optional<Supplier>,part_supplier)\
	X(std::optional<URL>,part_link)\
	X(std::optional<Date>,arrival_date)\
	X(std::optional<Decimal>,price)\
	X(std::optional<DNI>,dni)\
	X(std::optional<Weight>,weight)\
	X(std::optional<Bom_exemption>,bom_exemption)\
	X(std::optional<Decimal>,bom_cost_override)\
	
#define PART_DATA(X)\
	X(Valid,valid)\
	PART_DATA_INNER(X)\

struct Part_data{
	PART_DATA(INST)
};

std::ostream& operator<<(std::ostream&,Part_data const&);

#define PART_INFO_ROW(X)\
	X(Id,id)\
	X(Part_id,part_id)\
	X(User,edit_user)\
	X(Datetime,edit_date)\
	PART_DATA(X)\

#define MEETING_ROW(X)\
	X(Meeting_id,id)

using Meeting_length=int;
using Color=std::string;

#define MEETING_DATA(X)\
	X(Valid,valid)\
	X(Date,date)\
	X(Meeting_length,length)\
	X(Color,color)\
	X(std::string,notes)\

struct Meeting_data{
	MEETING_DATA(INST)
};

#define MEETING_INFO_ROW(X)\
	X(Id,id)\
	X(Meeting_id,meeting_id)\
	X(User,edit_user)\
	X(Datetime,edit_date)\
	MEETING_DATA(X)

using Column_type=std::pair<std::string,bool>; //type and primary key
using Table_type=std::vector<std::pair<std::string,Column_type>>;

Table_type read(DB db,std::string const& name);

void check_table(DB,Table_name const&,Table_type const&);
std::set<Table_name> show_tables(DB);

std::string to_db_type(const bool*);
std::string to_db_type(const unsigned*);
std::string to_db_type(const std::string*);
std::string to_db_type(const double*);

void create_table(DB,Table_name const&,Table_type const&);

void check_database(DB);

using P=std::map<std::string,std::vector<std::string>>;

std::string to_query(std::map<std::string,std::string> const&);

unsigned rand(const unsigned*);

struct Page{
	std::optional<std::string> sort_by;
	std::optional<std::string> sort_order;

	virtual ~Page()=0;
};

#define STR(X) ""#X

#define INST_EQ(A,B) if(a.B!=b.B) return 0;
#define SHOW(A,B) { o<<""#B<<":"<<a.B<<" "; }
#define DECL_LIST(A,B) A B,

#define DECL_OPTION(T,ITEMS)\
	struct T:Page{ \
		ITEMS(INST) \
	};\
	bool operator==(T const& a,T const& b);\
	bool operator!=(T const& a,T const& b);\
	std::ostream& operator<<(std::ostream& o,T const& a);\
	T rand(const T*);\
	std::string to_query(T const& a);\
	std::optional<T> parse_query(const T*,P const&);\
	void diff(T const& a,T const& b);\

#define HOME_ITEMS(X) \

#define T Home
DECL_OPTION(Home,HOME_ITEMS)
#undef T

#define SUBSYSTEMS_ITEMS(X) \

#define T Subsystems
DECL_OPTION(Subsystems,SUBSYSTEMS_ITEMS)
#undef T

#define SUBSYSTEM_NEW_ITEMS(X)\
	X(std::optional<Subsystem_id>,parent)
DECL_OPTION(Subsystem_new,SUBSYSTEM_NEW_ITEMS)

#define SUBSYSTEM_EDITOR_ITEMS(X)\
	X(Subsystem_id,id)\

DECL_OPTION(Subsystem_editor,SUBSYSTEM_EDITOR_ITEMS)

#define SUBSYSTEM_EDIT_DATA_ITEMS(X)\
	X(Subsystem_id,subsystem_id)\
	SUBSYSTEM_DATA(X)\

#define SUBSYSTEM_EDIT_ITEMS(X)\
	SUBSYSTEM_EDIT_DATA_ITEMS(X)\
	X(std::optional<URL>,after)\

DECL_OPTION(Subsystem_edit,SUBSYSTEM_EDIT_ITEMS)

#define PARTS_ITEMS(X)
DECL_OPTION(Parts,PARTS_ITEMS)

#define PART_NEW_ITEMS(X)\
	X(std::optional<Subsystem_id>,subsystem)
DECL_OPTION(Part_new,PART_NEW_ITEMS)

#define PART_EDITOR_ITEMS(X)\
	X(Part_id,id)\

DECL_OPTION(Part_editor,PART_EDITOR_ITEMS)

#define PART_EDIT_DATA_ITEMS(X)\
	X(Part_id,part_id)\
	PART_DATA(X)\

#define PART_EDIT_ITEMS(X)\
	PART_EDIT_DATA_ITEMS(X)\
	X(std::optional<URL>,after)\

DECL_OPTION(Part_edit,PART_EDIT_ITEMS)

#define CALENDAR_ITEMS(X)
DECL_OPTION(Calendar,CALENDAR_ITEMS)

#define MEETING_NEW_ITEMS(X)
DECL_OPTION(Meeting_new,MEETING_NEW_ITEMS)

#define MEETING_EDITOR_ITEMS(X)\
	X(Meeting_id,id)
DECL_OPTION(Meeting_editor,MEETING_EDITOR_ITEMS)

#define MEETING_EDIT_DATA_ITEMS(X)\
	X(Meeting_id,meeting_id)\
	MEETING_DATA(X)\
	
#define MEETING_EDIT_ITEMS(X)\
	MEETING_EDIT_DATA_ITEMS(X)\
	X(std::optional<URL>,after)\

DECL_OPTION(Meeting_edit,MEETING_EDIT_ITEMS)

#define ERROR_ITEMS(X)\
	X(std::string,s)
DECL_OPTION(Error,ERROR_ITEMS)

#define BY_USER_ITEMS(X) \
	X(User,user)
DECL_OPTION(By_user,BY_USER_ITEMS)

#define MACHINES_ITEMS(X)
DECL_OPTION(Machines,MACHINES_ITEMS)

#define ORDERS_ITEMS(X)
DECL_OPTION(Orders,ORDERS_ITEMS)

#define MACHINE_ITEMS(X)\
	X(Machine,machine)
DECL_OPTION(Machine_page,MACHINE_ITEMS)

#define STATE_ITEMS(X)\
	X(Part_state,state)
DECL_OPTION(State,STATE_ITEMS)

#define CSV_EXPORT_ITEMS(X)\
	X(Export_item,export_item)
DECL_OPTION(CSV_export,CSV_EXPORT_ITEMS)

#define EXTRA_ITEMS(X)
DECL_OPTION(Extra,EXTRA_ITEMS)

#define ORDER_EDIT_ITEMS(X)\
	X(Date,arrival_date)\
	X(std::vector<Part_id>,part_checkbox)
DECL_OPTION(Order_edit,ORDER_EDIT_ITEMS)

#define ARRIVED_ITEMS(X)\
	X(Part_id,part)
DECL_OPTION(Arrived,ARRIVED_ITEMS)

#define BY_SUPPLIER_ITEMS(X)\
	X(Supplier,supplier)
DECL_OPTION(By_supplier,BY_SUPPLIER_ITEMS)

#define BOM_ITEMS(X)\
	X(std::optional<Subsystem_id>,subsystem)
DECL_OPTION(BOM,BOM_ITEMS)

#define PART_DUPLICATE_ITEMS(X)\
	X(Part_id,part)
DECL_OPTION(Part_duplicate,PART_DUPLICATE_ITEMS)

#define SUBSYSTEM_DUPLICATE_ITEMS(X)\
	X(Subsystem_id,subsystem)
DECL_OPTION(Subsystem_duplicate,SUBSYSTEM_DUPLICATE_ITEMS)

#define STATE_CHANGE_ITEMS(X)\
	X(std::optional<Date>,start)\
	X(std::optional<Date>,end)
DECL_OPTION(State_change,STATE_CHANGE_ITEMS)

#define CHART_ITEMS(X)\
	X(std::set<Part_state>,ignore)
DECL_OPTION(Chart,CHART_ITEMS)
DECL_OPTION(Chart_image,CHART_ITEMS)

#define BATCH_ENTRY_ITEMS(X)
DECL_OPTION(Batch_entry,BATCH_ENTRY_ITEMS)

#define ENTRY_ITEMS(X)\
	X(Subsystem_id,subsystem)\
	X(std::string,name)\
	X(unsigned,qty)
struct Entry{
	ENTRY_ITEMS(INST)
};
std::ostream& operator<<(std::ostream&,Entry const&);

#define BATCH_ENTRY_BACKEND_ITEMS(X)\
	X(Subsystem_id,subsystem0)\
	X(std::string,name0)\
	X(unsigned,qty0)\
	X(Subsystem_id,subsystem1)\
	X(std::string,name1)\
	X(unsigned,qty1)\
	X(Subsystem_id,subsystem2)\
	X(std::string,name2)\
	X(unsigned,qty2)\
	X(Subsystem_id,subsystem3)\
	X(std::string,name3)\
	X(unsigned,qty3)\
	X(Subsystem_id,subsystem4)\
	X(std::string,name4)\
	X(unsigned,qty4)\

DECL_OPTION(Batch_entry_backend,BATCH_ENTRY_BACKEND_ITEMS)

#define BASIC_PAGES(X)\
	X(Home)\
	X(Subsystems)\
	X(Parts)\
	X(Batch_entry)\
	X(Calendar)\
	X(Machines)\
	X(Orders)\
	X(Extra)\
	
#define PAGES(X)\
	BASIC_PAGES(X)\
	X(Subsystem_new)\
	X(Part_new)\
	X(Meeting_new)\
	X(Subsystem_editor)\
	X(Subsystem_edit)\
	X(Part_editor)\
	X(Part_edit)\
	X(Meeting_editor)\
	X(Meeting_edit)\
	X(By_user)\
	X(Machine_page)\
	X(State)\
	X(CSV_export)\
	X(Order_edit)\
	X(Arrived)\
	X(By_supplier)\
	X(BOM)\
	X(Part_duplicate)\
	X(Subsystem_duplicate)\
	X(State_change)\
	X(Chart)\
	X(Chart_image)\
	X(Batch_entry_backend)
	
using Request=std::variant<
	#define X(A) A,
	PAGES(X)
	#undef X
	Error
>;

Request parse_query(std::string const&);

template<
	#define X(A) typename A,
	PAGES(X)
	#undef X
	typename Z
>
std::string to_query(std::variant<
	#define X(A) A,
	PAGES(X)
	#undef X
	Z
> const& a){
	#define X(A) if(std::holds_alternative<A>(a)) return to_query(std::get<A>(a));
	PAGES(X)
	X(Z)
	#undef X
	nyi
}

std::string link(Request const&,std::string const&);
std::map<Table_name,Table_type> expected_tables();

template<typename A,typename B>
std::vector<std::pair<std::optional<A>,std::optional<B>>> zip_extend(std::vector<A> const& a,std::vector<B> const& b){
	std::vector<std::pair<std::optional<A>,std::optional<B>>> r;
	for(auto i:range(std::max(a.size(),b.size()))){
		auto a1=[=]()->std::optional<A>{ if(i<a.size()) return a[i]; return std::nullopt; }();
		auto b1=[=]()->std::optional<B>{ if(i<b.size()) return b[i]; return std::nullopt; }();
		r|=std::make_pair(a1,b1);
	}
	return r;
}

template<typename T>
void diff(std::vector<T> const& a,std::vector<T> const& b){
	if(a.size()!=b.size()){
		auto z=zip_extend(a,b);
		for(auto p:z){
			auto [a1,b1]=p;
			if(a1!=b1){
				std::cout<<"a:"<<a1<<"\n";
				std::cout<<"b:"<<b1<<"\n";
			}
		}
		return;
	}

	for(auto [a1,b1]:zip(a,b)){
		if(a1!=b1){
			std::cout<<"a:"<<a1<<"\n";
			std::cout<<"b:"<<b1<<"\n";
		}
	}
}

#endif
