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

class ListScheduler {

    private:
        std::list <Event *> scheduler;
        double curr_time;
        char *namInstruction;
        double aux;
    
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
            aux=0;
            curr_time = 0;
            //auxiliary string for Nam file generator
            namInstruction =new char[100];
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
                    if ((*i)->get_timestamp() > event->get_timestamp())
                    {
                        scheduler.insert(i, event);
                        ins_done = true;
                        break;
                    }
                    if ((*i)->get_timestamp() == event->get_timestamp())
                    {
                        if(event->get_type() == PKT_DEPARTURE_FROM_TRANSPORT && event->get_to_node()->get_node_type() == SIMPLE_SRC )
                        {
                            i++;
                            scheduler.insert(i, event);
                            ins_done = true;
                            break;
                        }else
                        {
                            scheduler.insert(i, event);
                            ins_done = true;
                            break;
                        }

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
//            if(propagate_to == PKT_ARRIVAL_TO_QUEUE){
//                //Adding events to Nam file
//                sprintf(namInstruction,"- -t %f -e %i -s 0 -d 1 -c 1 -i 1 ",get_curr_time(), PACKETSIZE);
//                NamGenerator::Inst()->addEvent(namInstruction);
//                sprintf(namInstruction,"h -t %f -e %i -s 0 -d 1 -c 1 -i 1 ",get_curr_time(), PACKETSIZE);
//                NamGenerator::Inst()->addEvent(namInstruction);
//            }

            this->insert(e);

            out_link->set_available(true);

            // TODO: Review this code propagating following packets.
            //            if (from->get_queue_size() > 0)
            //                plan_service(from, dst, PKT_ARRIVAL_TO_QUEUE);
        }

        void plan_service(EVENT_TYPE nxt_service, Pdu * _pdu, Node * from, Node * dst, int pdu_sent = 1)
        {

            // Discriminates the type of service??? For example if it arrives from an APP or from another node, etc.
            // It shuould evaluate the service depending on the origin (from) and destination (dst).
            // for the time being it just gets plans for departures...
            //*****
            //pdu_sent : id for the tcp segment sent. Used only by transport node.

            Link * out_link = from->get_interface(dst);

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

            if (nxt_service == PKT_DEPARTURE_FROM_NODE)
            {
                sprintf(namInstruction,"- -t %f -e %i -s 0 -d 1 -c 1 -i 1 ",get_curr_time(), PACKETSIZE);
               NamGenerator::Inst()->addEvent(namInstruction);
               sprintf(namInstruction,"h -t %f -e %i -s 0 -d 1 -c 1 -i 1 ",get_curr_time(), PACKETSIZE);
               NamGenerator::Inst()->addEvent(namInstruction);

                e->set_timestamp(get_curr_time()
                          + (from->get_queue_size()) * (PACKETSIZE * 8.0/(out_link->get_bandwidth())));

            }
            else if (nxt_service == PKT_DEPARTURE_FROM_TRANSPORT)
            {
                e->set_timestamp(get_curr_time() + ((PACKETSIZE*8.0/(out_link->get_bandwidth()))+out_link->get_delay()));

            }
            else if (nxt_service == PKT_ARRIVAL_TO_TRANSPORT)
            {
                e->set_timestamp(get_curr_time() + (PACKETSIZE*8.0/(out_link->get_bandwidth()))+out_link->get_delay());
            }
            else if (nxt_service == ACK_DEPARTURE_FROM_TRANSPORT)
            {
                e->set_timestamp(get_curr_time() + (54*8.0/(out_link->get_bandwidth()))); //54B ACK SIZE
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
            if (nxt_service == ACK_ARRIVAL_TO_TRANSPORT)
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
