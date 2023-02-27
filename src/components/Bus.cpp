//
// Created by golas on 23.2.23.
//

#include "components/Bus.h"

Bus::Bus(int addrWidth, int dataWidth)
    : m_addrWidth(addrWidth),
      m_dataWidth(dataWidth) {

//    m_dataConnectors["access"] = {
//        .read = [&](){
//
//            for(auto & device : m_dataPorts) {
//                device.second()
//            }
//        },
//        .write = [](){
//
//        }
//    };
}

