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
    std::list <Pdu *> forward_queue;
    unsigned queue_capacity;
    // to make temporal disconnections of the node
    bool active;
    char name[30];
    // second queue, used only by Router node,  TODO: asign queues to interfaces.
    unsigned backward_queue_size;
    std::list <Pdu *> backward_queue;

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
        active(true),
        backward_queue_size(0)
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
            printf("ERROR: Droping a packet! pdu arr");
            exit(1);
        }
        else
        {
            queue_size++;
            forward_queue.push_back(_pdu);
        }
    }
    void ack_arrival(Pdu * _pdu)
    {

            backward_queue_size++;
            backward_queue.push_back(_pdu);

    }
    Pdu * pdu_departure()
    {
        if (queue_size == 0)
        {
            printf("ERROR: dequeueing an empty queue! pdu dep");
            exit(1);
        }

        queue_size--;

        Pdu * _pdu = forward_queue.front();

        forward_queue.pop_front();

        return _pdu;
    }
    Pdu * ack_departure()
    {

        backward_queue_size--;

        Pdu * _pdu = backward_queue.front();

        backward_queue.pop_front();

        return _pdu;
    }
    Pdu * get_front(){
        return forward_queue.front();
    }
    unsigned get_queue_size() {
        return queue_size;
    }
    unsigned get_backward_queue_size() {
        return backward_queue_size;
    }
    unsigned get_received_packets() {
        return receivedPackets;
    }
    unsigned get_queue_capacity() {
        return queue_capacity;
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

    void insert_in_front(Pdu * _pdu)
    {
        if (queue_size == queue_capacity)
        {
            // TODO: Missing drop event.
            printf("ERROR: Droping a packet! pdu in front");
            exit(1);
        }
        else
        {
            queue_size++;
            std::list<Pdu *>::iterator it = forward_queue.begin();
            forward_queue.insert(it,_pdu);
        }
    }
    ~Node() {}
};

#endif  // NODE_H
