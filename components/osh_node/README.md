<!--
 * @Author: Zheng, Yang kevin.cn.zhengyang@gmail.com
 * @Date: 2024-04-29 23:10:16
 * @LastEditors: Zheng, Yang kevin.cn.zhengyang@gmail.com
 * @LastEditTime: 2024-05-01 11:20:02
 * @FilePath: /OpenSmartHome/components/osh_node/README.md
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
-->
# Open Smart Home Node

the device node. using BLE or WiFi as the node transport, provide the
capability to be found and communicated with other nodes.

# Diagram

>> diagram of circuit

a RGB led to indicate the working status


# States

![FSM of node](images/node_fsm.svg)

# Transport

Two transports can be used:
- BLE: to be implemented
- WiFi: IPV4 and UDP would be adopt when choosing WiFi

choose any one of above in menuconfig before compiling this project

# COAP

[COAP](https://en.wikipedia.org/wiki/Constrained_Application_Protocol) used to transfer all data exchanged among nodes.

Those data would be:

- applicational service data: to be applied in users' application
- operational service data: to be applied in supervision or maintenance of the node


``` C

#ifndef OSH_NODE_
#define OSH_NODE_

#ifdef __cplusplus
extern "C" {
#endif

#include "osh_node_comm.h"
#include "osh_node_events.h"
#include "osh_node_errors.h"




#ifdef __cplusplus
}
#endif

#endif /* OSH_NODE_ */

```