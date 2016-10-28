#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

#define ALPHA 0.3
#define BETA 0.5
#define LOWER_THRESH (1.1 * min_rtt_)
#define UPPER_THRESH (3 * min_rtt_)
#define TARGET_RTT (2 * min_rtt_)

unsigned int window_size_=1;

  unsigned int num_ack_until_increment_=0;

  uint64_t prev_rtt_=0;

  uint64_t min_rtt_=0;

  double rtt_diff_=0.0;

  uint64_t md_interval_=0;


/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
{
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
	 << " window size is " << window_size_ << endl;
  }

  return window_size_;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
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
  uint64_t rtt = timestamp_ack_received - send_timestamp_acked;
  if(min_rtt_ == 0 || rtt < min_rtt_) min_rtt_ = rtt;
  if(prev_rtt_ == 0) prev_rtt_ = rtt;
  if(md_interval_ > 0) md_interval_--;;

  rtt_diff_ = ALPHA * (int)(rtt - prev_rtt_) + (1.0 - ALPHA) * rtt_diff_;

  // Queue is nearly empty
  if(rtt < LOWER_THRESH) {
    // Aggressively increase the window size
    window_size_ ++;

    num_ack_until_increment_ = 0;

  // Queue is nearly flooded
  } else if (rtt > UPPER_THRESH) {

    /* If MD happens lately, wait until the effect is noticeable.
       The reasoning here is, we do not want to penalty the window multiple times.
       When congestion happens, packets sent in same window time would suffer
       the congestion altogether. So we should wait for some time until
       MD's effect can be noticeable. */
    if(md_interval_ == 0) {
      md_interval_ = window_size_;
      double coeff = BETA * (1.0 - (TARGET_RTT/(double)rtt));
      window_size_ *= (1.0 - coeff);
      if(window_size_ < 1) window_size_ = 1;

      num_ack_until_increment_ = 0;
    }

  // Queue is draining
  } else if (rtt_diff_ <= 0) {

    /* If rtt is larger than TARGET_RTT, do nothing, hoping congestion would be
       resolved naturally.
       If rtt is smaller than TARGET_RTT, we should inflate the window size not
       to waste throughput. */
    if(rtt <= TARGET_RTT) {
      num_ack_until_increment_ ++;
      if(num_ack_until_increment_ == 1.5 * window_size_) {
        num_ack_until_increment_ = 0;
        window_size_ ++;
      }
    }

  // Queue is filling up
  } else { // rtt_diff_ > 0

    /* If rtt is larger than TARGET_RTT, aggressively deflate the window to
       resolve the congestion asap. */
    if(rtt > TARGET_RTT) {
      if(window_size_ > 1) {
        window_size_ --;
        num_ack_until_increment_ = 0;
      }

    // If rtt is smaller than TARGET_RTT, divide the case.
    } else {

      // If queue is filling up slowly, inflate the window to saturate the link.
      if((TARGET_RTT - rtt) * 2 > rtt_diff_ * window_size_) {
        num_ack_until_increment_ ++;
        if(num_ack_until_increment_ == 1.5 * window_size_) {
          num_ack_until_increment_ = 0;
          window_size_ ++;
        }

      // If queue is filling up rapidly, slowly deflate the window as we expect
      // congestion shortly.
      } else if ((TARGET_RTT - rtt) * 4 < rtt_diff_ * window_size_) {
        if(window_size_ > 1) {
          if(num_ack_until_increment_ == 0) {
            window_size_ --;
            num_ack_until_increment_ = 1.5 * window_size_;

          } else
            num_ack_until_increment_ --;
        }
      }
    }
  }

  prev_rtt_ = rtt;

  cerr << string(window_size(), '.') << "  " << rtt << endl;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return (min_rtt_ == 0) ? 100 : 2 * min_rtt_;
}