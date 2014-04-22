/**
 *  link.h
 *  Clase Link
 *
 **/

#ifndef LINK_H_
#define LINK_H_

class Node;

enum LinkType {
    
    LINK_NETWORK,
    LINK_STACK

};


class Link {

    private:

    bool available;    // whether it is available
    int bandwidth;     // in bit per seconds
    double delay;         // in seconds
    Node * source;
    Node * target;
    LinkType link_type;
    
    public:

        Link(bool _av, double _bw, double _dl, Node * _src, Node * _tgt, LinkType _lk) :
            available(_av),
            bandwidth(_bw),
            delay(_dl),
            source(_src),
            target(_tgt),
            link_type(_lk)
       {
             // EMTPTY
       }

        bool get_available() {
            return available;
        }

        void set_available(bool _av) {
            available = _av;
        }

        int get_bandwidth() {
            return bandwidth;
        }

        void set_bandwidth(double _bw) {
            bandwidth = _bw;
        }
    
        double get_delay()
        {
            return delay;
        }

        Node * get_target_node() {
            return target;
        }

        Node * get_source_node() {
            return source;
        }


        ~Link() {}
};

#endif  // LINK_H
