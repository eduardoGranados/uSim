/**
 *  node.h
 *  Class Node
 *
 **/

#ifndef NODE_H_
#define NODE_H_

#include "link.h"
#include "packet.h"
#include <vector>
#include <list>
#include <cstdlib>
#include <assert.h>

#include <string.h>

enum NodeType {
    SIMPLE_SRC,     // Nodo Simple que hace forward
    SIMPLE_END,     // Nodo Simple que recibe y consume
    APP,            // Nodo Aplicacion
    ROUTER,         // Nodo Router
    TRANSPORT       // Transport layer
};


class Node {

private:

    int node_id;
    NodeType type;
    unsigned receivedPackets;
    unsigned queue_size;
    // set of interfaces per node:
    std::vector <Link *> interfaces;
    std::list <Pdu *> queue;
    unsigned queue_capacity;
    // to make temporal disconnections of the node
    bool active;
    char name[30];

    Node()
    {
       // EMPTY CONSTRUCTOR PROHIBITED
    }

public:

    Node(int _id, NodeType _type, const char *_name, unsigned _qcap) :
        node_id(_id),
        type(_type),
        receivedPackets(0),
        queue_size(0),
        queue_capacity(_qcap),
        active(true)
    {
        set_name(_name);
        assert(queue_capacity>0);
    }

    int get_node_id() {
        return node_id;
    }

    NodeType get_node_type() {
        return type;
    }

    void increase_received_packets() {
        receivedPackets++;
    }

    void pdu_arrival(Pdu * _pdu)
    {
        if (queue_size == queue_capacity)
        {
            // TODO: Missing drop event.
            printf("ERROR: Droping a packet!");
            exit(1);
        }
        else
        {
            queue_size++;
            queue.push_back(_pdu);
        }
    }

    Pdu * pdu_departure()
    {
        if (queue_size == 0)
        {
            printf("ERROR: dequeueing an empty queue!");
            exit(1);
        }

        queue_size--;

        Pdu * _pdu = queue.front();

        queue.pop_front();

        return _pdu;
    }


    unsigned get_queue_size() {
        return queue_size;
    }

    unsigned get_received_packets() {
        return receivedPackets;
    }

    void add_interface(Link * link) {
        interfaces.push_back(link);
    }

    Link * get_interface(Node * targetNode) {

        Link * link = NULL;

        for (unsigned long i = 0; i < interfaces.size(); i++ ) {
            if (interfaces[i]->get_target_node() == targetNode) {
                link = interfaces[i];
                break;
            }
        }

        return link;
    }
    
    void set_name(const char *_name)
    {
        strcpy(name, _name);
    }
    
    const char *get_node_name()
    {
        return name;
    }
    
    bool get_activity() {
        return active;
    }

    void set_activity(bool _active) {
        active = _active;
    }

    ~Node() {}
};

#endif  // NODE_H
