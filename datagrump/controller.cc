#include <iostream>
#include <limits.h>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

float cwnd = 1;
float drop_factor = 2;
unsigned int max_wnd = 22;
unsigned int last_sequence_number_sent = 0;
unsigned int last_sequence_number_acked = 0;
unsigned int num_acks_til_next_md = 0;
uint64_t min_rtt = (uint64_t) UINT_MAX;
unsigned int ceil_threshold_factor = 2.5;
unsigned int floor_threshold_factor = 1.3;

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


  /* Check if timeout exceeded */
  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;

  min_rtt = (rtt < min_rtt) ? rtt : min_rtt;

  if (num_acks_til_next_md < 1) {
    if ( rtt > ceil_threshold_factor * min_rtt ) {
//      cerr << "************* DROP ********** " << "RTT: " << rtt << ", minRTT: " << min_rtt << endl;
      num_acks_til_next_md = window_size();
      cwnd = cwnd/drop_factor;
    } else {
      cwnd++;
    }
  }
  if ( rtt < floor_threshold_factor * min_rtt ) {
//    cerr << "************* Close to min ********** " << "RTT: " << rtt << endl;
    cwnd++;
  }

//  if ( rtt > threshold_factor * min_rtt ) {
//  if (sequence_number_acked - last_sequence_number_acked > 1 ) {
//    cerr << "!!!Drop. Seqnum acked: " << sequence_number_acked <<", last acked: " << last_sequence_number_acked << endl;
//    cerr << "acks til next md:" << num_acks_til_next_md << endl;
//    if (num_acks_til_next_md < 1) {
//      num_acks_til_next_md = window_size();
//      cwnd = cwnd/drop_factor;
//    }
//  }
//  cwnd = (cwnd >= max_wnd) ? max_wnd : cwnd+1;
//  cwnd++;

//  cerr << "acks til next md:" << num_acks_til_next_md << endl;

  if (num_acks_til_next_md > 0) num_acks_til_next_md--;
  last_sequence_number_acked = sequence_number_acked;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
