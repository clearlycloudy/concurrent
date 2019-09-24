//specialization for hazard pointer reclamation

#include <iostream>
#include "reclaim_hazard.hpp"

template< typename T >
queue_lockfree_total_impl<T, trait_reclamation::hp>::queue_lockfree_total_impl(){
    Node * sentinel = new Node();
    _head.store( sentinel );
    _tail.store( sentinel );
}
template< typename T >
queue_lockfree_total_impl<T, trait_reclamation::hp>::~queue_lockfree_total_impl(){

    //must ensure there are no other threads accessing the datastructure
  
    clear();

    thread_deinit();

    reclaim_hazard<Node>::final_deinit();
    
    if( _head ){
        Node * n = _head.load();
        if( _head.compare_exchange_strong( n, nullptr, std::memory_order_relaxed ) ){
            if( n ){
                delete n;
                _head.store(nullptr);
                _tail.store(nullptr);
            }
        }
    }
}

template< typename T >
void queue_lockfree_total_impl<T, trait_reclamation::hp>::thread_deinit(){
    reclaim_hazard<Node>::thread_deinit();
}

template< typename T >
bool queue_lockfree_total_impl<T, trait_reclamation::hp>::push_back( T const & val ){ //push item to the tail
    Node * new_node = new Node( val );
    while( true ){
	Node * tail = _tail.load( std::memory_order_relaxed );

	hazard_guard<Node> guard = reclaim_hazard<Node>::add_hazard( tail );
	
        if( nullptr == tail ){
            return false;
        }
                
        if( !_tail.compare_exchange_weak( tail, tail, std::memory_order_relaxed ) ){
	    continue;
        }
        
        Node * tail_next = tail->_next.load( std::memory_order_relaxed );
        if( nullptr == tail_next ){  //determine if thread has reached tail
            if( tail->_next.compare_exchange_weak( tail_next, new_node, std::memory_order_acq_rel ) ){ //add new node
                _tail.compare_exchange_weak( tail, new_node, std::memory_order_relaxed ); //if thread succeeds, set new tail
                return true;
            }
        }else{
            _tail.compare_exchange_weak( tail, tail_next, std::memory_order_relaxed ); //update tail and retry
        }
    }
}
template< typename T >
bool queue_lockfree_total_impl<T, trait_reclamation::hp>::pop_front( T & val ){ //obtain item from the head
    while( true ){
        Node * head = _head.load( std::memory_order_relaxed );

	hazard_guard<Node> guard1 = reclaim_hazard<Node>::add_hazard( head );
	
        if( nullptr == head ){
            return false;
        }

	if(!_head.compare_exchange_weak( head, head, std::memory_order_relaxed )){
	    continue;
        }
		
	Node * tail = _tail.load( std::memory_order_relaxed );
		
        Node * head_next = head->_next.load( std::memory_order_relaxed );

	hazard_guard<Node> guard2 = reclaim_hazard<Node>::add_hazard(head_next);
	
        if(!_head.compare_exchange_weak( head, head, std::memory_order_relaxed )){
	    continue;
        }

	if( head == tail ){
	    if( nullptr == head_next ){//empty
		return false;
	    }else{
		_tail.compare_exchange_weak( tail, head_next, std::memory_order_relaxed ); //other thread updated head/tail, so retry
	    }
	}else{
	    //val = head_next->_val; //optimization: reordered to after exchange due to hazard pointer guarantees
	    if( _head.compare_exchange_weak( head, head_next, std::memory_order_relaxed ) ){ //try add new item
		//thread suceeds
		val = head_next->_val; 
		reclaim_hazard<Node>::retire_hazard(head);
		return true;
	    }
	}
    }
}

template< typename T >
size_t queue_lockfree_total_impl<T, trait_reclamation::hp>::size(){
    size_t count = 0;
    Node * node = _head.load();
    if( nullptr == node ){
        return 0;
    }
    while( node ){
        Node * next = node->_next.load();
        node = next;
        ++count;
    }
    return count - 1; //discount for sentinel node
}
template< typename T >
bool queue_lockfree_total_impl<T, trait_reclamation::hp>::empty(){
    return size() == 0;
}
template< typename T >
bool queue_lockfree_total_impl<T, trait_reclamation::hp>::clear(){
    size_t count = 0;
    while( !empty() ){
        T t;
        pop_front( t );
        count++;
    }
    return true;
}