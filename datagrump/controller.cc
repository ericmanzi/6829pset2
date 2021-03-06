#include <iostream>
#include <limits.h>
#include <cmath>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

float cwnd = 1;
float ai_init = 1;
float ai = ai_init;
float md_factor = 2;
float ad = 1;
float delta_rtt = 0;
float ewma_alpha = 0.9;
float last_rtt = 50;
//float max_cwnd = 80;

uint64_t sent_table[50000];
uint64_t last_sequence_number_sent = 0;
uint64_t last_sequence_number_acked = 0;
unsigned int num_acks_til_next_md = 0;
uint64_t min_rtt = (uint64_t) 50;
unsigned int ceil_threshold_factor = 2.4;
unsigned int floor_threshold_factor = 1.1;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  unsigned int the_window_size = (unsigned int) cwnd;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
  }
  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }

  sent_table[(int) sequence_number] = send_timestamp;
  last_sequence_number_sent = sequence_number;
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }



  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  min_rtt = (rtt < min_rtt) ? rtt : min_rtt;
  float critical_rtt = (ceil_threshold_factor * min_rtt);
  delta_rtt = ewma_alpha * (rtt - last_rtt) + (1.0 - ewma_alpha)*delta_rtt;


//  if (num_acks_til_next_md < 1) {
//  cerr << "*************" << endl;
//  cerr << "delta_rtt: " << delta_rtt << endl;
//  cerr << "rtt: " << rtt << ", critical: " << critical_rtt << endl;



  if (rtt < floor_threshold_factor * min_rtt ) {
    cwnd += ai;
  } else if (rtt > 3.2 * min_rtt) {
    cwnd -= ad;
    ai = ai_init;
  } else {

    if (rtt > critical_rtt) {

      if (delta_rtt < 0) {
        ad = (rtt / critical_rtt) * 0.1;
      } else { // delta_rtt > 0
        ad = (rtt / critical_rtt) * 0.7;
      }
//      cerr << "ad: " << ad << endl;

//      cwnd -= ad/cwnd;
      cwnd -= ad;
      ai = ai_init;
    } else { // rtt < critical_rtt


      if (delta_rtt < 0) {
        ai *= 1.005;
        cwnd+= ai*1.5/cwnd;
      } else { // delta_rtt > 0
        cwnd+=ai*0.1/cwnd;
      }
    }

    /* check if timeout exceeded for packets that have not yet been acked */
    for ( uint64_t i = sequence_number_acked; i < std::max(last_sequence_number_sent, sequence_number_acked+1); i++ ) {
      uint64_t delay_so_far = timestamp_ms() - sent_table[i];
      if (delay_so_far > critical_rtt) {
        if (num_acks_til_next_md < 1) {
          cwnd -= ad;
          num_acks_til_next_md = last_sequence_number_sent - sequence_number_acked;
        }
//        cwnd -= ad;
        break;
      }
    }

//    }

  }


  cwnd = (cwnd > 1) ? cwnd : 1;
//  cwnd = (cwnd < max_cwnd) ? cwnd : max_cwnd;
//  cerr << string(window_size(), '.') << "  " << rtt << endl;
//  cerr << "window: " << cwnd << endl;

  if (num_acks_til_next_md > 0) num_acks_til_next_md--;
  last_sequence_number_acked = sequence_number_acked;
  last_rtt = rtt;

}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return (unsigned int) ceil_threshold_factor * min_rtt;
//  return 110;
}
