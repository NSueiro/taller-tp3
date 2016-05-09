#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 1
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <map>
#include <string>
#include "common_skt.h"
#include <sstream>
#include "client_mapper.h"

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
	std::istringstream is_day(day);
	is_day >> i_day;

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
		std::stringstream day_buf;
		day_buf << it->first; 
		std::stringstream temp_buf;
		temp_buf << it->second;
		std::string buffer = "";
		buffer += day_buf.str();
		buffer += " ";
		buffer += temp_buf.str();
		char *c_str_package = (char*) buffer.c_str();
		int i = buffer.size();
		socket_send(this->s, &i, INT_SIZE);
		socket_send(this->s, c_str_package, i);
	}
}