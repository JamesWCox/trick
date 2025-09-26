
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <cstring>

#include "trick/MessagePublisher.hh"
#include "trick/message_proto.h"
#include "trick/exec_proto.h"
#include "trick/memorymanager_c_intf.h"

#define MAX_MSG_HEADER_SIZE 256
#define PRNT_FMT_V1 

Trick::MessagePublisher * the_message_publisher ;

Trick::MessagePublisher::MessagePublisher() {

    sim_name = " " ;
    the_message_publisher = this ;

    tics_per_sec = 1000000 ;
    set_print_format() ;

}

void Trick::MessagePublisher::set_print_format() {
    set_print_format(5, 5, 5);
}

void Trick::MessagePublisher::set_print_format(int digits, int left, int right) {
	num_digits = digits;  
	radix_left = left; 
	radix_right = right;
#if defined(PRNT_FMT_V1) // | Trick Time | Sim Time | <Sim Status>
	sprintf(print_format,  "|%%%dlld.%%0%dlld|%%11.5f| ", radix_left, radix_right) ;

#else	// Original 
    num_digits = (int)round(log10((double)tics_per_sec)) ;
    sprintf(print_format, "|L %%3d|%%s|%%s|%%s|T %%d|%%lld.%%0%dlld| ", num_digits) ;
#endif 
}

int Trick::MessagePublisher::init() {
    tics_per_sec = exec_get_time_tic_value() ;
    set_print_format() ;
    return 0 ;
}

int Trick::MessagePublisher::publish(int level , std::string message) {

    /** @par Design Details: */
    std::list<Trick::MessageSubscriber *>::iterator p ;

    char date_buf[MAX_MSG_HEADER_SIZE];
    char header_buf[MAX_MSG_HEADER_SIZE];
    char hostname[64];
    time_t date ;
    std::string header ;
    long long tics = exec_get_time_tics() ;

	REF2 *pREF2 = 0;
	double simTime = 0;
	pREF2 = ref_attributes( "Sim.Rte.timer.sim_time_stamp" );
	if ( pREF2 != 0 && pREF2->ref_type == 0) {
		memcpy( &simTime, pREF2->address, sizeof(simTime) );
	}
	
    /** @li Create message header with level, date, host, sim name, process id, sim time. */
    date = time(NULL) ;
    strftime(date_buf, (size_t) 20, "%Y/%m/%d,%H:%M:%S", localtime(&date));

#if defined(PRNT_FMT_V1) // | Trick Time | Sim Time | <Sim Status>
	sprintf(header_buf , print_format, tics/tics_per_sec, 
            (long long)((double)(tics % tics_per_sec) * (double)(pow(10 , num_digits)/tics_per_sec)), simTime ) ; 

#else // Original 
    (void) gethostname(hostname, (size_t) 48);
    sprintf(header_buf , print_format , level, date_buf, hostname,
            sim_name.c_str(), exec_get_process_id(), tics/tics_per_sec ,
            (long long)((double)(tics % tics_per_sec) * (double)(pow(10 , num_digits)/tics_per_sec)) ) ;
#endif 
    header = header_buf ;

    /** @li Go through all its subscribers and send a message update to the subscriber that is enabled. */
    if ( ! subscribers.empty() ) {
        for ( p = subscribers.begin() ; p != subscribers.end() ; p++ ) {
            if ( (*p)->enabled ) {
                (*p)->update(level , header , message) ;
            }
        }
    } else {
        // If there are no subscribers, that probably means things have not been inited yet... just print message only

        // Building the final string in a temporary stream ensures an automic call to cout, which prevents
        // multithreaded sims from interleaving header and message elements.
        std::ostringstream oss;
        oss << header << message ;
        std::cout << oss.str() << std::flush ; } return(0) ;

}

Trick::MessageSubscriber * Trick::MessagePublisher::getSubscriber( std::string sub_name ) {
    std::list<Trick::MessageSubscriber *>::iterator lit ;
    for ( lit = subscribers.begin() ; lit != subscribers.end() ; lit++ ) {
        if ( ! (*lit)->name.compare(sub_name) ) {
            return *lit ;
        }
    }
    return NULL ;
}
