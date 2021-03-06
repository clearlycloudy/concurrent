#include <iostream>

template<class T>
thread_local std::atomic<uint64_t> reclam_epoch<T>::epoch_local{std::numeric_limits<uint64_t>::max()};

template<class T>
std::atomic<uint64_t> reclam_epoch<T>::freq_epoch{0};

template<class T>
thread_local int reclam_epoch<T>::local_freq_recycle{0};

template<class T>
thread_local std::vector<std::pair<T*,uint64_t>> reclam_epoch<T>::local_recycle {};

template<class T>
thread_local int reclam_epoch<T>::count_recycled{0};

template<class T>
std::atomic<uint64_t> reclam_epoch<T>::epoch_global{0};

template<class T>
queue_lockfree_simple<std::atomic<uint64_t>*> reclam_epoch<T>::epoch_list;

template<class T>
void reclam_epoch<T>::retire(T * resource){
    
    auto e = epoch_global.load(std::memory_order_acquire);
    
    epoch_local.store(e, std::memory_order_relaxed);

    local_recycle.push_back({resource, e});

    if(freq_epoch.fetch_add(1) % FREQ_EPOCH == 0){
        epoch_global.fetch_add(1);
    }

    if(++local_freq_recycle >= FREQ_RECYCLE){
        local_freq_recycle = 0;
        recycle();
    }
}

template<class T>
typename reclam_epoch<T>::epoch_guard reclam_epoch<T>::critical_section(){    
    uint8_t e = epoch_global.load(std::memory_order_acquire);
    epoch_local.store(e, std::memory_order_relaxed);
    return epoch_guard( &epoch_local );
}

template<class T>
void reclam_epoch<T>::recycle(){
    
    //check other threads' epochs and attempt reycle
    uint64_t low = std::numeric_limits<uint64_t>::max();

    auto fn = [&](queue_lockfree_simple<std::atomic<uint64_t>*>::Node * i){
        if(i && i->_val){
            auto v = i->_val->load(std::memory_order_acquire);
            low = std::min(low, v);
        }
    };

    epoch_list.for_each(fn);
        
    std::vector<std::pair<T*, uint64_t>> temp;
    for(auto &[resource, epoch]: local_recycle){
        if(epoch < low && resource){
            delete resource;
            ++count_recycled;
        }else{
            temp.push_back({resource,epoch});
        }
    }

    swap(temp, local_recycle);
}

template<class T>
void reclam_epoch<T>::recycle_final(){
    
    for(auto &[resource, epoch]: local_recycle){
        if(resource){
            delete resource;
        }
    }
    
    local_recycle.clear();
}

template<class T>
void reclam_epoch<T>::register_thread(){
    using NodeType = queue_lockfree_simple<std::atomic<uint64_t>*>::Node;
    epoch_list.push_back( new NodeType(&epoch_local) );
}

template<class T>
void reclam_epoch<T>::unregister_thread(){
    //todo: item search and deletion from epoch_list

    using NodeType = queue_lockfree_simple<std::atomic<uint64_t>*>::Node;
    
    auto fn = [&](NodeType * const i){
        if(i->_val == &epoch_local){
            i->_val = nullptr;
        }
    };
    epoch_list.for_each(fn);
}

template<class T>
int reclam_epoch<T>::sync(){
    // //todo.. wait for all threads' epochs to become equal
    // while(true){
    //     auto e = epoch_global.load();
    //     epoch_local.store(e);

    //     bool valid = true;
    //     auto fn = [&](auto i){
    //         if(i != &epoch_local){
    //             auto v = i->load();
    //             if(v!=std::numeric_limits<uint64_t>::max() && v!=e){
    //                 valid = false;
    //             }
    //         }
    //     };

    //     epoch_list.for_each(fn);
        
    //     if(valid)
    //         return e;
    // }
    // assert(false); //should not come here
    assert(false && "unsupported");
    return -1;
}

template<class T>
void reclam_epoch<T>::deinit_thread(){
    ///assumes threads have finished
    recycle_final();
}

template<class T>
void reclam_epoch<T>::stat(){
    std::cout << "recycled: " << count_recycled << ", in queue: " << local_recycle.size() << std::endl;
}

template<class T>
void reclam_epoch<T>::clear_epoch_list(){
    epoch_list.clear();
}
