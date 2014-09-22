#ifndef USIM_H
#define USIM_H

#include <stdio.h>
#include <iostream>
#include <time.h>
#include <string>
#include <sstream>
#include <QDebug>
#include <stdlib.h>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstdio>

#include "model/scheduler.h"
#include "model/node.h"
#include "model/packet.h"


using namespace std;

class USim
{
private:
    //uSim variables
    float TSIM;
    int APP_RATE;
    int BOTTLENECK;
    int BOTTLENECK2;    //BW from router to dst
    int DIVACKS;
    int SSTHRESH;       //segments
    int BUFFERSIZE;
    float DELAY1;
    float DELAY2;
    float TIMEOUT;      //seconds
    float DELTA;
    //************

    QString log;                    //gui's log
    ListScheduler usim_sched;       //event list
    unsigned msg_id;                //TODO: Later to be included in an app class...
    unsigned seqno;
    unsigned init_cwnd;
    int sst;                        //used by getter, we only want to give the original value, getter used to re run saved experiment
    vector <Pdu *> unacknowledged_packets;
    vector <Pdu *> received_packets;



    //TODO: if there is a segment with id > requested_pdu, this one should be acknowledged.
    bool acknowledge(int acked_pdu)
    {

        if(acked_pdu == -1){//is a divack
            return true;
        }


        for(std::vector<Pdu *>::iterator it = unacknowledged_packets.begin(); it != unacknowledged_packets.end(); ++it)
        {
            //cout<<static_cast<Tcp_segment *>(*it)->get_seqno()<<endl;
            if(static_cast<Tcp_segment *>(*it)->get_seqno() < (acked_pdu - 1) )
            {
                unacknowledged_packets.erase(it);

                if(unacknowledged_packets.empty()){return true;}
            }
            //we are comparing only <Tcp_segment> at the moment
             if(static_cast<Tcp_segment *>(*it)->get_seqno() == (acked_pdu - 1))  // - 1 = ack n acknowledges segment (n-1)
            {
                //acknowledged

                unacknowledged_packets.erase(it);
                return true;
            }
        }
        return false;
    }


    //
    // returns the separation between a pair of packets in seconds
    // to reach the planned rate
    // *rate* is in bit per seconds
    // *packet size* is in bytes
    // [P1]---[P2]---[P3] ... [PN]

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

public:
    USim (){
        TSIM=0;
        APP_RATE=0;
        BOTTLENECK=0;
        BOTTLENECK2=0;
        DIVACKS=0;
        SSTHRESH=0;
        BUFFERSIZE=0;
        DELAY1=0;
        DELAY2=0;
        TIMEOUT=0;
        DELTA=0;
    }

    USim (float tsim, int ar, int bw1, int bw2, int da, int sst, int bfs,float d1,float d2,float to, float del):
        TSIM(tsim),
        APP_RATE(ar),
        BOTTLENECK(bw1),
        BOTTLENECK2(bw2),
        DIVACKS(da),
        SSTHRESH(sst),
        BUFFERSIZE(bfs),
        DELAY1(d1),
        DELAY2(d2),
        TIMEOUT(to),
        DELTA(del)
    {
        msg_id=0;
        seqno=0;
        init_cwnd = 1;
        log = "";
    }

    ~USim(){}

    int run_sim()
    {
        sst=SSTHRESH;       //used by getter, we only want to give the original value, getter used to rerun saved experiment

        // files used by gnuplot
        FILE *cwnd_trace;
        cwnd_trace=fopen("temp/cwnd_trace.dat","w");
        fprintf(cwnd_trace,"0 %i\n",init_cwnd);

        FILE *ssthresh;
        ssthresh=fopen("temp/ssthresh_trace.dat","w");
        fprintf(ssthresh,"0 %i\n",SSTHRESH);

        float gp_counter = 0;

        FILE *gp_trace;
        gp_trace=fopen("temp/gp_trace.dat","w");
        fprintf(gp_trace,"0 %f\n",gp_counter);

        float tp_counter = 0;

        FILE *tp_trace;
        tp_trace=fopen("temp/tp_trace.dat","w");
        fprintf(tp_trace,"0 %f\n",tp_counter);

        //sliding window variables, Used by transport layer
        float sliding_window = 0;
        int last_ack_sent = -1;
        int dupAck_counter = 0;
        int current_ack;
        int lost_seqno;

        int discard_ack=0;
        bool allow_divacks= true;         //used to know if the divacks are accepted by TCP src

        int expected_seqno = 0;   // dst side transport layer


        //TIMEOUT
        float timed_out = 0;       // when, is going to be a timeout
        //********

        Pdu * sending_pdu;
        float last_cwnd = 0;
        float cwnd=0;             //congestion window
        bool inSS=true;        //is in fast recovery phase? bool
        bool inFR=false;        //is in fast recovery phase? bool
        bool inCA=false;        //is in CA phase? bool
        bool fromSS=true;       //comes from ss, used in recovery
        bool first_iteration = true;
        //Statistics
        int  app_counter =0;
        int timeout_counter = 0;
        int loss_counter = 0;
        int resend_counter = 0;

        //To iterate pdu vectors...TODO: use while instead of for to iterate
        std::vector<Pdu *>::iterator it;

        //namfile
        char *namInstruction =new char[100];
        NamGenerator::Inst(const_cast<char *>("temp/usim.nam"));
        NamGenerator::Inst()->addEvent("V -t * -v 1.0a5 -a 0");
        NamGenerator::Inst()->addEvent("n -t * -a 0 -s 0 -S UP -v circle -c black -i black");
        NamGenerator::Inst()->addEvent("n -t * -a 1 -s 1 -S UP -v box -c tan -i tan");
        NamGenerator::Inst()->addEvent("n -t * -a 2 -s 2 -S UP -v circle -c black -i black");

        Event * current_event;

        Node * a = new Node(0, APP, "app in src", 2147483647); //TODO: change to -1 to set it equal to inf
        Node * b = new Node(1, SIMPLE_SRC, "src iface", 2147483647);
        Node * c = new Node(2, SIMPLE_END, "dst iface", 2147483647);
        Node * tr_b = new Node(3, TRANSPORT, "<transport layer in src>", 2147483647);
        Node * tr_c = new Node(4, TRANSPORT, "<transport layer in dst>", 2147483647);
        Node * r = new Node(5, ROUTER, "router", BUFFERSIZE);

        vector <Node *> nodos;

        nodos.push_back(a);
        nodos.push_back(b);
        nodos.push_back(c);
        nodos.push_back(tr_b);
        nodos.push_back(tr_c);

        // APP to TRANSPORT (just communication to put messages into transport layer)
        Link * a_to_tr_b = new Link(true, -1, 0.001, a, tr_b, LINK_STACK);
        a->add_interface(a_to_tr_b); // connect the application to the transport layer

        // TRASNPORT TO SIMPLE (just communication to put messages into the queue of the interface)
        Link * tr_b_to_b = new Link(true, -1, 0.001, tr_b, b, LINK_STACK);
        tr_b->add_interface(tr_b_to_b); // connect the application to the transport layer

        Link * b_to_tr_b = new Link(true, -1, 0.001, b, tr_b, LINK_STACK);
        b->add_interface(b_to_tr_b);

        // SIMPLE to SIMPLE (real "link" in the middle), not used at the moment
        Link * bc = new Link(true, BOTTLENECK, 0.1, b, c, LINK_NETWORK);
        b->add_interface(bc);     // connect the interface (the link) to source node.

        //transport to transport, not used
        Link * tr_c_to_tr_b = new Link(true, BOTTLENECK, 0.1, tr_c, tr_b, LINK_NETWORK);
        tr_c->add_interface(tr_c_to_tr_b);

        Link * b_to_r = new Link(true, BOTTLENECK, DELAY1, b, r, LINK_NETWORK);
        b->add_interface(b_to_r);

        Link * r_to_b = new Link(true, BOTTLENECK, DELAY1, r, b, LINK_NETWORK);
        r->add_interface(r_to_b);

        Link * c_to_r = new Link(true, BOTTLENECK2, DELAY2, c, r, LINK_NETWORK);
        c->add_interface(c_to_r);

        Link * r_to_c = new Link(true, BOTTLENECK2, DELAY2, r, c, LINK_NETWORK);
        r->add_interface(r_to_c);

        Link * c_to_tr_c = new Link(true, -1, 0.001, c, tr_c, LINK_STACK);
        c->add_interface(c_to_tr_c);

        Link * tr_c_to_c = new Link(true, -1, 0.001, tr_c, c, LINK_STACK);
        tr_c->add_interface(tr_c_to_c);

        sprintf(namInstruction,"l -t * -s 0 -d 1 -S UP -r %i -D %.4f -c black -o 0deg", BOTTLENECK,DELAY1);
        NamGenerator::Inst()->addEvent(namInstruction);
        sprintf(namInstruction,"l -t * -s 1 -d 2 -S UP -r %i -D %.4f -c black -o 0deg", BOTTLENECK2,DELAY2);
        NamGenerator::Inst()->addEvent(namInstruction);
        NamGenerator::Inst()->addEvent("q -t * -s 1 -d 0 -a 0.5");
        NamGenerator::Inst()->addEvent("q -t * -s 0 -d 1 -a 0.5");
        NamGenerator::Inst()->addEvent("q -t * -s 1 -d 2 -a 0.5");
        NamGenerator::Inst()->addEvent("q -t * -s 2 -d 1 -a 0.5");

        // THIS INTERFACE IS BAD! I cannot infringe the destination node...
        //start_app(APP_RATE, a, b); <<-- TO MODIFY

        start_app(APP_RATE, a, tr_b);


        // statistics
        Tcp_segment *dummy_segment = NULL;
        Event * e = new Event(trace, dummy_segment, a, b);
        e->set_timestamp(usim_sched.get_curr_time() + DELTA);

        usim_sched.insert(e);

        current_event = usim_sched.dispatch();

        //****************************************************
        log  = "******************************************************************************";
        log += "\nSalida de uSim";
        log += "\n******************************************************************************";
        log += "\nDatos de entrada:";
        log += "\n TSIM: "+QString::number(TSIM) +" APP_RATE: "+QString::number(APP_RATE)+" BOTTLENECK: "
                + QString::number(BOTTLENECK)+"\n BOTTLENECK2: "+ QString::number(BOTTLENECK2)
                + " SSTHRESH: "+QString::number(SSTHRESH)+" BUFFERSIZE: "+QString::number(BUFFERSIZE)+" DELAY1: "+QString::number(DELAY1);
                + " DELAY2: "+QString::number(DELAY2)+" TIMEOUT: "+QString::number(TIMEOUT)+" DIVACKS: "+QString::number(DIVACKS);
        log += "\n******************************************************************************";
        log += "\nTIEMPO ----- Evento";
        log += "\n******************************";

        //cout << "\nTIME\tEVENT\tSRC\tDST" << endl;

        // there should be a application interaction with the scheduler. That is, a first trigger
        // for the application to start sending packets with a first delay. Then, at the planning of the
        // next packet, to plan the following packet and so on.
        // First trigger (T=0) ------> P1 @ T1 --> P2 @ T2 --> ... -> Pn @ Tn.

        /*  Prueba realizada sin tomar en cuenta el buffer de salida de los nodos   */

        while (usim_sched.get_curr_time() <= TSIM) {

            if (current_event->get_type() == trace)
            {
                Event * e = new Event(trace, dummy_segment, a, b);
                e->set_timestamp(usim_sched.get_curr_time() + DELTA );
                usim_sched.insert(e);

                fprintf(tp_trace,"%f %f\n",usim_sched.get_curr_time(),tp_counter*PACKETSIZE*8);
                tp_counter=0;
                fprintf(gp_trace,"%f %f\n",usim_sched.get_curr_time(),gp_counter*PACKETSIZE*8);
                gp_counter=0;
            }

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
                }

                //Sending first segment
                if(first_iteration)
                {
                    for(int i=1; i<=init_cwnd; i++)
                    {

                        if(current_event->get_to_node()->get_queue_size() > 0)
                        {
                            sending_pdu=current_event->get_to_node()->pdu_departure();
                            unacknowledged_packets.push_back(sending_pdu);

                            usim_sched.plan_service(PKT_DEPARTURE_FROM_TRANSPORT,
                                                                    sending_pdu,
                                                                    current_event->get_to_node(),
                                                                    b);
                            tp_counter++;
                        }
                    }
                    first_iteration=false;
                }

                //log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+" From APP -> PKT_ARRIVAL_TO_TRANSPORT";
                // Recall start_app to keep a rate
                start_app(APP_RATE, a, tr_b);
                // Liberate memory from original message

            }

            //SRC Transport Layer
            // based in TCP NEW RENO


            if (current_event->get_from_node()->get_node_type() == SIMPLE_SRC &&
                    current_event->get_type() == ACK_ARRIVAL_TO_TRANSPORT)
            {
                log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+" From SRC MAC layer -> ACK_ARRIVAL_TO_TRANSPORT";
                current_ack = static_cast<Ack *>(current_event->get_assoc_pdu())->get_ackno();


                //Adding event to Nam file
                sprintf(namInstruction,"r -t %.6f -e %i -s 1 -d 0 -c 1 -i 1",usim_sched.get_curr_time(), ACKSIZE);
                NamGenerator::Inst()->addEvent(namInstruction);

                if(inSS)
                {
                    if(cwnd==SSTHRESH){
                        last_cwnd= cwnd;
                        inCA=true;
                        inSS=false;
                        fromSS=true;
                    }else if(current_ack == -1 && !allow_divacks){      //discard remaining divacks from timeout
                        //do nothing
                    }else if(acknowledge(current_ack))
                    {

                        allow_divacks=true;

                        if(current_ack == -1 && dupAck_counter > 0)         //if a divack arrives and there's a possbile loss
                        {
                            //do nothing by now

                        }else
                        {
                            dupAck_counter=0;
                            sliding_window +=  2; //aproximacion a -> liberar espacio y agregar 1 a ventana deslizante TODO: buscar mejor manera
                            cwnd++;
                            timed_out = usim_sched.get_curr_time() + TIMEOUT;
                            while(sliding_window>0)
                            {
                                sending_pdu=current_event->get_to_node()->pdu_departure();

                                if(static_cast<Tcp_segment *>(sending_pdu)->get_seqno() < current_ack){      //if sender is sending a sent segment
                                    while(static_cast<Tcp_segment *>(sending_pdu)->get_seqno() < current_ack)
                                    {
                                        sending_pdu=current_event->get_to_node()->pdu_departure();
                                    }
                                }

                                unacknowledged_packets.push_back(sending_pdu);

                                usim_sched.plan_service(PKT_DEPARTURE_FROM_TRANSPORT,
                                                                        sending_pdu,
                                                                        current_event->get_to_node(),
                                                                        b);
                                tp_counter++;
                                sliding_window--;
                            }
                        }
                    }else           //not in sent buffer
                    {
                        if(usim_sched.get_curr_time() >= timed_out)            //TIMEOUT event  TODO: create the event "TIMEOUT"
                        {
                            log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+" TIMEOUT while in Slow Start";
                            //retransmit
                            for(std::vector<Pdu *>::reverse_iterator it = unacknowledged_packets.rbegin(); it != unacknowledged_packets.rend(); ++it)
                            {
                                current_event->get_to_node()->insert_in_front(*it);
                            }
                            unacknowledged_packets.erase(unacknowledged_packets.begin(),unacknowledged_packets.end());

                            discard_ack = lost_seqno;

                            cwnd=SSTHRESH;
                            fprintf(ssthresh,"%f %i \n",usim_sched.get_curr_time(),SSTHRESH);
                            SSTHRESH = floor(cwnd/2);
                            fprintf(ssthresh,"%f %i \n",usim_sched.get_curr_time(),SSTHRESH);

                            cwnd= 1;    //setting to initial cwnd

                            //resend
                            sending_pdu=current_event->get_to_node()->pdu_departure();
                            unacknowledged_packets.push_back(sending_pdu);
                            log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+"retransmitting from seqnum: "+ QString::number(static_cast<Tcp_segment *>(sending_pdu)->get_seqno());
                            usim_sched.plan_service(PKT_DEPARTURE_FROM_TRANSPORT,
                                                                    sending_pdu,
                                                                    current_event->get_to_node(),
                                                                    b);
                            timeout_counter++;
                            resend_counter++;
                            tp_counter++;
                            inFR=false;
                            inSS=true;
                            inCA=false;
                            dupAck_counter=0;
                            allow_divacks=false;
                            timed_out = usim_sched.get_curr_time() + TIMEOUT;

                        }else if(current_ack==discard_ack){                       //discarding remaining dupacks from a timeout
                            //do nothing

                        }else if(current_ack == last_ack_sent)
                        {
                            dupAck_counter++;
                            if( dupAck_counter == 3) //dupAck
                            {
                                //Packet loss
                                for(std::vector<Pdu *>::iterator it = unacknowledged_packets.begin(); it != unacknowledged_packets.end(); ++it)
                                {
                                    if(static_cast<Tcp_segment *>(*it)->get_seqno() == current_ack)//lost packet
                                    {
                                        log += "\n****************************";
                                        log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+"LOSS DETECTED!! Retransmitting seqnum: "+ QString::number(static_cast<Tcp_segment *>(*it)->get_seqno());
                                        log += "\n****************************";
                                        lost_seqno = current_ack;  //used by fast recovery
                                        usim_sched.plan_service(PKT_DEPARTURE_FROM_TRANSPORT,
                                                                                *it,
                                                                                current_event->get_to_node(),
                                                                                b);
                                        resend_counter++;
                                        tp_counter++;

                                        timed_out = usim_sched.get_curr_time() + TIMEOUT;
                                        //to fast recovery
                                        cwnd = floor(cwnd/2);

                                        fprintf(ssthresh,"%f %i \n",usim_sched.get_curr_time(),SSTHRESH);
                                        SSTHRESH = cwnd;
                                        fprintf(ssthresh,"%f %i \n",usim_sched.get_curr_time(),SSTHRESH);
                                        //fprintf(ssthresh,"%f %f \n",usim_sched.get_curr_time(),SSTHRESH);
                                        inFR=true;
                                        inSS=false;
                                        fromSS=true;
                                        dupAck_counter=0;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if(current_ack != -1)  //if is not a divack
                    {
                        last_ack_sent = current_ack;
                    }
                }


                if(inFR)// in fast recovery
                {

                    if(current_ack!=lost_seqno && current_ack!= -1)
                    {
                        cwnd=SSTHRESH;
                        last_cwnd= cwnd;
                        inCA=true;
                        inFR=false;
                        fromSS=true;

                    }else
                    {
                        if(usim_sched.get_curr_time() >= timed_out)            //TIMEOUT event  TODO: create the event "TIMEOUT"
                        {
                            log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+" TIMEOUT while in Fast Recovery";
                            //retransmit
                            for(std::vector<Pdu *>::reverse_iterator it = unacknowledged_packets.rbegin(); it != unacknowledged_packets.rend(); ++it)
                            {
                                current_event->get_to_node()->insert_in_front(*it);
                            }
                            unacknowledged_packets.erase(unacknowledged_packets.begin(),unacknowledged_packets.end());

                            discard_ack = lost_seqno;

                            cwnd=SSTHRESH;
                            fprintf(ssthresh,"%f %i \n",usim_sched.get_curr_time(),SSTHRESH);
                            SSTHRESH = floor(cwnd/2);
                            fprintf(ssthresh,"%f %i \n",usim_sched.get_curr_time(),SSTHRESH);

                            cwnd= 1;    //setting to initial cwnd

                            sending_pdu=current_event->get_to_node()->pdu_departure();
                            unacknowledged_packets.push_back(sending_pdu);
                            log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+"retransmitting from seqnum: "+ QString::number(static_cast<Tcp_segment *>(sending_pdu)->get_seqno());

                            usim_sched.plan_service(PKT_DEPARTURE_FROM_TRANSPORT,
                                                                    sending_pdu,
                                                                    current_event->get_to_node(),
                                                                    b);
                            timeout_counter++;
                            resend_counter++;
                            tp_counter++;
                            inFR=false;
                            inSS=true;
                            inCA=false;
                            dupAck_counter=0;
                            allow_divacks=false;


                        }else
                        {
                            last_ack_sent = current_ack;
                            sliding_window +=  1;
                            cwnd++;

                            while(sliding_window>0)
                            {
                                sending_pdu=current_event->get_to_node()->pdu_departure();
                                unacknowledged_packets.push_back(sending_pdu);

                                usim_sched.plan_service(PKT_DEPARTURE_FROM_TRANSPORT,
                                                                        sending_pdu,
                                                                        current_event->get_to_node(),
                                                                        b);
                                sliding_window--;
                                tp_counter++;

                            }
                        }
                    }
                }
                if(inCA) //In CA
                {

                    if(acknowledge(current_ack))
                    {
                        if(current_ack == -1 && dupAck_counter > 0)     //if a divack arrives and there's a possbile loss
                        {
                            //do nothing
                        }else
                        {
                            timed_out = usim_sched.get_curr_time() + TIMEOUT;

                            dupAck_counter=0;
                            sliding_window++;
                            //CA cwnd growth
                            cwnd=cwnd+(1/last_cwnd);

                            if (floor(cwnd) > floor(last_cwnd))
                            {
                                sliding_window++;
                                last_cwnd = cwnd;
                            }
                            while(sliding_window>0)
                            {
                                sending_pdu=current_event->get_to_node()->pdu_departure();

                                if(static_cast<Tcp_segment *>(sending_pdu)->get_seqno() < current_ack){      //if sender is sending a sent segment
                                    while(static_cast<Tcp_segment *>(sending_pdu)->get_seqno() < current_ack)
                                    {
                                        sending_pdu=current_event->get_to_node()->pdu_departure();
                                    }
                                }

                                unacknowledged_packets.push_back(sending_pdu);

                                usim_sched.plan_service(PKT_DEPARTURE_FROM_TRANSPORT,
                                                                        sending_pdu,
                                                                        current_event->get_to_node(),
                                                                        b);
                                tp_counter++;
                                sliding_window--;
                            }
                        }
                    }else{

                        if(usim_sched.get_curr_time() >= timed_out)            //timeout event  TODO: create the event "TIMEOUT"
                        {

                             log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+" TIMEOUT while in Congestion Avoidance";
                            //retransmit
                            for(std::vector<Pdu *>::reverse_iterator it = unacknowledged_packets.rbegin(); it != unacknowledged_packets.rend(); ++it)
                            {
                                current_event->get_to_node()->insert_in_front(*it);
                            }
                            unacknowledged_packets.erase(unacknowledged_packets.begin(),unacknowledged_packets.end());

                            discard_ack = lost_seqno;

                            cwnd=SSTHRESH;
                            fprintf(ssthresh,"%f %i \n",usim_sched.get_curr_time(),SSTHRESH);
                            SSTHRESH = floor(cwnd/2);
                            fprintf(ssthresh,"%f %i \n",usim_sched.get_curr_time(),SSTHRESH);

                            cwnd= 1;    //setting to initial cwnd

                            sending_pdu=current_event->get_to_node()->pdu_departure();
                            unacknowledged_packets.push_back(sending_pdu);

                            log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+"retransmitting from seqnum: "+ QString::number(static_cast<Tcp_segment *>(sending_pdu)->get_seqno());
                            usim_sched.plan_service(PKT_DEPARTURE_FROM_TRANSPORT,
                                                                    sending_pdu,
                                                                    current_event->get_to_node(),
                                                                    b);
                            timeout_counter++;
                            resend_counter++;
                            tp_counter++;
                            inFR=false;
                            inSS=true;
                            inCA=false;
                            dupAck_counter=0;
                            allow_divacks=false;
                            timed_out = usim_sched.get_curr_time() + TIMEOUT;


                        }else if(current_ack == last_ack_sent)
                        {
                            dupAck_counter++;
                            if(dupAck_counter == 3) //dupAck
                            {
                                //Packet loss

                                for(std::vector<Pdu *>::iterator it = unacknowledged_packets.begin(); it != unacknowledged_packets.end(); ++it)
                                {
                                    if(static_cast<Tcp_segment *>(*it)->get_seqno() == current_ack)//lost packet
                                    {
                                        log += "\n****************************";
                                        log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+"LOSS DETECTED!! Retransmitting seqnum: "+ QString::number(static_cast<Tcp_segment *>(*it)->get_seqno());
                                        log += "\n****************************";
                                        lost_seqno = current_ack;  //used by fast recovery
                                        usim_sched.plan_service(PKT_DEPARTURE_FROM_TRANSPORT,
                                                                                *it,
                                                                                current_event->get_to_node(),
                                                                                b);
                                        tp_counter++;

                                        resend_counter++;

                                        timed_out = usim_sched.get_curr_time() + TIMEOUT;

                                        cwnd = floor(cwnd/2);

                                        fprintf(ssthresh,"%f %i \n",usim_sched.get_curr_time(),SSTHRESH);
                                        SSTHRESH = cwnd;
                                        fprintf(ssthresh,"%f %i \n",usim_sched.get_curr_time(),SSTHRESH);

                                        inFR=true;
                                        inCA=false;
                                        dupAck_counter=0;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if(current_ack != -1)
                    {
                        last_ack_sent = current_ack;
                    }
                }

                //file to plot
                fprintf(cwnd_trace,"%f %f \n",usim_sched.get_curr_time(),cwnd);
            }

//******************************End transport layer ****************************************

            if (current_event->get_from_node()->get_node_type() == TRANSPORT &&
                    current_event->get_type() == PKT_DEPARTURE_FROM_TRANSPORT)
            {
                sprintf(namInstruction,"+ -t %.6f -e %i -s 0 -d 1 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                NamGenerator::Inst()->addEvent(namInstruction);
                log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+" From SRC Transport layer -> PKT_DEPARTURE_FROM_TRANSPORT";
                // put packets in the queue of a simple_src (to_node)
                if(current_event->get_to_node()->get_queue_size() == 0)
                {
                    // plan departure of the packet
                    usim_sched.plan_service(PKT_DEPARTURE_FROM_NODE,
                                            current_event->get_assoc_pdu(),
                                            current_event->get_to_node(),
                                            r);
                    sprintf(namInstruction,"- -t %.6f -e %i -s 0 -d 1 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                    NamGenerator::Inst()->addEvent(namInstruction);
                    sprintf(namInstruction,"h -t %.6f -e %i -s 0 -d 1 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                    NamGenerator::Inst()->addEvent(namInstruction);
                }

                current_event->get_to_node()->pdu_arrival(current_event->get_assoc_pdu());


            }
            if (current_event->get_from_node()->get_node_type() == SIMPLE_SRC &&
                    current_event->get_type() == PKT_DEPARTURE_FROM_NODE)
            {
                // dequeueing of the packet (from_node)
                log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+" PKT_DEPARTURE_FROM_NODE";
                Pdu * _pdu = current_event->get_from_node()->pdu_departure();
                // plan propagation of the packet
                usim_sched.plan_propagation(_pdu, current_event->get_from_node(), r);

                if(current_event->get_from_node()->get_queue_size() > 0)
                {
                    usim_sched.plan_service(PKT_DEPARTURE_FROM_NODE,
                                            current_event->get_from_node()->get_front(),
                                            current_event->get_from_node(),
                                            r);
                    //Adding events to Nam file
                    sprintf(namInstruction,"- -t %.6f -e %i -s 0 -d 1 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                    NamGenerator::Inst()->addEvent(namInstruction);
                    sprintf(namInstruction,"h -t %.6f -e %i -s 0 -d 1 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                    NamGenerator::Inst()->addEvent(namInstruction);
                }
            }
            if (current_event->get_type()== PKT_ARRIVAL_TO_QUEUE)
            {
                if (current_event->get_to_node()->get_node_type() == SIMPLE_END)
                {

                    log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+" From Router Node -> PKT_ARRIVAL_TO_QUEUE";
                    // just consume the packet
                    current_event->get_to_node()->increase_received_packets();

                    //Adding event to Nam file
                    sprintf(namInstruction,"r -t %.6f -e %i -s 1 -d 2 -c 1 -i 1",usim_sched.get_curr_time(), PACKETSIZE);
                    NamGenerator::Inst()->addEvent(namInstruction);

                    //sending to transport layer
                    usim_sched.plan_service(PKT_ARRIVAL_TO_TRANSPORT,
                                                            current_event->get_assoc_pdu(),
                                                            current_event->get_to_node(),
                                                            tr_c);
                    // liberate current packet
                    //delete current_event->get_assoc_pdu();
                }
                //ARRIVAL TO ROUTER NODE
                else if (current_event->get_to_node()->get_node_type() == ROUTER)
                {
                    log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+" From SRC MAC layer -> PKT_ARRIVAL_TO_QUEUE";

                    sprintf(namInstruction,"r -t %.6f -e %i -s 0 -d 1 -c 1 -i 1",usim_sched.get_curr_time(), PACKETSIZE);
                    NamGenerator::Inst()->addEvent(namInstruction);
                    sprintf(namInstruction,"+ -t %.6f -e %i -s 1 -d 2 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                    NamGenerator::Inst()->addEvent(namInstruction);

                    if(current_event->get_to_node()->get_queue_size() == 0 )
                    {
                        usim_sched.plan_service(PKT_DEPARTURE_FROM_ROUTER,
                                                                current_event->get_assoc_pdu(),
                                                                current_event->get_to_node(),
                                                                c);
                        sprintf(namInstruction,"- -t %.6f -e %i -s 1 -d 2 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                        NamGenerator::Inst()->addEvent(namInstruction);
                        sprintf(namInstruction,"h -t %.6f -e %i -s 1 -d 2 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                        NamGenerator::Inst()->addEvent(namInstruction);
                    }
                    if(current_event->get_to_node()->get_queue_size() + 1 > current_event->get_to_node()->get_queue_capacity())
                    {

                        log += "\n****************************";
                        log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+" Full Router node forward queue -> DROPPING pck id:" + QString::number(static_cast<Tcp_segment *>(current_event->get_assoc_pdu())->get_seqno());
                        log += "\n****************************";
                        loss_counter++;
                        sprintf(namInstruction,"d -t %.6f -e %i -s 1 -d 2 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                        NamGenerator::Inst()->addEvent(namInstruction);
                    }
                    else
                    {
                        current_event->get_to_node()->pdu_arrival(current_event->get_assoc_pdu());
                    }

                }
                else
                    printf("ERROR: unknown event\n");
            }
            if (current_event->get_from_node()->get_node_type() == ROUTER &&
                    current_event->get_type() == PKT_DEPARTURE_FROM_ROUTER)
            {

                    log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+"PKT_DEPARTURE_FROM_ROUTER";
                    Pdu * _pdu = current_event->get_from_node()->pdu_departure();
                    usim_sched.plan_propagation(_pdu, current_event->get_from_node(), c);

                    if(current_event->get_from_node()->get_queue_size() > 0 )
                    {
                        usim_sched.plan_service(PKT_DEPARTURE_FROM_ROUTER,
                                                                current_event->get_from_node()->get_front(),
                                                                current_event->get_from_node(),
                                                                c);
                        sprintf(namInstruction,"- -t %.6f -e %i -s 1 -d 2 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                        NamGenerator::Inst()->addEvent(namInstruction);
                        sprintf(namInstruction,"h -t %.6f -e %i -s 1 -d 2 -c 1 -i 1 ",usim_sched.get_curr_time(), PACKETSIZE);
                        NamGenerator::Inst()->addEvent(namInstruction);
                    }
            }

    //      ACKs events dinamic

//*****************TRANSPORT LAYER
            if (current_event->get_from_node()->get_node_type() == SIMPLE_END &&
                    current_event->get_type() == PKT_ARRIVAL_TO_TRANSPORT)
            {
                log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+" From DST MAC layer -> PKT_ARRIVAL_TO_TRANSPORT";
                Tcp_segment * acknowledged_segment =  static_cast<Tcp_segment *>(current_event->get_assoc_pdu());
                it = received_packets.begin();

//                if(usim_sched.get_curr_time()  > next_RTT){

//                    fprintf(gp_trace,"%f %f\n",usim_sched.get_curr_time(),gp_counter*PACKETSIZE*8);
//                    gp_counter=0;
//                    next_RTT += (DELAY1 + DELAY2);
//                }

                 if(expected_seqno == acknowledged_segment->get_seqno())
                {
                    expected_seqno++;
                    //*************************
                    //send to app this segment
                        app_counter ++;
                        gp_counter++;
                    //*************************

                    while(it != received_packets.end())
                    {
                        if(expected_seqno == static_cast<Tcp_segment *>(*it)->get_seqno())
                        {
                            //*******************
                            //send to app
                                app_counter ++;
                                gp_counter++;
                            //*******************
                            expected_seqno++;
                        }else{
                            //missing packet, needs to be retransmitted
                            break;
                        }
                        received_packets.erase(it);  //moving on

                        if(received_packets.empty()){break;}
                    }
                }else{
                    //missing packet, needs to be retransmitted
                    received_packets.push_back(acknowledged_segment); //sent packets buffer
                }

                    //DIVACKS
                    for(int i=0; i<DIVACKS; i++){
                     Ack *prepared_ack = new Ack(ACKSIZE, -1);
                     usim_sched.plan_service(ACK_DEPARTURE_FROM_TRANSPORT,
                                                             prepared_ack,
                                                             current_event->get_to_node(),
                                                             c);
                    }

                    Ack *prepared_ack = new Ack(ACKSIZE, expected_seqno);
                    usim_sched.plan_service(ACK_DEPARTURE_FROM_TRANSPORT,
                                                         prepared_ack,
                                                         current_event->get_to_node(),
                                                         c);


            }

//************************

            if (current_event->get_from_node()->get_node_type() == TRANSPORT &&
                    current_event->get_type() == ACK_DEPARTURE_FROM_TRANSPORT)
            {
                log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+"ACK_DEPARTURE_FROM_TRANSPORT";
                // plan departure of the packet
                usim_sched.plan_service(ACK_DEPARTURE_FROM_NODE,
                                        current_event->get_assoc_pdu(),
                                        current_event->get_to_node(),
                                        r);
            }
            //MAC LAYER
            if (current_event->get_from_node()->get_node_type() == SIMPLE_END &&
                    current_event->get_type() == ACK_DEPARTURE_FROM_NODE)
            {
                log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+"ACK_DEPARTURE_FROM_NODE";
                Pdu * _pdu = current_event->get_assoc_pdu();
                usim_sched.plan_propagation(_pdu, current_event->get_from_node(),current_event->get_to_node(), ACK_ARRIVAL_TO_ROUTER);

                //Adding event to Nam file
                sprintf(namInstruction,"h -t %.6f -e %i -s 2 -d 1 -c 1 -i 1",usim_sched.get_curr_time(), ACKSIZE);
                NamGenerator::Inst()->addEvent(namInstruction);
            }
            if (current_event->get_from_node()->get_node_type() == SIMPLE_END &&
                    current_event->get_type() == ACK_ARRIVAL_TO_ROUTER)
            {
                log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+"ACK_ARRIVAL_TO_ROUTER";
                sprintf(namInstruction,"r -t %.6f -e %i -s 2 -d 1 -c 1 -i 1",usim_sched.get_curr_time(), ACKSIZE);
                NamGenerator::Inst()->addEvent(namInstruction);

                current_event->get_to_node()->ack_arrival(current_event->get_assoc_pdu());
                usim_sched.plan_service(ACK_DEPARTURE_FROM_ROUTER,
                                                        current_event->get_assoc_pdu(),
                                                        current_event->get_to_node(),
                                                        b);
            }
            if (current_event->get_from_node()->get_node_type() == ROUTER &&
                    current_event->get_type() == ACK_DEPARTURE_FROM_ROUTER)
            {
                log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+"ACK_DEPARTURE_FROM_ROUTER";
                Pdu * _pdu = current_event->get_from_node()->ack_departure();
                usim_sched.plan_propagation(_pdu, current_event->get_from_node(), b, ACK_ARRIVAL_TO_SRC);

                sprintf(namInstruction,"h -t %.6f -e %i -s 1 -d 0 -c 1 -i 1",usim_sched.get_curr_time(), ACKSIZE);
                NamGenerator::Inst()->addEvent(namInstruction);
            }
            if (current_event->get_from_node()->get_node_type() == ROUTER &&
                    current_event->get_type() == ACK_ARRIVAL_TO_SRC)
            {
                log += "\n"+QString::number(usim_sched.get_curr_time()) +" ----- "+"ACK_ARRIVAL_TO_SRC";
                usim_sched.plan_service(ACK_ARRIVAL_TO_TRANSPORT,
                                                        current_event->get_assoc_pdu(),
                                                        current_event->get_to_node(),
                                                        tr_b);
            }
            delete current_event;
            current_event = usim_sched.dispatch();
        }
        log += "\n******************************************************************************";
        log += "\nCantidad neta de paquetes recibidos Nodo final: "+ QString::number(c->get_received_packets());
        log += "\nCantidad de paquetes recibidos por capa APP: "+ QString::number(app_counter);
        log += "\nCantidad de paquetes perdidos: "+ QString::number(loss_counter);
        log += "\nCantidad de paquetes reenviados: "+ QString::number(resend_counter);
        log += "\nCantidad de Timeouts: "+ QString::number(timeout_counter);
        log += "\n******************************************************************************";

        fclose(cwnd_trace);
        fclose(gp_trace);
        fclose(tp_trace);
        fprintf(ssthresh,"%f %i \n",(usim_sched.get_curr_time() - 0.01),SSTHRESH);
        fclose(ssthresh);


        return 0;
    }

    void plot_cwnd()
    {
        FILE *pipe = popen("gnuplot -persist","w");

        fprintf(pipe, "set grid\n");
        fprintf(pipe, "set boxwidth 1 absolute\n");
        fprintf(pipe, "set style fill solid 1.00 border -1\n");
        fprintf(pipe, "set terminal jpeg medium\n");
        fprintf(pipe, "set output 'temp/uSim_cwnd.jpeg'\n");
        fprintf(pipe, "set xlabel 'Tiempo(s)'\n");
        fprintf(pipe, "set ylabel 'CWND(segmentos)'\n");
        fprintf(pipe, "set key top left\n");
        fprintf(pipe, "plot \"temp/cwnd_trace.dat\" using 1:2 with linespoints  title \" cwnd \", \"temp/ssthresh_trace.dat\" using 1:2 with linespoints linetype 12 title \" ssthreshold \" \n");
        //fprintf(pipe, "plot \"temp/cwnd_trace.dat\" using 1:2 with linespoints  title \" cwnd \"\n");
        //fprintf(pipe, "plot \"temp/cwnd_trace1.dat\" using 1:2 with linespoints  title \" cwnd 0 divacks\", \"temp/cwnd_trace2.dat\" using 1:2 with linespoints  title \" cwnd 1 divacks\", \"temp/cwnd_trace3.dat\" using 1:2 with linespoints  title \" cwnd 4 divacks\",\"temp/cwnd_trace4.dat\" using 1:2 with linespoints  title \" cwnd 8 divacks\"  \n");
        fclose(pipe);

    }
    void plot_gp()
    {
        FILE *pipe = popen("gnuplot -persist","w");

        fprintf(pipe, "set grid\n");
        fprintf(pipe, "set boxwidth 1 absolute\n");
        fprintf(pipe, "set style fill solid 1.00 border -1\n");
        fprintf(pipe, "set terminal jpeg medium\n");
        fprintf(pipe, "set output 'temp/uSim_gp.jpeg'\n");
        fprintf(pipe, "set xlabel 'Tiempo(s)'\n");
        fprintf(pipe, "set ylabel 'Goodput(bps)'\n");
        fprintf(pipe, "set key top left\n");
        fprintf(pipe, "plot \"temp/gp_trace.dat\" using 1:2 with linespoints linetype rgb \"#FF4000\" title \" Goodput \" \n");
        fclose(pipe);
    }
    void plot_tp()
    {
        FILE *pipe = popen("gnuplot -persist","w");

        fprintf(pipe, "set grid\n");
        fprintf(pipe, "set boxwidth 1 absolute\n");
        fprintf(pipe, "set style fill solid 1.00 border -1\n");
        fprintf(pipe, "set terminal jpeg medium\n");
        fprintf(pipe, "set output 'temp/uSim_tp.jpeg'\n");
        fprintf(pipe, "set xlabel 'Tiempo(s)'\n");
        fprintf(pipe, "set ylabel 'Rendimiento(bps)'\n");
        fprintf(pipe, "set key top left\n");
        fprintf(pipe, "plot \"temp/tp_trace.dat\" using 1:2 with linespoints linetype 3 title \" Throughput \" \n");
//        fprintf(pipe, "plot \"temp/tp_trace.dat\" using 1:2 with linespoints linetype rgb \"#0404B4\" title \" Throughput \", \"temp/gp_trace.dat\" using 1:2 with linespoints linetype rgb \"#FF4000\" title \" Goodput \"\n");
        fclose(pipe);
    }
    void plot_tp_and_gp()
    {
        FILE *pipe = popen("gnuplot -persist","w");

        fprintf(pipe, "set grid\n");
        fprintf(pipe, "set boxwidth 1 absolute\n");
        fprintf(pipe, "set style fill solid 1.00 border -1\n");
        fprintf(pipe, "set terminal jpeg medium\n");
        fprintf(pipe, "set output 'temp/uSim_tp&gp.jpeg'\n");
        fprintf(pipe, "set xlabel 'Tiempo(s)'\n");
        fprintf(pipe, "set ylabel 'Rendimiento(bps)'\n");
        fprintf(pipe, "set key top left\n");

        fprintf(pipe, "plot \"temp/tp_trace.dat\" using 1:2 with linespoints linetype rgb \"#0404B4\" title \" Throughput \", \"temp/gp_trace.dat\" using 1:2 with linespoints linetype rgb \"#FF4000\" title \" Goodput \"\n");
        fclose(pipe);
    }

    bool isEmpty(){
        if (TSIM==0 && APP_RATE==0 && BOTTLENECK==0 && BOTTLENECK2==0 && DIVACKS==0 && SSTHRESH==0 && BUFFERSIZE==0 && TIMEOUT == 0)
        {
            return true;
        }
        return false;
    }
    //GETTERS

    float getTSIM(){return TSIM;}
    int getAPP_RATE(){return APP_RATE;}
    int getBOTTLENECK(){return BOTTLENECK;}
    int getBOTTLENECK2(){return BOTTLENECK2;}
    int getDIVACKS(){return DIVACKS;}
    int getSSTHRESH(){return sst;}
    int getBUFFERSIZE(){return BUFFERSIZE;}
    float getDELAY1(){return DELAY1;}
    float getDELAY2(){return DELAY2;}
    float getTIMEOUT(){return TIMEOUT;}
    float getDELTA(){return DELTA;}
    QString getlog(){return log;}

};

#endif // USIM_H
