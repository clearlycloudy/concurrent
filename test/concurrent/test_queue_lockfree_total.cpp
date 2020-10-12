#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include <thread>
#include <vector>
#include <iostream>
#include <mutex>
#include <set>
#include <chrono>

#include "catch.hpp"
#include "queue_lockfree_total.hpp"

using namespace std;

TEST_CASE( "queue_lockfree_total", "[queue push pop]" ) { 
    
    SECTION( "push-pop" ) {
    
        queue_lockfree_total<int, trait_reclamation::hp> queue;
            
        size_t count = queue.size();
        CHECK( 0 == count );
        int val = 5;
        queue.put(val);
        count = queue.size();
        CHECK( 1 == count );

        auto ret = queue.get();
        count = queue.size();
        CHECK( 0 == count );
        CHECK( ret );
        CHECK( 5 == *ret );
    
        queue_lockfree_total<int, trait_reclamation::hp>::thread_deinit();
    }
    SECTION( "pop_empty" ) {
    
        queue_lockfree_total<int, trait_reclamation::hp> queue;
        
        size_t count;
        auto ret = queue.get();
        count = queue.size();
        CHECK( 0 == count );
        CHECK( !ret );
    
        queue_lockfree_total<int, trait_reclamation::hp>::thread_deinit();
    }
    SECTION( "multiple instances" ) {
    
        queue_lockfree_total<int, trait_reclamation::hp> q1,q2;
            
        CHECK( 0 == q1.size() );
        CHECK( 0 == q2.size() );
    
        q1.put(5);
        q2.put(6);

        CHECK( 1 == q1.size() );
        CHECK( 1 == q2.size() );
      
        auto b1 = q1.get();
        CHECK( 0 == q1.size() );
        auto b2 = q2.get();

        CHECK( b1 );
        CHECK( 5 == *b1 );
        CHECK( b2 );
        CHECK( 6 == *b2 );

        CHECK( 0 == q1.size() );
        CHECK( 0 == q2.size() );
    
        queue_lockfree_total<int, trait_reclamation::hp>::thread_deinit();
    }
}

TEST_CASE( "queue_lockfree_total_multithread", "[queue multithread]" ) { 
        
    SECTION( "multi-thread push-pop" ) {

        queue_lockfree_total<int, trait_reclamation::hp> queue;

        int count_loop = 10;
        while( --count_loop >=0 ){
            size_t count;
            unsigned int num_threads = 50;
            vector<thread> threads( num_threads );
            for( int i = 0; i < num_threads; ++i ){
                threads[i] = std::thread( [ &, i ](){
                        int val = i;
                        queue.put( val );
                        queue_lockfree_total<int, trait_reclamation::hp>::thread_deinit();
                    } );
            }
            // count = queue.size();
            // cout << "queue size after push threads started: " << count << endl;

            vector<thread> threads2( num_threads );
            set<int> vals_retrieve;
            for( int i = 0; i < num_threads * 0.1; ++i ){
                threads2[i] = std::thread( [&](){
                        auto ret = queue.get();
                        if( ret ){
                            // std::cout << pop_val << std::endl;
                        }
                        queue_lockfree_total<int, trait_reclamation::hp>::thread_deinit();
                    } );
            }
            for( int i = 0; i < num_threads * 0.1; ++i ){
                threads2[i].join();
            }
            for( auto & i : threads ){
                i.join();
            }
            count = queue.size();
      
            CHECK( ((num_threads * 0.9 ) * 0.8) <= count );
            CHECK( ((num_threads * 0.9 ) * 1.2) >= count );
      
            for( int i = 0; i < num_threads * 0.9; ++i ){
                threads2[i] = std::thread( [&](){
                        auto ret = queue.get();
                        if( ret ){
                            // std::cout << pop_val << std::endl;
                        }
                        queue_lockfree_total<int, trait_reclamation::hp>::thread_deinit();
                    } );
            }
            for( int i = 0; i < num_threads * 0.9; ++i ){
                threads2[i].join();
            }
            count = queue.size();
      
            CHECK( 0 <= count );
            CHECK( (num_threads * 0.1 ) >= count );
        }
        queue_lockfree_total<int, trait_reclamation::hp>::thread_deinit();
    }
}

TEST_CASE( "queue_lockfree_total_multithread_long_lived", "[queue multithread longlived]" ) { 
    
    queue_lockfree_total<int, trait_reclamation::hp> queue;

    unsigned int num_threads = std::thread::hardware_concurrency();
    
    size_t nums = num_threads * 3000000;
    
    vector<int> retrieved( nums, 0);

    vector<thread> threads( num_threads );
    vector<thread> threads2( num_threads );
    
    auto t0 = std::chrono::high_resolution_clock::now();

    // int sync = 0;
    // std::mutex m;
    // std::condition_variable cv;
    
    for( int i = 0; i < num_threads; ++i ){
        threads[i] = std::thread( [ &, i ](){
                int val = nums/num_threads*i;
                for( int j = 0; j < nums/num_threads; ++j ){
                    while( !queue.put( val + j ) ){} //force enqueue
                }
                
                // {
                //     std::scoped_lock<std::mutex> l(m);
                //     sync++;
                // }
                // cv.notify_all();
                // {
                //     std::unique_lock<std::mutex> l(m);
                //     cv.wait(l, [&](){return sync==2*num_threads;});
                // }
                
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

    CHECK( 0 == count );

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
    CHECK( 0 == k );  
}


TEST_CASE( "queue_locking_reference", "[queue locking reference]" ) { 
    
    vector<int> queue;

    unsigned int num_threads = std::thread::hardware_concurrency();
    
    size_t nums = num_threads * 3000000;

    vector<int> retrieve(nums, 0);

    vector<thread> threads( num_threads );
    vector<thread> threads2( num_threads );

    std::mutex m;

    auto t0 = std::chrono::high_resolution_clock::now();
    
    for( int i = 0; i < num_threads; ++i ){
        threads[i] = std::thread( [ &, i ](){
                int val = nums/num_threads*i;
                for( int j = 0; j < nums/num_threads; ++j ){
                    std::scoped_lock lock(m);
                    queue.push_back(val+j);
                }
            } );
    }

    for( int i = 0; i < num_threads; ++i ){
        threads2[i] = std::thread( [&](){
                for( int j = 0; j < nums/num_threads; ++j ){
                    while(1){
                        std::scoped_lock lock(m);
                        if(!queue.empty()){
                            int v = queue.back();
                            ++retrieve[v];
                            queue.pop_back();
                            break;
                        }
                    }
                }
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
    std::cout << "reference: duration: " << dur_ms.count() << " ms. rate: " <<  (double)nums/dur_ms.count()*1000.0 << " put-get/sec." << std::endl;

    assert(queue.size()==0);
}
