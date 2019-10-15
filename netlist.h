#ifndef GUARD_NETLIST_H_
#define GUARD_NETLIST_H_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include<sstream>
#include<algorithm>
#include <assert.h>
#include<stdlib.h>
#include <exception>
#include<map>
#include "syn.h"


class netlist;
class gate;
class net;
class pin;
typedef std::map<std::string, int> evl_wires_table;
typedef std::map<std::string, net*> evl_nets_table;
//typedef std::map<std::string, std::string> look_up_table;
class net{
    std::string name_; // e.g. "a", "s[0]"
    char signal_; // e.g. ’0’ or ’1’
    std::list<pin *> connections_; // relationship "connect"
    std::string wireName;
public:
    net(std::string name);
    void setName(std::string n);
    std::string getName();
    void set_signal(char s);
    void append_pin (pin *p);
    std::list<pin *> getConnections();
    char get_signal();
}; // class net

class pin {
    char dir_; // e.g. ’I’ or ’O’
    gate *gate_; // relationship "contain"
    size_t index_; // attribute of "contain"
    std::vector<net *> nets_;
public:
    char get_dir();
    std::string make_net_name(std::string wire_name, int i);
    std::string showGateName();
    std::string showGateType();
    size_t getIndex();
    std::string showNetsName();
    std::vector<net *> getNets();
    size_t getNetsSize();
    bool create(gate *g, size_t index,  const evl_pin &p,
            const std::map<std::string, net *> &nets_table);
    bool createBus(gate *g, size_t index,  const evl_pin &p,
                const std::map<std::string, net *> &nets_table, size_t busNum);
    bool createBusses(gate *g, size_t index,  const evl_pin &p,
                    const std::map<std::string, net *> &nets_table, size_t msb, size_t lsb);
    char compute_signal();
    void set_as_output();
    void set_as_input();
    std::string bin_to_hex();
    void hex_to_bin(std::string hex);
}; // class pin

class gate {
private:
    std::string name_;
    std::string type_; // e.g. "and", "or"
protected:
    std::vector<pin *> pins_; // relationship "contain"
public:
    std::string getName();
    std::string getType();
    std::vector<pin *> getPins();
    bool create(const evl_component &c, const std::map<std::string, net *> &nets_table,
            const evl_wires_table &wires_table);
    bool create_pin(const evl_pin &ep, size_t index,const std::map<std::string, net *> &nets_table,
            const evl_wires_table &wires_table);
    virtual bool validate_structural_semantics()=0;
    virtual void compute_next_state_or_output()=0;
    virtual char compute_signal(int pin_index)=0;
    virtual void fileBehaviour(std::string fileName)=0;
    virtual void end_Iteration_Behaviour()=0;
    virtual void before_Iteration_Behaviour()=0;
}; // class gate

class flip_flop: public gate {
    char state_, next_state_;

public:
    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName){} ;
    void setState(char s);
    void end_Iteration_Behaviour();
    void before_Iteration_Behaviour(){}

};

class and_gate : public gate {
public:
    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName){} ;
    void end_Iteration_Behaviour(){}
    void before_Iteration_Behaviour(){}

};

class or_gate: public gate {
public:
    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName){} ;
    void end_Iteration_Behaviour(){}
    void before_Iteration_Behaviour(){}

};

class xor_gate: public gate {
    char state_, next_state_;
public:
    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName){}
    void end_Iteration_Behaviour(){}
    void before_Iteration_Behaviour(){}

};

class not_gate: public gate {
    char state_, next_state_;
public:
    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName){}
    void end_Iteration_Behaviour(){}
    void before_Iteration_Behaviour(){}

};

class buf_gate: public gate {
    char state_, next_state_;
public:
    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName){}
    void end_Iteration_Behaviour(){}
    void before_Iteration_Behaviour(){}

};

class tris_gate: public gate {
    char state_, next_state_;
public:
    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName){}
    void end_Iteration_Behaviour(){}
    void before_Iteration_Behaviour(){}

};

class evl_clock: public gate {
    char state_, next_state_;
public:
    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName){}
    void end_Iteration_Behaviour(){}
    void before_Iteration_Behaviour(){}

};

class evl_one: public gate {
    char state_, next_state_;
public:
    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName){}
    void end_Iteration_Behaviour(){}
    void before_Iteration_Behaviour(){}

};

class evl_zero: public gate {
    char state_, next_state_;
public:
    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName){}
    void end_Iteration_Behaviour(){}
    void before_Iteration_Behaviour(){}

};

class evl_input: public gate {
    int line = 0;
    std::ifstream in;
    std::string current_line;
    int transitions = 0;    //number of cycle already done
public:

    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName);
    void end_Iteration_Behaviour(){};
    void before_Iteration_Behaviour();

};

class evl_output: public gate {
    char state_, next_state_;
    std::ofstream out;
public:
    bool validate_structural_semantics();
    void compute_next_state_or_output();
    char compute_signal(int pin_index);
    void fileBehaviour(std::string fileName);
    void end_Iteration_Behaviour(){}
    void before_Iteration_Behaviour(){}


};
class evl_lut: public gate {
    std::ifstream in;
    std::map<int, std::string> look_up_table;
public:
    bool validate_structural_semantics();
    void compute_next_state_or_output(){};
    char compute_signal(int pin_index){assert(false);};
    void fileBehaviour(std::string fileName);
    void end_Iteration_Behaviour(){};
    void before_Iteration_Behaviour();

};

class netlist {
private:
	std::map<std::string, net *> nets_table_;
    std::list<gate *> gates_;
    std::list<net *> nets_;
public:
    bool create(const evl_wires &wires,const evl_components &comps,const evl_wires_table &wires_table);
    bool create_gates(const evl_components &comps, const evl_wires_table &wires_table);
    bool create_nets(const evl_wires &wires);
    void create_net(std::string net_name);
    bool create_gate(const evl_component &c, const evl_wires_table &wires_table);
    std::string make_net_name(std::string wire_name, int i);
    void save(std::ostream &out, std::string module_name);
    void compute_next_state_and_output();
    void simulate(size_t n);
    void load_file(std::string fileName);

}; // class netlist



#endif
