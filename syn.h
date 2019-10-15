
#ifndef GUARD_SYN_H_
#define GUARD_SYN_H_
#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <vector>
#include <list>
#include<algorithm>
#include <assert.h>
#include<stdlib.h>
#include<map>
#include "netlist.h"
#include "syn.h"

//--------------------------------struct--------------------------------
struct evl_token {
    enum token_type {NAME, NUMBER, SINGLE};
    token_type type;
    std::string str;
    int line_no;
}; // struct evl_token
typedef std::list<evl_token> evl_tokens;

struct evl_statement {
    enum statement_type {MODULE, WIRE, COMPONENT, ENDMODULE};
    statement_type type;
    evl_tokens tokens;
}; // struct evl_statement
typedef std::list<evl_statement> evl_statements;

struct evl_wire
{
    std::string name;
    int width;
}; // struct evl_wire
typedef std::vector<evl_wire> evl_wires;

struct evl_pin{
	std::string name;
	size_t msb;
	size_t lsb;
};
typedef std::vector<evl_pin> evl_pins;

struct evl_component{
	std::string name;
	std::string type;
	evl_pins pins;
};
typedef std::vector<evl_component> evl_components;


#endif /* SYN_H_ */
