#ifndef MAPPER_H
#define MAPPER_H

#include <map>
#include <string>
#include "common_skt.h"

class Mapper{
	private:
		std::map<int, int> max_temps;
		std::string city;
		socket_t *s;
		void send_city();
		void send_temp_info();
	public:
		Mapper(socket_t *s);
		// Checks the city and adds the temperature to the max_temps map if it's
		// higher than the one that was stored
		void map(const std::string& line); 
		void send_data();
};

#endif
