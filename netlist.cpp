#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include<algorithm>
#include <assert.h>
#include<stdlib.h>
#include<map>
#include <exception>
#include "netlist.h"
#include "syn.h"
#include <math.h>
typedef std::map<std::string, int> evl_wires_table;
//typedef std::map<std::string, std::string> look_up_table;

//__________________ net member function_______________________
net::net(std::string name){
    name_=name;
    signal_='0';
}

void net::append_pin (pin *p) {
    connections_.push_back(p);
}

void net::setName(std::string n){
    name_=n;
}

std::string net::getName(){
    return name_;
}

std::list<pin *> net::getConnections(){
    return connections_;
}

void net::set_signal(char s){
    signal_=s;
}

char net::get_signal(){
    bool loop =true;
    bool outGate=false;
    if (signal_ == '?') {
        auto it = connections_.begin();
        for(;loop;){
            auto filter = [](pin *p){ return p->get_dir() == 'O'; };
            it = std::find_if(it,connections_.end(), filter);
            if (it == connections_.end()&&outGate==false)
            {
                throw std::runtime_error("floating net");
                break;
            }
            if(it == connections_.end()){
                break;
            }
            pin *driver = *it;
            outGate=true;
            signal_ = driver->compute_signal();
            //std::cout<<"signal: "<<signal_<<std::endl;
            if(signal_=='0'||signal_=='1'){
                loop=false;
                break;
            }
            it++;
    }
    }
        return signal_;
}

//__________________ pin member function_______________________
char pin::get_dir(){
    return dir_;
}

std::string pin::make_net_name(std::string wire_name, int i) {
    assert(i >= 0);
    std::ostringstream oss;
    oss << wire_name << "[" << i << "]";
    return oss.str();
}

bool pin::create(gate *g, size_t index,  const evl_pin &p,
        const std::map<std::string, net *> &nets_table) {
   // store g and index;
    gate_=g;
    index_=index;
    if (p.msb == -1) { // a 1-bit wire
       //net_->setName(p.name);
       std::map<std::string, net *>::const_iterator result = nets_table.find(p.name);
       if (result != nets_table.end()) {
           nets_.push_back( result->second);
       } else {
           // error: unable to find in map
           std::cerr << "error in pin::create"<< std::endl;
           return false;
       }

    }nets_[0]->append_pin(this);
    //dir_of a pin!!!!!!!!!!!!!!!!!!
    return true;

}

bool pin::createBus(gate *g, size_t index,  const evl_pin &p,

                const std::map<std::string, net *> &nets_table, size_t busNum){
    gate_=g;
    index_=index;
    std::string name = make_net_name(p.name, busNum);
    std::map<std::string, net *>::const_iterator result = nets_table.find(name);
    if (result != nets_table.end()) {
           nets_.push_back(result->second);
           nets_[0]->append_pin(this);
           return true;
    } else {
         // error: unable to find in map
         std::cerr << "Error in createBus"<< std::endl;
         return false;
    }
}

bool pin::createBusses(gate *g, size_t index,  const evl_pin &p,
                    const std::map<std::string, net *> &nets_table, size_t msb, size_t lsb){
    gate_=g;
    index_=index;
    int count = 0;
    if(lsb==0){
        for(size_t i=lsb; i<=msb; ++i){
            std::string name = make_net_name(p.name, i);
            std::map<std::string, net *>::const_iterator result = nets_table.find(name);
            if (result != nets_table.end()) {
                nets_.push_back(result->second);
                nets_[i]->append_pin(this);
            }else{
                std::cerr << "Error in createBusses"<< std::endl;
                return false;
        }
    }
    }else{
        for(size_t i=lsb; i<=msb; ++i){
                std::string name = make_net_name(p.name, i);
                std::map<std::string, net *>::const_iterator result = nets_table.find(name);

                if (result != nets_table.end()) {
                    nets_.push_back(result->second);
                    nets_[count]->append_pin(this);
                    count++;
                }else{
                    std::cerr << "Error in createBusses"<< std::endl;
                    return false;
                }

    }
    return true;
}
}

std::string pin::showGateName(){
    return (*gate_).getName();
}

std::string pin::showGateType(){
    return (*gate_).getType();
}

size_t pin::getIndex(){
    return index_;
}

std::vector<net *> pin::getNets(){
    return nets_;
}

size_t pin::getNetsSize(){
    return nets_.size();
}

std::string pin::showNetsName(){
    std::string name;
    for(int i=0;i<nets_.size();++i){
        name += nets_[i]->getName()+" ";
    }
    return name;
}

char pin::compute_signal() {
    if (dir_ == 'O')
        return gate_->compute_signal(index_);
    else // dir_ == ’I’
        //signal in nets are different!!!!!!!!!!!!!!!!!!!!!!!
        for(net *s:nets_){
            //std::cout<<"compute_signal called"<<std::endl;
            s->get_signal();
        }
    return nets_[0]->get_signal();
}

void pin::set_as_output(){
        dir_='O';
}

void pin::set_as_input(){
        dir_='I';
}

std::string pin::bin_to_hex(){
    std::string bin;
    std::string hex;
    //make up 0
    //std::cout<<"size "<<nets_.size()<<std::endl;

    if(nets_.size()%4==1){
        bin.append("000");
    }else if(nets_.size()%4==2){
       bin.append("00");
    }else if(nets_.size()%4==3){
        bin.push_back('0');
    }
   // std::cout<<"bin(before):"<<bin<<std::endl;
    //load the number
    for(int t=nets_.size()-1; t>=0;t--){
        bin.push_back(nets_[t]->get_signal());
    }
    //std::cout<<"bin(after):"<<bin<<std::endl;
    //make the hex
    for(int t=0;t<bin.size();t+=4){

        std::string sub = bin.substr(t,4);
        if(sub=="0000"){
            hex.push_back('0');
        }else if(sub=="0001"){
            hex.push_back('1');
        }else if(sub=="0010"){
            hex.push_back('2');
        }else if(sub=="0011"){
            hex.push_back('3');
        }else if(sub=="0100"){
            hex.push_back('4');
        }else if(sub=="0101"){
            hex.push_back('5');
        }else if(sub=="0110"){
            hex.push_back('6');
        }else if(sub=="0111"){
            hex.push_back('7');
        }else if(sub=="1000"){
            hex.push_back('8');
        }else if(sub=="1001"){
            hex.push_back('9');
        }else if(sub=="1010"){
            hex.push_back('A');
        }else if(sub=="1011"){
            hex.push_back('B');
        }else if(sub=="1100"){
            hex.push_back('C');
        }else if(sub=="1101"){
            hex.push_back('D');
        }else if(sub=="1110"){
            hex.push_back('E');
        }else if(sub=="1111"){
            hex.push_back('F');
        }else{
            std::cerr<<"something wrong with bin_to_hex, no such signal:"<<sub<<std::endl;
        }
    }
    //std::cout<<"hex: "<<hex<<std::endl;
    return hex;
}

void pin::hex_to_bin(std::string hex){
   //std::cout<<"hex_to_bin called with hex: "<<hex<<std::endl;
    if(nets_.size()!=1){
        std::string full_bin="";
        std::string bin="";
        char h;
        for(size_t t= 0;t<hex.length();t++){
            h = hex[t];
            if(h=='0'){
                bin.append("0000");
            }else if(h=='1'){
                bin.append("0001");
            }else if(h=='2'){
                bin.append("0010");
            }else if(h=='3'){
                bin.append("0011");
            }else if(h=='4'){
                bin.append("0100");
            }else if(h=='5'){
                bin.append("0101");
            }else if(h=='6'){
                bin.append("0110");
            }else if(h=='7'){
                bin.append("0111");
            }else if(h=='8'){
                bin.append("1000");
            }else if(h=='9'){
                bin.append("1001");
            }else if(h=='A'|| h=='a'){
                bin.append("1010");
            }else if(h=='B'|| h=='b'){
                bin.append("1011");
            }else if(h=='C'|| h=='c'){
                bin.append("1100");
            }else if(h=='D'|| h=='d'){
                bin.append("1101");
            }else if(h=='E'|| h=='e'){
                bin.append("1110");
            }else if(h=='F'|| h=='f'){
                bin.append("1111");
            }
        }
       // std::cout<<"hex value "<<hex<<std::endl;
        if(bin.length()!=this->getNets().size()){
            if(bin.length()>this->getNets().size()){
                std::cout<<"bin length:  "<<bin.length()<<std::endl;
                std::cout<<"nets length:  "<<getNets().size()<<std::endl;
                std::cerr<<"length of the hex shouldn't longer than pin's "<<std::endl;
            }else{
                int n = getNets().size()-bin.length();
                for(int i=0;i<n;i++){
                full_bin.push_back('0');
                }
            }
        }
        full_bin.append(bin);
        int length = getNets().size();
        for(int i =0;i<length;i++){
            this->getNets()[length-i-1]->set_signal(full_bin[i]);
    }
    }else{
        this->getNets()[0]->set_signal(hex[0]);
    }
}
//__________________ gate member function_______________________

bool gate::create(const evl_component &c, const std::map<std::string, net *> &nets_table,
        const evl_wires_table &wires_table) {
    name_=c.name;
    //std::cout<<c.type<<std::endl;
    type_=c.type;
    size_t index = 0;
        for (int i=0;i!=c.pins.size();++i) {
            create_pin(c.pins[i], index, nets_table, wires_table);
           // std::cout<<c.pins[i].name<<std::endl;std::cout<<"lsb:"<<c.pins[i].lsb<<std::endl;std::cout<<"msb:"<<c.pins[i].msb<<std::endl;
                ++index;
        }
       return validate_structural_semantics();
       // return true;
}

bool gate::create_pin(const evl_pin &ep, size_t index, const std::map<std::string, net *> &nets_table,
        const evl_wires_table &wires_table) {
    if(ep.lsb==-1&&ep.msb==-1){
       // std::cout<<ep.name<<std::endl; std::cout<<"lsb: "<<ep.lsb<<std::endl; std::cout<<"msb: "<<ep.msb<<std::endl;
        auto result = wires_table.find(ep.name);
        if(result!=wires_table.end()){
           //std::cout<<result->second<<std::endl;
            if(result->second>1){
                pin *p = new pin;
                pins_.push_back(p);
                return p->createBusses(this, index, ep, nets_table, result->second-1,0);
            }else{
                pin *p = new pin;
                pins_.push_back(p);
                return p->create(this, index, ep, nets_table);
            }

        }else{
                std::cerr<<"sth wrong in creat_pin"<<std::endl;
            }
    }else{
        //std::cout<<"pin!!!!"<<std::endl;
            if(ep.lsb==-1){
                pin *p = new pin;
                pins_.push_back(p);
                return p->createBus(this, index, ep, nets_table,ep.msb);
            }else{
                pin *p = new pin;
                pins_.push_back(p);
                return p->createBusses(this, index, ep, nets_table,ep.msb,ep.lsb);
        }
        return true;
    }
}

std::string gate::getName(){
    return name_;
}

std::string gate::getType(){
    return type_;
}

std::vector<pin *> gate::getPins(){
    return pins_;
}

void flip_flop::setState(char s){
    state_ =s;
}

void flip_flop::end_Iteration_Behaviour(){
    state_=next_state_;
}

void evl_output::fileBehaviour(std::string fileName) {
    std::ofstream output_file(fileName);
    out.swap(output_file);
    //std::cout<<"fileBehaviour in evl_output called"<<std::endl;
    out<<this->getPins().size()<<std::endl;
    std::vector<pin *>  ps=this->getPins();
    for(size_t i =0; i<ps.size();++i){
        out<<ps[i]->getNetsSize()<<std::endl;
    }
}

void evl_input::fileBehaviour(std::string fileName){
    //set input and validate input file
    std::ifstream input_file(fileName);
    if (!input_file) {
            std::cerr << "I can't read " << fileName<< "." << std::endl;
    }
    in.swap(input_file);
    std::string first_line;
    std::getline(in,first_line);
    line++;
    std::string token="";
    size_t line_no =0;
    for(line_no;first_line[line_no]!=' ';line_no++){
        token.push_back(first_line[line_no]);
    }
    if(this->getPins().size()!=std::stoi(token)){
        std::cerr<<"pin size not match in evl_input: "<<fileName<<std::endl;
    }
    //verify pin width

    for(size_t i = 0; i<this->getPins().size();++i){
        //std::cout<<this->getPins().size()<<std::endl;
        token="";
        //delete the space
        for(;line_no<first_line.size();line_no++){
            if(first_line[line_no]!=' '){
                break;
            }
        }
        //get the width
        for(;first_line[line_no]!=' ';line_no++){
            token.push_back(first_line[line_no]);
        }
      //  std::cout<<i<<" "<<token<<std::endl;
        //std::cout<<"line nomber "<<line_no<<token<<std::endl;
        //check the width
        if(this->getPins()[i]->getNets().size()!=std::stoi(token)){
           // std::cout<<"pin width: "<<getPins()[i]->getNets().size()<<" width in file: "<<std::stoi(token)<<std::endl;
            std::cerr<<"pin width not match in evl_input: "<<fileName<<std::endl;
        }
    }
    /*
    for(;line_no!=first_line.size()-1;line_no++){
            if(first_line[line_no]!=' '){
                std::cerr<<"there are extra characters in the first line"<<std::endl;
                break;
            }
    }*/



}

void evl_input::before_Iteration_Behaviour(){
    //for evl_input, it read the signal from the file and upload it to nets
    //read the signal from the file
   // std::cout<<"called"<<std::endl;

    int line_count = 0;
    std::string transition_string="";
    for(;line_count<current_line.length();line_count++){
            if(current_line[line_count]==' '){
                line_count++;
                break;
            }else{
                transition_string.push_back(current_line[line_count]);
            }
    }
    int transition_int = atoi(transition_string.c_str());
    //check for transistions
    if(transition_int==transitions){
        if(std::getline(in,current_line));
        line++;
        line_count=0;
        for(;line_count<current_line.length();line_count++){
                if(current_line[line_count]==' '){
                    line_count++;
                    break;
               }else{
                    transition_string.push_back(current_line[line_count]);
                }
        }
        transitions=0;
    }
    for(int i=0;i<this->getPins().size();i++){
        std::string hex = "";
         for(;line_count<current_line.length();line_count++){
             if(current_line[line_count]==' '){
                 line_count++;
                 break;
             }else{
                 hex.push_back(current_line[line_count]);
             }
         }
         //std::cout<<hex<<std::endl;
         getPins()[i]->hex_to_bin(hex);

    }
    transitions++;

}

void evl_lut::fileBehaviour(std::string fileName){
    std::ifstream input_file(fileName);
    if (!input_file) {
         std::cerr << "I can't read " << fileName<< "." << std::endl;
    }
    in.swap(input_file);
    std::string line ="";
    std::getline(in,line);//supoose there are no mistake with first line
    int address=0;
    for(;std::getline(in,line);){
        std::string word="";
        int line_count=0;
         for(;line_count<line.length();line_count++){
            if(line[line_count]==' '){
                line_count++;
                break;
             }else{
                 word.push_back(line[line_count]);
             }
        }
       // std::cout<<"address: "<<address<<" word: "<<word<<std::endl;
        look_up_table[address] = word;
        address++;
    }


}

void evl_lut::before_Iteration_Behaviour(){
    std::string address=this->getPins()[1]->bin_to_hex();
    int int_address = 0;
    int i;
    for(int t=0;t<address.length();++t){
        //std::cout<<"address["<<i<<"]: "<<address[i]<<std::endl;
        i = address.length()-t-1;
        if(address[t]=='1'){
            int_address+=pow(16,i);
        }else if(address[t]=='2'){
            int_address+=2*pow(16,i);
        }else if(address[t]=='3'){
            int_address+=3*pow(16,i);
        }else if(address[t]=='4'){
            int_address+=4*pow(16,i);
        }else if(address[t]=='5'){
            int_address+=5*pow(16,i);
        }else if(address[t]=='6'){
            int_address+=6*pow(16,i);
        }else if(address[t]=='7'){
            int_address+=7*pow(16,i);
        }else if(address[t]=='8'){
            int_address+=8*pow(16,i);
        }else if(address[t]=='9'){
            int_address+=9*pow(16,i);
        }else if(address[t]=='A'){
            int_address+=10*pow(16,i);
        }else if(address[t]=='B'){
            int_address+=11*pow(16,i);
        }else if(address[t]=='C'){
            int_address+=12*pow(16,i);
        }else if(address[t]=='D'){
            int_address+=13*pow(16,i);
        }else if(address[t]=='E'){
            int_address+=14*pow(16,i);
        }else if(address[t]=='F'){
            int_address+=15*pow(16,i);
        }
    }
    auto result = look_up_table.find(int_address);
    if(result!=look_up_table.end()){
        this->pins_[0]->hex_to_bin(result->second);
    }else{
        std::cout<<"error in address:"<<address<<" "<<int_address<<std::endl;
        //std::cout<<pow(16,0)<<std::endl;
        std::cerr<<"errors in evl_lut::before_Iteration_Behaviour, no such address"<<std::endl;
    }

}

//__________________废线「废弃车站下车之旅」__________________
//__________________validate_structural_semantics__________________
bool flip_flop::validate_structural_semantics() {

    if (pins_.size() != 3) return false;
    pins_[0]->set_as_output(); // q
    pins_[1]->set_as_input(); // d
    pins_[2]->set_as_input(); // clk
    this->setState('0');
    return true;
}

bool and_gate::validate_structural_semantics(){
    if (pins_.size() < 3) return false;
    pins_[0]->set_as_output(); // out
    for(size_t i=1;i<pins_.size();++i){
        pins_[i]->set_as_input(); // q
    }
        return true;
}

bool or_gate::validate_structural_semantics(){
    if (pins_.size() < 3) return false;
    pins_[0]->set_as_output(); // out
    for(size_t i=1;i<pins_.size();++i){
        pins_[i]->set_as_input(); // in_i
    }
        return true;
}

bool xor_gate::validate_structural_semantics(){
    if (pins_.size() < 3) return false;
    pins_[0]->set_as_output(); // out
    for(size_t i=1;i<pins_.size();++i){
        pins_[i]->set_as_input(); // in_i
    }
        return true;
}

bool not_gate::validate_structural_semantics() {

    if (pins_.size() != 2) return false;
    pins_[0]->set_as_output();
    pins_[1]->set_as_input();

    return true;
}

bool buf_gate::validate_structural_semantics() {

    if (pins_.size() != 2) return false;
    pins_[0]->set_as_output();
    pins_[1]->set_as_input();

    return true;
}

bool tris_gate::validate_structural_semantics() {

    if (pins_.size() != 3) return false;
    pins_[0]->set_as_output(); // out
    pins_[1]->set_as_input(); // in
    pins_[2]->set_as_input(); // en

    return true;
}

bool evl_clock::validate_structural_semantics() {

    if (pins_.size() != 1) return false;
    pins_[0]->set_as_output();

    return true;
}

bool evl_one::validate_structural_semantics(){
    if (pins_.size() != 1) return false;
    for(size_t i=0;i<pins_.size();++i){
        pins_[i]->set_as_output();
    }
        return true;
}

bool evl_zero::validate_structural_semantics(){
    if (pins_.size() != 1) return false;
    for(size_t i=0;i<pins_.size();++i){
        pins_[i]->set_as_output();
    }
        return true;
}

bool evl_input::validate_structural_semantics(){
    if (pins_.size() < 1) return false;
    for(size_t i=0;i<pins_.size();++i){
        pins_[i]->set_as_output();
    }
        return true;
}

bool evl_output::validate_structural_semantics(){
    if (pins_.size() <= 1) return false;
    for(size_t i=0;i<pins_.size();++i){
        pins_[i]->set_as_input();
    }
        return true;
}

bool evl_lut::validate_structural_semantics(){
    if (pins_.size() != 2) return false;
    pins_[0]->set_as_output();
    pins_[1]->set_as_input();
    return true;
}
//__________________compute_next_state_or_output__________________
void flip_flop::compute_next_state_or_output() {
   // std::cout<<"compute_next_state_and_output in flip_flop called"<<std::endl;
    next_state_ = pins_[1]->compute_signal(); // d=
    //std::cout<<"next_state_ in flip_flop: "<<next_state_<<std::endl;
}

void and_gate::compute_next_state_or_output() {
}

void or_gate::compute_next_state_or_output() {
}

void xor_gate::compute_next_state_or_output() {
}

void not_gate::compute_next_state_or_output() {

}

void buf_gate::compute_next_state_or_output() {
}

void tris_gate::compute_next_state_or_output() {
}

void evl_clock::compute_next_state_or_output() {
}

void evl_one::compute_next_state_or_output() {
}

void evl_zero::compute_next_state_or_output() {
}

void evl_input::compute_next_state_or_output() {
}

void evl_output::compute_next_state_or_output() {
   // std::cout<<"evl_output compute_next_state_or_output activited "<<std::endl;
    for(pin *p: pins_){
        p->compute_signal();
        out<<p->bin_to_hex()<<" ";
    //    std::cout<<p->bin_to_hex()<<std::endl;
    }
    out<<std::endl;
}

//__________________compute_signal__________________
char flip_flop::compute_signal(int pin_index) {

    assert (pin_index == 0); // must be q
   // std::cout<<"state: "<<state_<<std::endl;
    return state_;
}

char and_gate::compute_signal(int pin_index) {
    assert(pin_index==0);
    char flag ='1';
    for(int i=1;i<pins_.size();++i){
        if(pins_[i]->compute_signal()=='0'){
            flag='0';
            //return '0';
        }
    }
    return flag;
}

char or_gate::compute_signal(int pin_index) {
    assert(pin_index==0);
    for(int i=1;i<pins_.size();++i){
            if(pins_[i]->compute_signal()=='1'){
                return '1';
            }
    }
    return '0';
}

char xor_gate::compute_signal(int pin_index) {
    assert(pin_index==0);
    int count = 0;
    for(int i=1;i<pins_.size();++i){
        if(pins_[i]->compute_signal()=='1'){
            count++;
        }
    }
    if(count%2==0){
        return '0';
    }
    else{
        return '1';
    }

}

char not_gate::compute_signal(int pin_index) {
    assert(pin_index==0);
    if(pins_[1]->compute_signal()=='0'){
        return '1';
    }else{
        return '0';
    }
}

char buf_gate::compute_signal(int pin_index) {
    assert(pin_index==0);
        if(pins_[1]->compute_signal()=='0'){
            return '0';
        }else if(pins_[1]->compute_signal()=='1'){
            return '1';
        }else if(pins_[1]->compute_signal()=='Z'){
            return 'Z';
        }
        std::cerr<<"error in buf_gate compute signal"<<std::endl;
}

char tris_gate::compute_signal(int pin_index) {
    assert(pin_index==0);
    if(pins_[2]->compute_signal()=='1'){
        return pins_[1]->compute_signal();
    }else{
        return 'Z';
    }
    //std::cerr<<"error in tris_gate compute signal"<<std::endl;
}

char evl_clock::compute_signal(int pin_index) {
    std::cerr<<"seems evl_clock is a fake gate"<<std::endl;
    return 'X';
}

char evl_one::compute_signal(int pin_index) {
    assert (pin_index == 0);
    return '1';
}

char evl_zero::compute_signal(int pin_index) {
    assert (pin_index == 0);
    return '0';
}

char evl_input::compute_signal(int pin_index) {
    //undefined
    return 'X';
}

char evl_output::compute_signal(int pin_index) {
    std::cerr<<"shouldn't use compute_signal in evl_output"<<std::endl;
    return 'X';
}

//__________________ netlist member function_______________________

std::string netlist::make_net_name(std::string wire_name, int i) {
    assert(i >= 0);
    std::ostringstream oss;
    oss << wire_name << "[" << i << "]";
    return oss.str();
}

void netlist::create_net(std::string net_name) {
    assert(nets_table_.find(net_name) == nets_table_.end());
    net *n = new net(net_name);
    nets_table_[net_name] = n;
   // std::cout<<net_name<<std::endl;
    nets_.push_back(n);
}

bool netlist::create_nets(const evl_wires &wires) {
    for (size_t t = 0; t != wires.size(); ++t) {
        if (wires[t].width == 1) {
             create_net(wires[t].name);
             //std::cout<<wires[t].name<<std::endl;
        } else if (wires[t].width > 1) {
            for (int i = 0; i <wires[t].width; ++i) {
            create_net(make_net_name(wires[t].name,i));
            //std::cout<<make_net_name(wires[t].name,i)<<std::endl;
            }
        } else {
            std::cout<<"error in create_net"<<std::endl;
            return false;
        }
    }
    return true;
}

bool netlist::create_gates(const evl_components &comps,
        const evl_wires_table &wires_table) {
    for (size_t t = 0; t != comps.size(); ++t) {
        if(!create_gate(comps[t], wires_table)){
            std::cout<<"error in create_gate"<<std::endl;
        }
    }
    return true;
}

bool netlist::create_gate(const evl_component &c,
        const evl_wires_table &wires_table) {
    if (c.type == "evl_dff") {
            gates_.push_back(new flip_flop());
        }
    else if(c.type =="and"){
        gates_.push_back(new and_gate());
    }
    else if(c.type =="or"){
        gates_.push_back(new or_gate());
    }
    else if(c.type =="xor"){
        gates_.push_back(new xor_gate());
    }
    else if(c.type =="not"){
        gates_.push_back(new not_gate());
    }
    else if(c.type =="buf"){
        gates_.push_back(new buf_gate());
    }
    else if(c.type =="tris"){
        gates_.push_back(new tris_gate());
    }
    else if(c.type =="evl_clock"){
        gates_.push_back(new evl_clock());
    }
    else if(c.type =="evl_one"){
        gates_.push_back(new evl_one());
    }
    else if(c.type =="evl_zero"){
        gates_.push_back(new evl_zero());
    }
    else if(c.type =="evl_input"){
        gates_.push_back(new evl_input());
    }
    else if(c.type =="evl_output"){
        gates_.push_back(new evl_output());
    }else if(c.type =="evl_lut"){
        gates_.push_back(new evl_lut());
    }else{
        std::cout<<"no such gate: "<<c.type<<std::endl;

        return false;
    }

    return gates_.back()->create(c, nets_table_, wires_table);
}

bool netlist::create(const evl_wires &wires, const evl_components &comps,
        const evl_wires_table &wires_table) {
    return create_nets(wires) && create_gates(comps, wires_table);
}

void netlist::save(std::ostream &out, std::string module_name){
    out<< "module "<<module_name<<std::endl;
    out<<"nets "<<nets_.size()<<std::endl;

    for(std::list<net *>::iterator i = nets_.begin();i!=nets_.end(); ++i){
        std::list<pin *> pins = (**i).getConnections();
        out<<" net "<<(**i).getName()<<" "<<pins.size()<<std::endl;
        for(std::list<pin *>::iterator t=pins.begin(); t!=pins.end(); ++t){
            if((**t).showGateName()==""){
                out<<"  "<<(**t).showGateType()<<" "<<(**t).getIndex()<<std::endl;
            }else{
                out<<"  "<<(**t).showGateType()<<" "<<(**t).showGateName()<<" "<<(**t).getIndex()<<std::endl;
            }
        }
    }

    out<<"components "<<gates_.size()<<std::endl;
    for(std::list<gate *>::iterator i = gates_.begin();i!=gates_.end();++i){
        std::vector<pin *> pins = (**i).getPins();
        if((**i).getName()==""){
            out<<" component "<<(**i).getType()<<" "<<pins.size()<<std::endl;
        }else{
            out<<" component "<<(**i).getType()<<" "<<(**i).getName()<<" "<<pins.size()<<std::endl;
        }
        for(size_t t= 0; t != pins.size() ; ++t){
           //simple output
            out<<"   "<<"pin "<<(*pins[t]).getNetsSize()<<" "<<(*pins[t]).showNetsName()<<std::endl;

        }

    }

}

void netlist::compute_next_state_and_output(){
    //std::cout<<"compute_next_state_and_output called"<<std::endl;
    for (net *n: nets_)
        n->set_signal('?');
    for (gate *g: gates_){
        if(g->getType()=="evl_input"){
            g->before_Iteration_Behaviour();
        }
    }
    for (gate *g: gates_){
            if(g->getType()=="evl_lut"){
                g->before_Iteration_Behaviour();
            }
        }
    for (gate *g: gates_)
        g->compute_next_state_or_output();
    for (gate *g: gates_){
        g->end_Iteration_Behaviour();
    }

}

void netlist::load_file(std::string fileName){
    for(gate *g: gates_){
        //g->getPins()[0]->bin_to_hex();
        if(g->getType()=="evl_output"){
            std::string output_file = fileName+"."+g->getName()+".evl_output";
            g->fileBehaviour(output_file);
        }else if(g->getType()=="evl_input"){
            std::string input_file = fileName+"."+g->getName()+".evl_input";
            g->fileBehaviour(input_file);
        }else if(g->getType()=="evl_lut"){
            std::string input_file = fileName+"."+g->getName()+".evl_lut";
            g->fileBehaviour(input_file);
        }
    }
}

void netlist::simulate(size_t n){
    for(int i=0;i<n;i++){
        //std::cout<<i<<std::endl;
        compute_next_state_and_output();
    }
}


