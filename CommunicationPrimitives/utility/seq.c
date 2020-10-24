/*********************************************************
 * This code was written in the context of the Lightkone
 * European project.
 * Code is of the authorship of NOVA (NOVA LINCS @ DI FCT
 * NOVA University of Lisbon)
 * Author:
 * André Rosa (af.rosa@campus.fct.unl.pt
 * Under the guidance of:
 * Pedro Ákos Costa (pah.costa@campus.fct.unl.pt)
 * João Leitão (jc.leitao@fct.unl.pt)
 * (C) 2020
 *********************************************************/

#include "seq.h"

unsigned short inc_seq(unsigned short current_seq, bool ignore_zero) {
    unsigned short next_seq = current_seq + 1;
    return ignore_zero ? (next_seq == 0 ? 1 : next_seq) : next_seq;
}

int compare_seq(unsigned short s1, unsigned short s2, bool ignore_zero) {

    // TODO: incluir o ignore zero

    int delta = s1 - s2;

    if( delta == 0 ) { // s1 == s2
        return 0;
    }

    else if( delta > 0 ) { // s1 > s2
        if( delta <= MAX_SEQ_HALF ) {
            return delta; // >0 --> s1 > s2
        } else {
            return (-1)*(MAX_SEQ - s1 + s2 + (ignore_zero ? 0 : -1) ); // <0 --> s2 > s1
        }
    }

    else { // s2 > s1
        if( (-1)*delta > MAX_SEQ_HALF ) {
            return MAX_SEQ - s2 + s1 + (ignore_zero ? 0 : -1); // >0 --> s1 > s2
        } else {
            return delta; // <0 --> s2 > s1
        }
    }

}
