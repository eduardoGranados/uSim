/**
 *  scheduler.h
 *  Clase Scheduler
 *
 **/

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "event.h"
#include "namgenerator.h"
#include <cstdlib>
#include <iostream>
#include <list>
#include <math.h>

class ListScheduler {

    private:

        std::list <Event *> scheduler;
        double curr_time;
        char *namInstruction;
        double last_pdu_time;   //temporary solution to out packets from router node
    
        bool check_event(Event *e)
        {
            if (e->get_type() < PKT_ARRIVAL_TO_QUEUE || e->get_type() >= LAST_EVENT)
                return false;
            if (e->get_from_node() == NULL || e->get_to_node() == NULL)
                return false;
            if (e->get_assoc_pdu() == NULL)
                return false;
            return true;
        }

    public:
        ListScheduler() 
        {
            curr_time = 0;
            //auxiliary string for Nam file generator
            namInstruction =new char[100];
            last_pdu_time = 0;
        }

        bool compare(double a, double b){ // to compare 2 floats

            if (fabs(a - b) <= 0.00000001){   //whether is equal or not.  10ˆ-8 epsilon
                return false;
            }else
                return (a>b);
        }

        void insert(Event * event) {
            std::list <Event *>::iterator i;
            bool ins_done;
            
            if (!check_event(event))
            {
                printf("ERROR: invalid event!\n");
                exit(1);
            }

            ins_done = false;
            
            if (scheduler.empty()) {
                scheduler.push_back(event);
            }
            else {

                for (i = scheduler.begin(); i != scheduler.end(); i++)
                {
                    if ( compare((*i)->get_timestamp(), event->get_timestamp()) )
                    {
                        //printf("%.20f > %.20f \n", (*i)->get_timestamp() ,event->get_timestamp());
                        scheduler.insert(i, event);
                        ins_done = true;
                        break;
                    }
                }
                // the event goes at the very end if not done  
                if (!ins_done)
                {
                    scheduler.push_back(event);
                }
            }
        }

        Event * dispatch() {
            Event * curr_event;
            if (!scheduler.empty())
            {
            curr_event = scheduler.front();
            scheduler.pop_front();
            
            //updates the current time
            curr_time = curr_event->get_timestamp();
            }
            else
            {
                printf("error: empty scheduler.\n");
                exit(1);
            }
            return curr_event;
        }

        double get_curr_time()
        {
            return curr_time;
        }
    
        void plan_propagation(Pdu * _pdu, Node * from, Node * dst, EVENT_TYPE propagate_to = PKT_ARRIVAL_TO_QUEUE)
        {
            //***
            //
            // TODO: to determine new target,
            // this SHOULD have a routing table, but we're not there yet!
            // for the moment, we "know" that the destination node is "c".
            //

            // planify packet departure to the link to propagate packet
            Event * e = new Event(propagate_to, _pdu, from, dst);
            // determine new source

            Link * out_link = from->get_interface(dst);

            if (out_link == 0)
            {
                printf("ERROR: interface not found or link not connected!\n");
                exit(1);
            }

            e->set_timestamp(get_curr_time() + out_link->get_delay());


            this->insert(e);

            out_link->set_available(true);



            // TODO: Review this code propagating following packets.
            //            if (from->get_queue_size() > 0)
            //                plan_service(from, dst, PKT_ARRIVAL_TO_QUEUE);
        }

        void plan_service(EVENT_TYPE nxt_service, Pdu * _pdu, Node * from, Node * dst)
        {

            // Discriminates the type of service??? For example if it arrives from an APP or from another node, etc.
            // It shuould evaluate the service depending on the origin (from) and destination (dst).
            // for the time being it just gets plans for departures...

            Link * out_link = from->get_interface(dst);
            if(out_link == NULL)
            {
                printf("ERROR: some needed interfaces are not connected\n");
                cout<<from->get_node_type()<<"-->"<<dst->get_node_type()<<endl;
            }
            if (!out_link->get_available())
            {
                printf("\nERROR: interface at %s is busy!\n",from->get_node_name());
                // enqueue segment in source (from) because can not get the service
                // so far the enqueued packets counts from the 1st in queue on (1 in service & 1 in queue).
                from->pdu_arrival(_pdu);
                return;
            }
            // planify packet departure to the link to propagate packet
            Event * e = new Event(nxt_service, _pdu, from, dst);

            //
            // determine new target
            // this SHOULD include the routing problem, but we're not there yet!
            // for the moment, we "know" that the destination node is "c"
            //

            //TODO:  shorten these algorithms, seems easy since some look alike.

            if (nxt_service == PKT_DEPARTURE_FROM_NODE)
            {
                e->set_timestamp(get_curr_time()
                            + (PACKETSIZE * 8.0/(out_link->get_bandwidth())));
            }
            else if (nxt_service == ACK_DEPARTURE_FROM_NODE)
            {

                e->set_timestamp(get_curr_time()
                            + ((ACKSIZE) * 8.0/(out_link->get_bandwidth())));
            }
            else if (nxt_service == PKT_DEPARTURE_FROM_ROUTER)
            {
                e->set_timestamp(get_curr_time()
                            + (PACKETSIZE * 8.0/(out_link->get_bandwidth())));
            }
            else if (nxt_service == ACK_DEPARTURE_FROM_ROUTER)
            {
                e->set_timestamp(get_curr_time()
                            + (ACKSIZE * 8.0/(out_link->get_bandwidth())));
            }
            else if (nxt_service == PKT_DEPARTURE_FROM_TRANSPORT)
            {
                if(out_link->get_bandwidth()==-1){  // where -1 = infinite Bandwidth
                    e->set_timestamp(get_curr_time());
                }else{
                    e->set_timestamp(get_curr_time() + ((PACKETSIZE*8.0/(out_link->get_bandwidth()))));
                }
            }
            else if (nxt_service == PKT_ARRIVAL_TO_TRANSPORT)
            {
                if(out_link->get_bandwidth()==-1){
                    e->set_timestamp(get_curr_time());
                }else{
                    e->set_timestamp(get_curr_time() + ((PACKETSIZE*8.0/(out_link->get_bandwidth()))));
                }
            }
            else if (nxt_service == ACK_DEPARTURE_FROM_TRANSPORT)
            {
                if(out_link->get_bandwidth()==-1){
                    e->set_timestamp(get_curr_time());
                }else{
                    e->set_timestamp(get_curr_time() + (ACKSIZE*8.0/(out_link->get_bandwidth())));
                }
            }
            else if (nxt_service == ACK_ARRIVAL_TO_TRANSPORT)
            {
                if(out_link->get_bandwidth()==-1){
                    e->set_timestamp(get_curr_time());
                }else{
                    e->set_timestamp(get_curr_time() + (ACKSIZE*8.0/(out_link->get_bandwidth())));
                }
            }
            else
            {
                printf("ERROR: Unhandable event\n");
                exit(1);
            }

            if (nxt_service == PKT_ARRIVAL_TO_QUEUE)
            {
                // we're blocking the interface when
                // planning the event because from the moment of the planning to the
                // execution of the event, the packet is being sent.

                out_link->set_available(false);
            }


            this->insert(e);

        }


        void debug() {
            std::list <Event *>::iterator i;

            for (i = scheduler.begin(); i != scheduler.end(); i++) {
                std::cout << (*i)->get_timestamp();
            }
        }
};

#endif  // SCHEDULER_H
