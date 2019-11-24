#ifndef QUERIES_H
#define QUERIES_H

#include<memory>
#include "data_types.h"

#define SUBSYSTEM_DATA(X)\
	X(bool,valid)\
	X(std::string,name)\
	X(Subsystem_prefix,prefix)\
	X(std::optional<Subsystem_id>,parent)\

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
	SUBSYSTEM_DATA(X)

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
	X(std::optional<Decimal>,time)\
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
	X(std::optional<Decimal>,price)

#define PART_DATA(X)\
	X(bool,valid)\
	PART_DATA_INNER(X)

struct Part_data{
	PART_DATA(INST)
};

#define PART_INFO_ROW(X)\
	X(Id,id)\
	X(Part_id,part_id)\
	X(User,edit_user)\
	X(Datetime,edit_date)\
	PART_DATA(X)

#define MEETING_ROW(X)\
	X(Meeting_id,id)

using Meeting_length=int;
using Color=std::string;

#define MEETING_DATA(X)\
	X(bool,valid)\
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

using Table_name=std::string;

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

#define SUBSYSTEM_EDIT_ITEMS(X)\
	X(Subsystem_id,subsystem_id)\
	SUBSYSTEM_DATA(X)

DECL_OPTION(Subsystem_edit,SUBSYSTEM_EDIT_ITEMS)

#define PARTS_ITEMS(X)
DECL_OPTION(Parts,PARTS_ITEMS)

#define PART_NEW_ITEMS(X)\
	X(std::optional<Subsystem_id>,subsystem)
DECL_OPTION(Part_new,PART_NEW_ITEMS)

#define PART_EDITOR_ITEMS(X)\
	X(Part_id,id)\

DECL_OPTION(Part_editor,PART_EDITOR_ITEMS)
	
#define PART_EDIT_ITEMS(X)\
	X(Part_id,part_id)\
	PART_DATA(X)\

DECL_OPTION(Part_edit,PART_EDIT_ITEMS)

#define CALENDAR_ITEMS(X)
DECL_OPTION(Calendar,CALENDAR_ITEMS)

#define MEETING_NEW_ITEMS(X)
DECL_OPTION(Meeting_new,MEETING_NEW_ITEMS)

#define MEETING_EDITOR_ITEMS(X)\
	X(Meeting_id,id)
DECL_OPTION(Meeting_editor,MEETING_EDITOR_ITEMS)

#define MEETING_EDIT_ITEMS(X)\
	X(Meeting_id,meeting_id)\
	MEETING_DATA(X)
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

#define BASIC_PAGES(X)\
	X(Home)\
	X(Subsystems)\
	X(Subsystem_new)\
	X(Parts)\
	X(Part_new)\
	X(Calendar)\
	X(Meeting_new)\
	X(Machines)\
	X(Orders)\
	X(Extra)
	
#define PAGES(X)\
	BASIC_PAGES(X)\
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

#endif
