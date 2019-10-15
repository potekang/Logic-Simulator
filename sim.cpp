#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include<algorithm>
#include <assert.h>
#include<stdlib.h>
#include<map>
#include "netlist.h"
#include "syn.h"

typedef std::vector<evl_wire> evl_wires;
typedef std::vector<evl_pin> evl_pins;
typedef std::vector<evl_component> evl_components;
typedef std::map<std::string, int> evl_wires_table;
typedef std::map<std::string, std::string> look_up_table;
//function
bool extract_tokens_from_line(std::string line, int line_no,
        evl_tokens &tokens) { // use reference to modify it
    for (size_t i = 0; i < line.size(); ++i) {
        // comments
        if (line[i] == '/') {
            ++i;
            if ((i == line.size()) || (line[i] != '/')) {
                std::cerr << "LINE " << line_no << ": a single / is not allowed"
                        << std::endl;
                return -1;
            }
            break; // skip the rest of the line by exiting the loop
        }
        // spaces
        if ((line[i] == ' ') || (line[i] == '\t') || (line[i] == '\r')
                || (line[i] == '\n')) {
            continue; // skip the rest of the iteration
        }

        // SINGLE
        if ((line[i] == '(') || (line[i] == ')') || (line[i] == '[')
                || (line[i] == ']') || (line[i] == ':') || (line[i] == ';')
                || (line[i] == ',')) {
            evl_token token;
            token.line_no = line_no;
            token.type = evl_token::SINGLE;
            token.str = line[i];
            tokens.push_back(token);
            continue; // skip the rest of the iteration
        }

        // NAME
        if (((line[i] >= 'a') && (line[i] <= 'z'))       // a to z
        || ((line[i] >= 'A') && (line[i] <= 'Z'))    // A to Z
                || (line[i] == '_')) {
            size_t name_begin = i;
            for (++i; i < line.size(); ++i) {
                if (!(((line[i] >= 'a') && (line[i] <= 'z'))
                        || ((line[i] >= 'A') && (line[i] <= 'Z'))
                        || ((line[i] >= '0') && (line[i] <= '9'))
                        || (line[i] == '_') || (line[i] == '$'))) {
                    break; // [name_begin, i) is the range for the token
                }
            }
            evl_token token;
            token.line_no = line_no;
            token.type = evl_token::NAME;
            token.str = line.substr(name_begin, i - name_begin);
            tokens.push_back(token);
            i -= 1;
            continue;
        }

        // Number
        if ((line[i] >= '0') && (line[i] <= '9')) {
            size_t number_begin = i;
            for (++i; i < line.size(); ++i) {
                if (!((line[i] >= '0') && (line[i] <= '9'))) {
                    break;
                }
            }
            evl_token token;
            token.line_no = line_no;
            token.type = evl_token::NUMBER;
            token.str = line.substr(number_begin, i - number_begin);
            tokens.push_back(token);
            --i;
        } else {
            std::cerr << "LINE" << line_no << ": invalid character"
                    << std::endl;
            return false;
        }
    }
    return true; // nothing left
}

bool extract_tokens_from_file(std::string file_name, evl_tokens &tokens) { // use reference to modify it
    std::ifstream input_file(file_name);
    if (!input_file) {
        std::cerr << "I can’t read " << file_name << "." << std::endl;
        return false;
    }
    tokens.clear(); // be defensive, make it empty
    std::string line;
    for (int line_no = 1; std::getline(input_file, line); ++line_no) {
        if (!extract_tokens_from_line(line, line_no, tokens)) {
            return false;
        }
    }
    return true;
}

void display_tokens(std::ostream &out, evl_tokens &tokens) {
    for (evl_tokens::iterator i = tokens.begin(); i != tokens.end(); ++i) {
        if ((*i).type == evl_token::SINGLE) {
            out << "SINGLE " << (*i).str << std::endl;
        } else if ((*i).type == evl_token::NAME) {
            out << "NAME " << (*i).str << std::endl;
        } else { // must be NUMBER
            out << "NUMBER " << (*i).str << std::endl;
        }
    }
}

bool token_is_semicolon(const evl_token &token) {
    return token.str == ";";
}

bool move_tokens_to_statement(evl_tokens &statement_tokens,
        evl_tokens &tokens) {
    // What if someone passes the two parameters in the wrong order?
    assert(statement_tokens.empty());
    // search for ";"
    evl_tokens::iterator next_sc = std::find_if(tokens.begin(), tokens.end(),
            token_is_semicolon);
    if (next_sc == tokens.end()) {
        std::cerr << "Look for ’;’ but reach the end of file" << std::endl;
        return false;
    }
    // move tokens within [tokens.begin(), next_sc] to statement_tokens
    evl_tokens::iterator after_sc = next_sc;
    ++after_sc;
    statement_tokens.splice(statement_tokens.begin(), tokens, tokens.begin(),
            after_sc);
    return true;
}

bool process_wire_statement(evl_wires &wires, evl_statement &s) {
    enum state_type {
        INIT,
        WIRE,
        DONE,
        WIRES,
        WIRE_NAME,
        BUS,
        BUS_MSB,
        BUS_COLON,
        BUS_LSB,
        BUS_DONE
    };
    state_type state = INIT;
    size_t busWidth = 1;
    for (; !s.tokens.empty() && (state != DONE); s.tokens.pop_front()) {
        evl_token t = s.tokens.front();
        if (state == INIT) {
            if (t.str == "wire") {
                state = WIRE;
            } else {
                std::cerr << "Need ’wire’ but found ’" << t.str << "’ on line "
                        << t.line_no << std::endl;
                return false;
            }
        } else if (state == WIRE) {
            if (t.type == evl_token::NAME) {
                evl_wire wire;
                wire.name = t.str;
                wire.width = busWidth;
                wires.push_back(wire);
                state = WIRE_NAME;
            } else if (t.str == "[") {
                state = BUS;
            } else {
                std::cerr << "Need NAME but found ’" << t.str << "’ on line "
                        << t.line_no << std::endl;
                return false;
            }
        } else if (state == WIRES) {
            if (t.type == evl_token::NAME) {
                evl_wire wire;
                wire.name = t.str;
                wire.width = busWidth;
                wires.push_back(wire);
                state = WIRE_NAME;
            } else {
                std::cerr << "Need NAME but found ’" << t.str << "’ on line "
                        << t.line_no << std::endl;
                return false;
            }
        } else if (state == WIRE_NAME) {
            if (t.str == ",") {
                state = WIRES;
            } else if (t.str == ";") {
                state = DONE;
            } else {
                std::cerr << "Need ’,’ or ’;’ but found ’" << t.str
                        << "’ on line " << t.line_no << std::endl;
                return false;
            }

        } else if (state == BUS) {
            if (t.type == evl_token::NUMBER) {
                busWidth = atoi(t.str.c_str()) + 1;
                // std::cout<<t.str<<endl;
                state = BUS_MSB;
            } else {
                std::cerr << "Need NUMBER but found '" << t.str << "' on line"
                        << t.line_no;
                return false;
            }
        } else if (state == BUS_MSB) {
            if (t.str == ":") {
                state = BUS_COLON;
            } else {
                std::cerr << "Need ':' but found '" << t.str << "' on line"
                        << t.line_no;
                return false;
            }
        } else if (state == BUS_COLON) {
            if (t.str == "0") {
                state = BUS_LSB;
            } else {
                std::cerr << "Need '0' but found '" << t.str << "' on line"
                        << t.line_no;
                return false;
            }
        } else if (state == BUS_LSB) {
            if (t.str == "]") {
                state = BUS_DONE;
            } else {
                std::cerr << "Need ']' but found '" << t.str << "' on line"
                        << t.line_no;
                return false;
            }
        } else if (state == BUS_DONE) {
            if (t.type == evl_token::NAME) {
                evl_wire wire;
                wire.name = t.str;
                wire.width = busWidth;
                wires.push_back(wire);
                state = WIRE_NAME;
            } else {
                std::cerr << "Need NAME but found '" << t.str << "' on line"
                        << t.line_no;
                return false;
            }
        }
        // use branches here to compute state transitions
    }
    if (!s.tokens.empty() || (state != DONE)) {
        std::cerr << "something wrong with the statement" << std::endl;
        return false;
    }
    return true;
}

bool process_component_statement(evl_components &comps, evl_statement &s) {

    enum state_type {
        INIT,
        TYPE,
        NAME,
        PINS,
        PIN_NAME,
        BUS,
        BUS_MSB,
        BUS_COLON,
        BUS_LSB,
        BUS_DONE,
        PINS_DONE,
        DONE
    };
    state_type state = INIT;
    evl_component comp;
    evl_pin pin;
    for (; !s.tokens.empty() && state != DONE; s.tokens.pop_front()) {
        evl_token t = s.tokens.front();
        //FSM
        if (state == INIT) {
            //std::cout<<t.str<<"sdf"<<std::endl;
            if (t.type == evl_token::NAME) {
                state = TYPE;
                comp.type = t.str;
                comp.name = "";
            } else {
                std::cerr << "Need NAME but found ’" << t.str << "’ on line "
                        << t.line_no << std::endl;
                return false;
            }
        } else if (state == TYPE) {
            if (t.type == evl_token::NAME) {
                state = NAME;
                comp.name = t.str;
            } else if (t.str == "(") {
                state = PINS;
            } else {

                std::cerr << "NEED NAME or '(' but found '" << "' on line "
                        << t.line_no << std::endl;
                return false;
            }
        } else if (state == NAME) {
            //std::cout<<t.str<<"name"<<std::endl;
            if (t.str == "(") {
                state = PINS;
            } else {
                std::cerr << "NEED '(' but found '" << "' on line " << t.line_no
                        << std::endl;
                return false;
            }
        } else if (state == PINS) {
            if (t.type == evl_token::NAME) {
                state = PIN_NAME;
                pin.name = t.str;
                pin.msb = -1;
                pin.lsb = -1;
            } else {
                std::cerr << "Need NAME but found ’" << t.str << "’ on line "
                        << t.line_no << std::endl;
                return false;
            }
        } else if (state == PIN_NAME) {
            if (t.str == "[") {
                state = BUS;
            } else if (t.str == ",") {
                state = PINS;
                comp.pins.push_back(pin);
            } else if (t.str == ")") {
                state = PINS_DONE;
                comp.pins.push_back(pin);
            } else {
                std::cerr << "Need '[', ',' or ')' but found ’" << t.str
                        << "’ on line " << t.line_no << std::endl;
                return false;
            }

        } else if (state == PINS_DONE) {
            if (t.str == ";") {
                state = DONE;
                //initialize everything
                comps.push_back(comp);
                comp.name = "";
                comp.pins.clear();
            } else {
                std::cerr << "Need ';' but found ’" << t.str << "’ on line "
                        << t.line_no << std::endl;
                return false;
            }
        } else if (state == BUS) {
            if (t.type == evl_token::NUMBER) {
                state = BUS_MSB;
                pin.msb = atoi(t.str.c_str());
            } else {
                std::cerr << "Need NUMBER but found ’" << t.str << "’ on line "
                        << t.line_no << std::endl;
                return false;
            }

        } else if (state == BUS_MSB) {
            if (t.str == ":") {
                state = BUS_COLON;
            } else if (t.str == "]") {
                state = BUS_DONE;
            } else {
                std::cerr << "Need ':' or ']' but found ’" << t.str
                        << "’ on line " << t.line_no << std::endl;
                return false;
            }
        } else if (state == BUS_COLON) {
            if (t.type == evl_token::NUMBER) {
                state = BUS_LSB;
                pin.lsb = atoi(t.str.c_str());
            } else {
                std::cerr << "Need NUMBER but found ’" << t.str << "’ on line "
                        << t.line_no << std::endl;
                return false;
            }
        } else if (state == BUS_LSB) {
            if (t.str == "]") {
                state = BUS_DONE;
            } else {
                std::cerr << "Need ']' but found ’" << t.str << "’ on line "
                        << t.line_no << std::endl;
                return false;
            }
        } else if (state == BUS_DONE) {
            if (t.str == ",") {
                state = PINS;
                comp.pins.push_back(pin);
            } else if (t.str == ")") {
                state = PINS_DONE;
                comp.pins.push_back(pin);
            } else {
                std::cerr << "Need ',' or ')' but found ’" << t.str
                        << "’ on line " << t.line_no << std::endl;
                return false;
            }
        } else if (!s.tokens.empty() || (state != DONE)) {
            std::cerr << "something wrong with the statement" << std::endl;
            return false;
        }
    }
    return true;
}

bool group_tokens_into_statements(evl_statements &statements,
        evl_tokens &tokens, evl_wires &wires, evl_components &components) {
    for (; !tokens.empty();) { // generate one statement per iteration
        evl_token token = tokens.front();
        if (token.type != evl_token::NAME) {
            std::cerr << "Need a NAME token but found ’" << token.str
                    << "’ on line " << token.line_no << std::endl;
            return false;
        }
        if (token.str == "module") {         // MODULE statement
            evl_statement module;
            module.type = evl_statement::MODULE;
            if (!move_tokens_to_statement(module.tokens, tokens))
                return false;
            statements.push_back(module);
        } else if (token.str == "endmodule") { // ENDMODULE statement
            evl_statement endmodule;
            endmodule.type = evl_statement::ENDMODULE;
            endmodule.tokens.push_back(token);
            tokens.pop_front();
            statements.push_back(endmodule);
        } else if (token.str == "wire") { //WIRE statement
            evl_statement wire;
            wire.type = evl_statement::WIRE;
            if (!move_tokens_to_statement(wire.tokens, tokens))
                return false;
            statements.push_back(wire);
            process_wire_statement(wires, wire);
        } else {	//COMPONENT statement
            evl_statement component;
            component.type = evl_statement::COMPONENT;
            if (!move_tokens_to_statement(component.tokens, tokens))
                return false;
            statements.push_back(component);
            process_component_statement(components, component);
        }
    }
    return true;
}

void display_statements(std::ostream &out, evl_statements &statements,
        evl_wires &wires, evl_components &components) {
    //statements go first
    evl_statements::iterator i = statements.begin();
    if ((*i).type == evl_statement::MODULE) {
        evl_tokens::iterator v = (*i).tokens.begin();
        ++v;
        out << "module " << (*v).str << std::endl;
    }
    //dispaly wires
    out << "wires " << wires.size() << std::endl;
    for (size_t t = 0; t != wires.size(); ++t) {
        out << " wire " << wires[t].name << " " << wires[t].width << std::endl;
    }
    //display components
    out << "components " << components.size() << std::endl;
    for (size_t t = 0; t != components.size(); ++t) {
        if (components[t].name == "") {
            out << " component " << components[t].type << " "
                    << components[t].pins.size() << std::endl;
        } else {
            out << " component " << components[t].type << " "
                    << components[t].name << " " << components[t].pins.size()
                    << std::endl;
        }
        for (size_t i = 0; i != components[t].pins.size(); i++) {
            evl_pin p = components[t].pins[i];
            if (p.msb == -1) {
                out << "  pin " << p.name << std::endl;
            } else if ((p.msb != -1) && (p.lsb == -1)) {
                out << "  pin " << p.name << " " << p.msb << std::endl;
            } else {
                out << "  pin " << p.name << " " << p.msb << " " << p.lsb
                        << std::endl;
            }
        }
    }

}

 bool make_wires_table(const evl_wires &wires,
 evl_wires_table &wires_table) {
     for (auto &wire: wires) {
         auto same_name = wires_table.find(wire.name);
         if (same_name != wires_table.end()) {
             std::cerr << "Wire ’" << wire.name
                    << "’is already defined" << std::endl;
            return false;
         }
        // std::cout<<wire.name<<std::endl;
         wires_table[wire.name] = wire.width;
         }
         return true;
 }


//Main
int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "You should provide a file name." << std::endl;
        return -1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file) {
        std::cerr << "I can't read " << argv[1] << "." << std::endl;
        return -1;
    }

    std::string output_file_name = std::string(argv[1]) + ".tokens";
    std::ofstream output_file(output_file_name);
    if (!output_file) {
        std::cerr << "I can't write " << argv[1] << ".tokens ." << std::endl;
        return -1;
    }
    std::string evl_file = argv[1]; //input file
    evl_tokens tokens;
    if (!output_file) {
        std::cerr << "I can't write " << argv[1] << ".tokens ." << std::endl;
        return -1;
    }
    if (!extract_tokens_from_file(evl_file, tokens)) {
        return -1;
    }
    display_tokens(output_file, tokens);
    // -------------------------------lex analysis--------------------------------
    // --------------------------------syn analysis--------------------------------
    evl_statements statements;
    evl_wires wires;
    evl_components components;					//group tooken into statements
    if (!group_tokens_into_statements(statements, tokens, wires, components)) {
        return -1;
    }

    std::string output_file_name_syn = std::string(argv[1]) + ".syntax";
    std::ofstream output_file_syn(output_file_name_syn);
    display_statements(output_file_syn, statements, wires, components);
    // --------------------------------syn analysis--------------------------------
    // --------------------------------NET&SIM analysis--------------------------------


     evl_wires_table wires_table;
     if (!make_wires_table(wires, wires_table))
         return -1;
     netlist nl;
     if (!nl.create(wires, components, wires_table))
         return -1;
     std::string nl_file_name = std::string(argv[1])+".netlist";
     std::ofstream nl_file(nl_file_name);
     statements.begin();
     //get module name
     evl_statements::iterator i = statements.begin();
     evl_tokens::iterator v = (*i).tokens.begin();
     ++v;
     std::string module_name = (*v).str;
     //-------------------
     nl.save(nl_file, module_name);  // save the netlist for Project 3
     nl.load_file(std::string(argv[1]));
     nl.simulate(1000); // simulate 1000 cycles for Project 4

    return 0;
}

