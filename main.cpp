//#include <QCoreApplication>

//
//  main.cpp
//  usim
//
//  Created by Andres Arcia on 6/20/12.
//  Copyright 2012 __RESIDE__. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstdio>

#define TSIM       2
#define PACKETSIZE 1000
#define APP_RATE   4000000 // it was 400000 for every 0.1 secs
#define BOTTLENECK 500000

#include "scheduler.h"
#include "node.h"
#include "packet.h"
#include "namgenerator.h"

using namespace std;

ListScheduler usim_sched;         //Lista de eventos pendientes
unsigned msg_id=0;                //TODO: Later to be included in an app class...
unsigned seqno=0;
unsigned init_cwnd = 1;

double calc_interval(double in_rate, double packet_size);
void start_app(unsigned int rate, Node * app_node, Node * src_comp_node);
void service(Node * from, Node * dst);
void propagation(Node * from, Node * dst);

//
// returns the separation between a pair of packets in seconds
// to reach the planned rate
// *rate* is in bit per seconds
// *packet size* is in bytes
// [P1]---[P2]---[P3] ... [PN]
//
double calc_interval(double in_rate, double packet_size)
{
    double pkt_per_sec = (in_rate/8.0)/packet_size;

    double interval = 1.0/pkt_per_sec;

    return interval;
}


//
// kicks off the application
//

void start_app(unsigned int rate, Node * app_node, Node * nxt_node) {

    Event * event;

    int msg_size = PACKETSIZE*5;

    msg_id++;

    if (nxt_node->get_node_type()==SIMPLE_SRC)
    {
        event = new Event(PKT_ARRIVAL_TO_QUEUE,
                                  static_cast<Pdu *>(new Message(msg_id, msg_size)),
                                  app_node,
                                  nxt_node);
    }
    else if (nxt_node->get_node_type()==TRANSPORT)
    {
        event = new Event(PKT_ARRIVAL_TO_TRANSPORT,
                                  static_cast<Pdu *>(new Message(msg_id, msg_size)),
                                  app_node,
                                  nxt_node);
    }
    else
    {
        printf("ERROR: Not know next node for APP nodes\n");
        exit(1);
    }

    // start calculation for certain rate

    event->set_timestamp(usim_sched.get_curr_time() + calc_interval(rate, msg_size));

    // schedule the event
    usim_sched.insert(event);
}


int main()
{

    int lastCwnd=1;
        int cwnd=0;             //congestion window
        int segtsSent=0;        //number of segments sent this RTT = number of ACKs needed to recieve, in order to keep sending segments.
        int dataSegtsCount=0;   //overall data segments sent counter.
        int acksCount=0;        //overall acks sent counter.



    bool first_iteration = true;
    //namfile
    char *namInstruction =new char[100];
    NamGenerator::Inst(const_cast<char *>("usim.nam"));
    NamGenerator::Inst()->addEvent("V -t * -v 1.0a5 -a 0");
    NamGenerator::Inst()->addEvent("n -t * -a 0 -s 0 -S UP -v circle -c black -i black");
    NamGenerator::Inst()->addEvent("n -t * -a 1 -s 1 -S UP -v circle -c black -i black");

    Event * current_event;

    Node * a = new Node(0, APP, "app in src", 10000);
    Node * b = new Node(1, SIMPLE_SRC, "src iface", 200000);
    Node * c = new Node(2, SIMPLE_END, "dst iface", 200000);
    Node * tr_b = new Node(3, TRANSPORT, "<transport layer in src>", 50000);
    Node * tr_c = new Node(4, TRANSPORT, "<transport layer in dst>", 50000);

    vector <Node *> nodos;

    nodos.push_back(a);
    nodos.push_back(b);
    nodos.push_back(c);
    nodos.push_back(tr_b);
    nodos.push_back(tr_c);

    // APP to TRANSPORT (just communication to put messages into transport layer)
    Link * a_to_tr_b = new Link(true, 99999999.0, 0.0001, a, tr_b, LINK_STACK);
    a->add_interface(a_to_tr_b); // connect the application to the transport layer

    // TRASNPORT TO SIMPLE (just communication to put messages into the queue of the interface)
    Link * tr_b_to_b = new Link(true, 99999999.0, 0.0001, tr_b, b, LINK_STACK);
    tr_b->add_interface(tr_b_to_b); // connect the application to the transport layer

    // SIMPLE to SIMPLE (real "link" in the middle)
    Link * bc = new Link(true, BOTTLENECK, 0.10, b, c, LINK_NETWORK);
    b->add_interface(bc);     // connect the interface (the link) to source node.

    // TRASNPORT TO SIMPLE (just communication to put messages into the queue of the interface)
    Link * c_to_tr_c = new Link(true, 99999999.0, 0.0001, c, tr_c, LINK_STACK);
    c->add_interface(c_to_tr_c); // connect the application to the transport layer

    Link * tr_c_to_tr_b = new Link(true, BOTTLENECK, 0.10, tr_c, tr_b, LINK_NETWORK);
    tr_c->add_interface(tr_c_to_tr_b); // connect the application to the transport layer
    vector <Link *> links;

    links.push_back(a_to_tr_b);
    links.push_back(tr_b_to_b);
    links.push_back(bc);
    links.push_back(c_to_tr_c);
    links.push_back(tr_c_to_tr_b);

    sprintf(namInstruction,"l -t * -s 0 -d 1 -S UP -r %i -D %.4f -c black -o 0deg", BOTTLENECK,0.10);
    NamGenerator::Inst()->addEvent(namInstruction);
    NamGenerator::Inst()->addEvent("q -t * -s 1 -d 0 -a 0.5");
    NamGenerator::Inst()->addEvent("q -t * -s 0 -d 1 -a 0.5");


    // THIS INTERFACE IS BAD! I cannot infringe the destination node...
    //start_app(APP_RATE, a, b); <<-- TO MODIFY

    start_app(APP_RATE, a, tr_b);

    // statistics?

    current_event = usim_sched.dispatch();


    //****************************************************


    cout << "\nTIME\tEVENT\tSRC\tDST" << endl;

    // there should be a application interaction with the scheduler. That is, a first trigger
    // for the application to start sending packets with a first delay. Then, at the planning of the
    // next packet, to plan the following packet and so on.
    // First trigger (T=0) ------> P1 @ T1 --> P2 @ T2 --> ... -> Pn @ Tn.

    /*  Prueba realizada sin tomar en cuenta el buffer de salida de los nodos   */
    while (usim_sched.get_curr_time() <= TSIM) {
        // packet arrival to the node from the application (at a certain rate)
        // this is the input rate, what may cause queuing delay

        if (current_event->get_from_node()->get_node_type() == APP &&
                current_event->get_type() == PKT_ARRIVAL_TO_TRANSPORT)
        {
            // Receive the message at transport level
            Message *msg = static_cast<Message*>(current_event->get_assoc_pdu());

            unsigned msg_size = msg->get_pdu_size();

            Tcp_segment *prepared_segment = NULL;

            while (msg_size > 0)
            {
                msg_size -= PACKETSIZE;

                prepared_segment = new Tcp_segment(PACKETSIZE, seqno);
                seqno++;
                current_event->get_to_node()->pdu_arrival(prepared_segment);

                //printf("%.4f\t\t\tencolo\t\t\t%d\n", usim_sched.get_curr_time(), 0);
            }

            //Sending first segments
            if(first_iteration)
            {
                for(int i=1; i<=init_cwnd; i++)
                {
                    if(current_event->get_to_node()->get_queue_size() > 0)
                    {
                        sprintf(namInstruction,"+ -t %f -e %i -s 0 -d 1 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                        NamGenerator::Inst()->addEvent(namInstruction);
                        usim_sched.plan_service(PKT_DEPARTURE_FROM_TRANSPORT,
                                                                current_event->get_to_node()->pdu_departure(),
                                                                current_event->get_to_node(),
                                                                b,
                                                                i);
                    }

                }
                first_iteration=false;
            }

            // Recall start_app to keep a rate
            start_app(APP_RATE, a, tr_b);
          //printf("%.4f\t\t\ttamano de cola\t\t\t%u\n", usim_sched.get_curr_time(), current_event->get_to_node()->get_queue_size());
            // Liberate memory from original message

        }
        if (current_event->get_from_node()->get_node_type() == TRANSPORT &&
                current_event->get_type() == ACK_ARRIVAL_TO_TRANSPORT)
        {

            cwnd++;
            segtsSent++;
            if(segtsSent==lastCwnd){  // 1 RTT
                lastCwnd=cwnd;
                segtsSent=0;
                printf("RTT\n");
            }

            //Adding event to Nam file
            sprintf(namInstruction,"r -t %f -e %i -s 1 -d 0 -c 1 -i 1",usim_sched.get_curr_time(), 54);
            NamGenerator::Inst()->addEvent(namInstruction);

            for(int i=1; i<=2; i++)
            {
                NamGenerator::Inst()->addEvent(namInstruction);
                sprintf(namInstruction,"+ -t %f -e %i -s 0 -d 1 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                usim_sched.plan_service(PKT_DEPARTURE_FROM_TRANSPORT,
                                                        current_event->get_to_node()->pdu_departure(),
                                                        current_event->get_to_node(),
                                                        b,
                                                        i);
            }
            printf("%.20f\tACK-ARRiv \t%d\t%d\n",
                   usim_sched.get_curr_time(),
                   current_event->get_from_node()->get_node_id(),
                    (static_cast<Tcp_segment*>(current_event->get_assoc_pdu()))->get_seqno());
        }

        if (current_event->get_from_node()->get_node_type() == TRANSPORT &&
                current_event->get_type() == PKT_DEPARTURE_FROM_TRANSPORT)
        {
            // put packets in the queue of a simple_src (to_node)
            current_event->get_to_node()->pdu_arrival(current_event->get_assoc_pdu());

            // plan departure of the packet
            usim_sched.plan_service(PKT_DEPARTURE_FROM_NODE,
                                    current_event->get_assoc_pdu(),
                                    current_event->get_to_node(),
                                    c);

            // output simulation trace
            printf("%.20f\tARR-FROM-TRNSP\t%i\t%i\n",
                   usim_sched.get_curr_time(),
                   current_event->get_from_node()->get_node_id(),
//                   current_event->get_to_node()->get_node_id());
                   (static_cast<Tcp_segment*>(current_event->get_assoc_pdu()))->get_seqno());

        }


        if (current_event->get_from_node()->get_node_type() == SIMPLE_SRC &&
                current_event->get_type() == PKT_DEPARTURE_FROM_NODE)
        {

            // dequeueing of the packet (from_node)
            Pdu * _pdu = current_event->get_from_node()->pdu_departure();
            // plan propagation of the packet
            usim_sched.plan_propagation(_pdu, current_event->get_from_node(), c);

            // output simulation trace
            printf("%.20f\tDEP-FROM-SRC\t%i\t%i\n",
                   usim_sched.get_curr_time(),
                   current_event->get_from_node()->get_node_id(),
                   //current_event->get_to_node()->get_node_id());
                   (static_cast<Tcp_segment*>(current_event->get_assoc_pdu()))->get_seqno());
        }

        // this packet is arriving at destination

        if (current_event->get_from_node()->get_node_type() == SIMPLE_SRC &&
                current_event->get_type()== PKT_ARRIVAL_TO_QUEUE)
        {
            if (current_event->get_to_node()->get_node_type() == SIMPLE_END)
            {
                // just consume the packet
                current_event->get_to_node()->increase_received_packets();

                //Adding event to Nam file
                sprintf(namInstruction,"r -t %f -e %i -s 0 -d 1 -c 1 -i 1",usim_sched.get_curr_time(), PACKETSIZE);
                NamGenerator::Inst()->addEvent(namInstruction);

                // output simulation trace
                printf("%.20f\tARR-TO-DEST\t%d\t%d\n",
                       usim_sched.get_curr_time(),
                       current_event->get_from_node()->get_node_id(),
                        (static_cast<Tcp_segment*>(current_event->get_assoc_pdu()))->get_seqno());
//                       acknowledged_segment->get_seqno());

                //sending to transport layer
                usim_sched.plan_service(PKT_ARRIVAL_TO_TRANSPORT,
                                                        current_event->get_assoc_pdu(),
                                                        current_event->get_to_node(),
                                                        tr_c);
                // liberate current packet
                delete current_event->get_assoc_pdu();
            }
            else
                printf("ERROR: unknown event\n");
        }


        if (current_event->get_from_node()->get_node_type() == SIMPLE_END &&
                current_event->get_type() == PKT_ARRIVAL_TO_TRANSPORT)
        {
            Tcp_segment * acknowledged_segment =  static_cast<Tcp_segment *>(current_event->get_assoc_pdu());
            Ack *prepared_ack = new Ack(54, acknowledged_segment->get_seqno());
            usim_sched.plan_service(ACK_DEPARTURE_FROM_TRANSPORT,
                                                    prepared_ack,
                                                    current_event->get_to_node(),
                                                    tr_b);
            //Adding event to Nam file
            sprintf(namInstruction,"h -t %f -e %i -s 1 -d 0 -c 1 -i 1",usim_sched.get_curr_time(), 54);
            NamGenerator::Inst()->addEvent(namInstruction);

            printf("%.20f\tACK-CREAT\t%d\t%d\n",
                   usim_sched.get_curr_time(),
                   current_event->get_from_node()->get_node_id(),
                    (static_cast<Tcp_segment*>(current_event->get_assoc_pdu()))->get_seqno());

        }
        if (current_event->get_from_node()->get_node_type() == TRANSPORT &&
                current_event->get_type() == ACK_DEPARTURE_FROM_TRANSPORT)
        {
            Pdu * _pdu = current_event->get_assoc_pdu();
            usim_sched.plan_propagation(_pdu, current_event->get_from_node(), tr_b, ACK_ARRIVAL_TO_TRANSPORT);

            printf("%.20f\tACK-TO-TRANS\t%d\t%d\n",
                   usim_sched.get_curr_time(),
                   current_event->get_from_node()->get_node_id(),
                    (static_cast<Tcp_segment*>(current_event->get_assoc_pdu()))->get_seqno());
        }


        delete current_event;

        current_event = usim_sched.dispatch();

    }


    cout << "\nPaquetes recibidos Nodo final: " << c->get_received_packets() << endl;

    return 0;
}
