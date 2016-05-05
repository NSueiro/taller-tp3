#include <map>
#include <string>
#include "common_skt.h"
#include <sstream>
#include "mapper.h"

#define INT_SIZE 4
#define END_SIG "End\n"

Mapper::Mapper(socket_t *s){
	this->s = s;
}

void Mapper::map(const std::string& line){
	std::string city;
	std::string temp;
	int i_temp;
	std::string day;
	int i_day;
	
	std::stringstream ss(line);
	// First token is the name of the city
	ss >> city;
	if (this->city == "") this->city = city;
	
	// Second token is the temperature
	ss >> temp;
	std::istringstream is_temp(temp);
	is_temp >> i_temp;
	
	// Third token is the day
	ss >> day;
	std::istringstream is_day(temp);
	is_temp >> i_day;

	std::map<int, int>::iterator it = this->max_temps.find(i_day);
	if (it == this->max_temps.end()) this->max_temps[i_day] = i_temp;
	else if (this->max_temps[i_day] < i_temp) this->max_temps[i_day] = i_temp;	
}

void Mapper::send_city(){
	int str_size = this->city.size();
	socket_send(this->s, &str_size, INT_SIZE);
	char *c_str_city = (char*) this->city.c_str();
	socket_send(this->s, c_str_city, this->city.size());
}

void Mapper::send_temp_info(){
	for (std::map<int,int>::iterator it = this->max_temps.begin(); 
		it != this->max_temps.end(); ++it){
		std::ostringstream buffer;
		buffer << it->first << " " << it->second;
		char *c_str_package = (char*) buffer.str().c_str();
		socket_send(this->s, c_str_package, buffer.str().size());
	}
}

void Mapper::send_data(){
	this->send_city();
	this->send_temp_info();
}

int main(){
	return 0;
}
