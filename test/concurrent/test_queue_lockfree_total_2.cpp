#include <thread>
#include <vector>
#include <iostream>
#include <mutex>
#include <set>
#include <chrono>
#include <cassert>

#include "queue_lockfree_total.hpp"

using namespace std;

int main(){

    queue_lockfree_total<int, trait_reclamation::hp> queue;

    unsigned int num_threads = std::thread::hardware_concurrency();
    
    size_t nums = num_threads * 10000;
    
    vector<int> retrieved( nums, 0);

    vector<thread> threads( num_threads );
    vector<thread> threads2( num_threads );
    
    auto t0 = std::chrono::high_resolution_clock::now();
        
    for( int i = 0; i < num_threads; ++i ){
        threads[i] = std::thread( [ &, i ](){
                int val = nums/num_threads*i;
                for( int j = 0; j < nums/num_threads; ++j ){
                    while( !queue.put( val + j ) ){} //force enqueue
                }
                queue_lockfree_total<int, trait_reclamation::hp>::thread_deinit();
            } );
    }

    for( int i = 0; i < num_threads; ++i ){
        threads2[i] = std::thread( [&](){
                for( int j = 0; j < nums/num_threads; ++j ){
                    while(true){
                        if(auto ret = queue.get()){
                            ++retrieved[*ret];
                            break;
                        }
                        std::this_thread::yield();
                    } 
                }
                queue_lockfree_total<int, trait_reclamation::hp>::thread_deinit();
            } );
    }
  
    for( auto & i : threads ){
        i.join();
    }
    for( auto & i : threads2 ){
        i.join();
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur = t1 - t0;
    auto dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
    std::cout << "duration: " << dur_ms.count() << " ms. rate: " <<  (double)nums/dur_ms.count()*1000.0 << " put-get/sec." << std::endl;
    
    size_t count = queue.size();

    assert( 0 == count );

    int k = 0;
    int n = 0;
    //expect count of 1 for each retrieved number
    for(auto i: retrieved){
        if(i!=1){
            //oops, something is wrong, write out count
            ++k;
            std::cout << n << ": " << i << std::endl;
        }
        ++n;
    }
    
    assert( 0 == k );

    return 0;
}