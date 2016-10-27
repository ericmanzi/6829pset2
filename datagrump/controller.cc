#include <iostream>
#include <limits.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

float cwnd = 1;
float ai_init = 1;
float ai = ai_init;
float md_factor = 2;
uint64_t sent_table[50000];
uint64_t rtt_table[50000];
uint64_t last_sequence_number_sent = 0;
uint64_t last_sequence_number_acked = 0;
unsigned int num_acks_til_next_md = 0;
uint64_t min_rtt = (uint64_t) UINT_MAX/100;
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
  float target_rtt = (ceil_threshold_factor * min_rtt);

//  if (num_acks_til_next_md < 1) {

    /* check if timeout exceeded for packets that have not yet been acked */
    for ( uint64_t i = sequence_number_acked; i < std::max(last_sequence_number_sent, sequence_number_acked+1); i++ ) {
      uint64_t delay_so_far = timestamp_ms() - sent_table[i];
      if (delay_so_far > target_rtt) {
  //        cerr << "************" << endl;
  //        cerr << "At time " << timestamp_ms() << ", havent received ack for packet: " << i << ", delay: " << delay_so_far << ", sent at: " << sent_table[i] << ", current rtt: " << rtt << ", last acked:" << last_sequence_number_acked << endl;
  //        cerr << "************" << endl;

          // Wait for buffer to clear the last window before decreasing the window size
  //          if ( rtt > ceil_threshold_factor * min_rtt ) {

//          md_factor = (( (rtt-target_rtt) / target_rtt ) * 0.05) + 1;
          cwnd = cwnd/md_factor;

          ai = ai_init;
          num_acks_til_next_md = (unsigned int) 1.5 * window_size();
//          num_acks_til_next_md = last_sequence_number_sent - sequence_number_acked;
          break;
  //          }

        }
     }
//  }



  ai *= 1.005;
  if ( rtt < floor_threshold_factor * min_rtt ) {
    cwnd+=ai;
  } else {
    cwnd+=ai/cwnd;
  }

  if (num_acks_til_next_md > 0) num_acks_til_next_md--;
  last_sequence_number_acked = sequence_number_acked;


}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return (unsigned int) ceil_threshold_factor * min_rtt;
//  return 110;
}
