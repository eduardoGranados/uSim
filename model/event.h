/**
 *  event.h
 *  Clase Evento
 *
 **/

#ifndef EVENT_H
#define EVENT_H

#include "node.h"
#include "packet.h"

enum EVENT_TYPE {
    PKT_ARRIVAL_TO_QUEUE,
    PKT_ARRIVAL_TO_SERVICE,
    PKT_DEPARTURE_FROM_NODE,
    PKT_DEPARTURE_FROM_APP,
    PKT_ARRIVAL_TO_TRANSPORT,
    PKT_DEPARTURE_FROM_TRANSPORT,
    PKT_DEPARTURE_FROM_ROUTER,
    ACK_DEPARTURE_FROM_TRANSPORT,
    ACK_ARRIVAL_TO_TRANSPORT,
    ACK_DEPARTURE_FROM_NODE,
    ACK_ARRIVAL_TO_ROUTER,
    ACK_DEPARTURE_FROM_ROUTER,
    ACK_ARRIVAL_TO_SRC,
    LAST_EVENT
};


class Event {

    EVENT_TYPE type;
    Pdu * assoc_pdu;
    Node * from_node;
    Node * to_node;

    double timestamp;

public:
        
    Event(EVENT_TYPE _type, Pdu * _assoc_pdu, Node * _from_node, Node * _to_node) :
        type(_type),
        assoc_pdu(_assoc_pdu),
        from_node(_from_node),
        to_node(_to_node)
    {
        // EMTPTY
    }

    Event()
    {
        type = LAST_EVENT;
        from_node = to_node = NULL;
        assoc_pdu = NULL;
    }

    void set_timestamp(double _ts)
    {
        timestamp = _ts;
    }

    double get_timestamp()
    {
        return timestamp;
    }

    EVENT_TYPE get_type()
    {
        return type;
    }

    Node * get_from_node()
    {
        return from_node;
    }

    Node * get_to_node()
    {
        return to_node;
    }

    Pdu * get_assoc_pdu()
    {
        return assoc_pdu;
    }

    ~Event()
    {
        // EMTPY
    }

};

#endif  // EVENT_H
