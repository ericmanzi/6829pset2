#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

float cwnd = 1000;
float factor = 2;
unsigned int max_wnd = 80;
unsigned int last_sequence_number_sent = 0;
unsigned int last_sequence_number_acked = 0;
unsigned int num_acks_since_last_md = 0;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  /* Default: fixed window size of 100 outstanding datagrams */
  unsigned int the_window_size = (unsigned int) cwnd;

//  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << the_window_size << endl;
//  }

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
  if ((unsigned int) rtt > timeout_ms() ) {
    cerr << "Timeout exceeded: " << rtt << endl;
    if (num_acks_since_last_md > window_size()) {
//      cwnd = cwnd/factor;
//      cerr << "acks since last md:" << num_acks_since_last_md << endl;
//      num_acks_since_last_md = 0;
    }
  } else {
    cerr << "within timeout: " << rtt << endl;
//    cwnd += 1;
  }


// if Duplicate
//  if (last_sequence_number_acked == sequence_number_acked) {
//    cerr << "found duplicate ack: " << sequence_number_acked << endl;
//  }

  num_acks_since_last_md++;
  last_sequence_number_acked = sequence_number_acked;
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 1000; /* timeout of one second */
}
