#-*-coding:UTF-8-*-


'''
* @Author      : kevin.z.y <kevin.cn.zhengyang@gmail.com>
* @Date        : 2024-05-01 10:38:43
* @LastEditors : kevin.z.y <kevin.cn.zhengyang@gmail.com>
* @LastEditTime: 2024-05-01 11:19:45
* @FilePath    : /OpenSmartHome/tools/node_state.py
* @Description :
* @Copyright (c) 2024 by Zheng, Yang, All Rights Reserved.
'''


from graphviz import Digraph

# Digraph
dot = Digraph()

# Nodes
dot.attr('node', shape='doublecircle')
dot.node("init", "OSH_FSM_STATE_INIT")
dot.node("idle", "OSH_FSM_STATE_IDLE")
dot.node("ongo", "OSH_FSM_STATE_ONGOING")
dot.node("save", "OSH_FSM_STATE_SAVING")
dot.node("upgrade", "OSH_FSM_STATE_UPGRADING")

# Edges
dot.edge("init", "init", label="power on")
dot.edge("init", "idle", label="connect")

dot.edge("idle", "init", label="disconnect")
dot.edge("idle", "save", label="timeout T_SAVE")
dot.edge("idle", "ongo", label="request or invoke")
dot.edge("idle", "upgrade", label="update")

dot.edge("ongo", "idle", label="timeout T_IDLE")
dot.edge("ongo", "ongo", label="request or invoke")
dot.edge("ongo", "init", label="disconnect")
dot.edge("ongo", "upgrade", label="update")

dot.edge("upgrade", "upgrade", label="download")
dot.edge("upgrade", "init", label="complete")
dot.edge("upgrade", "init", label="rollback")

dot.edge("save", "idle", label="timeout T_WAKEUP")

# Render and Save
dot.render("../components/osh_node/images/node_fsm", format="svg", cleanup=True)

