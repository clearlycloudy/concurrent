//---based on Art of Multiprocessor Programming section 11.4
#ifndef EXCHANGER_LOCKFREE_H
#define EXCHANGER_LOCKFREE_H

#include <atomic>

enum class exchanger_status { //expected to be monotically increasing in terms of exchange progression
    EMPTY, //resource is empty and is ready to accept deposit from an active thread
    EMPTY_2, //active thread in process of depositing its value
    WAITING, //active thread has deposited its value
    EXCHANGING, //2nd thread has detected a presence of an active thread
    EXCHANGING_2, //2nd thread has claimed resource and is in process of exchanging its value
    EXCHANGING_3, //2nd thread has exchanged and deposited its value and the active thread will be exchanging its value with the 2nd thread's deposited value
    COMPLETE,
    COMPLETE_2,
};

template< class T >
class exchanger_lockfree {
public:

    static_assert(std::is_default_constructible<T>::value);
    static_assert(std::is_move_assignable<T>::value);
    static_assert(std::is_move_constructible<T>::value);
    
    using _t_val = T;
    class Node;
    // using _t_node = std::atomic< Node * >;
    using _t_status = std::atomic< exchanger_status >;
               exchanger_lockfree();
              ~exchanger_lockfree();
               exchanger_lockfree( exchanger_lockfree const & ) = delete;
               exchanger_lockfree & operator=( exchanger_lockfree const & ) = delete;
    _t_status _status;
       _t_val _val;
         bool exchange( T & item, long timeout_us ); //true if exchanged with another thread, false if timed out; item is moved
};

#include "exchanger_lockfree.tpp"

#endif
