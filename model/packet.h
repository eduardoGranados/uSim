#ifndef PACKET_H
#define PACKET_H

enum PDU_TYPE
{
    PDU_TYPE_ICMP_ECHO_REQ,
    PDU_TYPE_ICMP_ECHO_REP,
    PDU_TYPE_TCP_DATA,
    PDU_TYPE_TCP_ACK,
    PDU_TYPE_MESSAGE
};

class Pdu
{
    unsigned pdu_size;
    PDU_TYPE pdu_type;

public:
    Pdu(unsigned _size, PDU_TYPE _pdu_type) :
        pdu_size(_size),
        pdu_type(_pdu_type)
    {
        // EMPTY
    }
    virtual ~Pdu() { }

    unsigned get_pdu_size() const
    {
        return pdu_size;
    }

    PDU_TYPE get_pdu_type() const
    {
        return pdu_type;
    }
};

class Message : public Pdu
{
       unsigned msg_id;

public:

       Message(unsigned _msg_id, unsigned _msg_size) :
           Pdu(_msg_size, PDU_TYPE_MESSAGE),
           msg_id(_msg_id)
       {
           // Empty
       }

       unsigned get_msg_id()
       {
           return msg_id;
       }
};

class Tcp_segment : public Pdu
{
    int seqno;

public:

    Tcp_segment(unsigned segment_size, int _seqno) :
        Pdu(segment_size, PDU_TYPE_TCP_DATA),
        seqno(_seqno)
    {
       // Empty
    }

    int get_seqno()
    {
        return seqno;
    }

};

class Ack: public Pdu
{
    int ackno;

public:

    Ack(unsigned _size, int _ackno) :
        Pdu(_size, PDU_TYPE_TCP_ACK),
        ackno(_ackno)
    {
       // Empty
    }

    int get_ackno()
    {
        return ackno;
    }

};
#endif // PACKET_H
