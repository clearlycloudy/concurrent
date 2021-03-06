#define CATCH_CONFIG_MAIN

#include <vector>
#include <thread>
#include <iostream>
#include <mutex>
#include <cstdint>
#include <cassert>
#include <random>
#include <iostream>

#include "reclam_hazard.hpp"

#include "catch.hpp"


struct Data {
    uint64_t p;
    uint64_t q;
    Data(){
        p = 0;
        q = 0;
    }
};

TEST_CASE( "hazard pointer", "[hazard]" ) {

    int nums = 20000;
    std::vector<Data*> temp;
    for(int i = 0; i < nums; ++i ){
        temp.push_back(new Data());
    }

    int numthread = 4;
  
    auto f = [&temp,numthread,nums](int id){

        CHECK(nullptr == reclam_hazard<Data>::new_from_recycled());

        int offset = nums/numthread*id;
        for(int j = 0; j < nums/numthread/20; ++j){
            //craete some scope guards
            hazard_guard<Data> g1 = reclam_hazard<Data>::add_hazard(temp[offset+j*20]);
            hazard_guard<Data> g2 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+1]);
            hazard_guard<Data> g3 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+2]);
            hazard_guard<Data> g4 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+3]);
            hazard_guard<Data> g5 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+4]);
            hazard_guard<Data> g6 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+5]);
            hazard_guard<Data> g7 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+6]);
            hazard_guard<Data> g8 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+7]);
            hazard_guard<Data> g9 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+8]);
            hazard_guard<Data> g10 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+9]);
            hazard_guard<Data> g11 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+10]);
            hazard_guard<Data> g12 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+11]);
            hazard_guard<Data> g13 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+12]);
            hazard_guard<Data> g14 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+13]);
            hazard_guard<Data> g15 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+14]);
            hazard_guard<Data> g16 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+15]);
            hazard_guard<Data> g17 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+16]);
            hazard_guard<Data> g18 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+17]);
            hazard_guard<Data> g19 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+18]);
            hazard_guard<Data> g20 = reclam_hazard<Data>::add_hazard(temp[offset+j*20+19]);
        }

        CHECK(nullptr == reclam_hazard<Data>::new_from_recycled());

        bool is_there_recycled = false;
        for(int j = nums/numthread*id; j < nums/numthread*id+nums/numthread; ++j){
      
            reclam_hazard<Data>::retire_hazard(temp[j]);

            //check some recycling sweeps were done
            if(!is_there_recycled && (j%2==1)){
                auto ret = reclam_hazard<Data>::new_from_recycled();
                if( ret ){
                    delete ret;
                    is_there_recycled = true;
                }
            }
        }

        // CHECK( is_there_recycled );
    
        //thread cleanup
        reclam_hazard<Data>::thread_deinit();
    };

    std::vector<std::thread> th;
  
    for( int i = 0; i < numthread; ++i ){
        th.push_back( std::thread(f, i) );
    }
  
    for( auto & i: th ){
        i.join();
    }

    //test double deinit
    reclam_hazard<Data>::thread_deinit();
    //reclam_hazard<Data>::thread_deinit();
  
    //final cleanup
    reclam_hazard<Data>::final_deinit();

    //there should no memory leak
}

TEST_CASE( "hazard pointer stress", "[hazard]" ) {

    int nums = 1000;
    int numthread = 4;

    std::vector<int> count_recycled(4,0);
    
    auto f = [nums,&count_recycled](int id){

        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> dis(1, 20);

        int recycled = 0;
    
        CHECK(nullptr == reclam_hazard<Data>::new_from_recycled());
        for(int i = 0; i < nums; ++i ){

            int c = dis(gen);

            std::vector<hazard_guard<Data>> guards;
        
            for(int j = 0; j < c; ++j){
                Data * d = reclam_hazard<Data>::new_from_recycled();
                if(nullptr==d){
                    d = new Data();
                }else{
                    ++recycled;
                }
                ++d->q;
                guards.push_back(reclam_hazard<Data>::add_hazard(d));

                reclam_hazard<Data>::retire_hazard(d);
            }
        }
    
        //thread cleanup
        reclam_hazard<Data>::thread_deinit();

        count_recycled[id]=recycled;
    };

    std::vector<std::thread> th;
  
    for( int i = 0; i < numthread; ++i ){
        th.push_back( std::thread(f, i) );
    }
  
    for( auto & i: th ){
        i.join();
    }

    //test double deinit
    reclam_hazard<Data>::thread_deinit();
  
    //final cleanup
    reclam_hazard<Data>::final_deinit();

    std::cout << "count recycled: ";
    for(auto i: count_recycled ){
        std::cout << i << " ";
    }
    std::cout<<std::endl;
    
    //there should no memory leak
}
