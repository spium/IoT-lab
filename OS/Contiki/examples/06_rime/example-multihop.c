/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * $Id: example-multihop.c,v 1.4 2009/03/23 18:10:09 adamdunkels Exp $
 */

/**
 * \file
 *         Testing the multihop forwarding layer (multihop) in Rime
 * \author
 *         Adam Dunkels <adam@sics.se>
 *
 *
 *         This example shows how to use the multihop Rime module, how
 *         to use the announcement mechanism, how to manage a list
 *         with the list module, and how to allocate memory with the
 *         memb module.
 *
 *         The multihop module provides hooks for forwarding packets
 *         in a multi-hop fashion, but does not implement any routing
 *         protocol. A routing mechanism must be provided by the
 *         application or protocol running on top of the multihop
 *         module. In this case, this example program provides the
 *         routing mechanism.
 *
 *         The routing mechanism implemented by this example program
 *         is very simple: it forwards every incoming packet to a
 *         random neighbor. The program maintains a list of neighbors,
 *         which it populated through the use of the announcement
 *         mechanism.
 *
 *         The neighbor list is populated by incoming announcements
 *         from neighbors. The program maintains a list of neighbors,
 *         where each entry is allocated from a MEMB() (memory block
 *         pool). Each neighbor has a timeout so that they do not
 *         occupy their list entry for too long.
 *
 *         When a packet arrives to the node, the function forward()
 *         is called by the multihop layer. This function picks a
 *         random neighbor to send the packet to. The packet is
 *         forwarded by every node in the network until it reaches its
 *         final destination (or is discarded in transit due to a
 *         transmission error or a collision).
 *
 */

#include "contiki.h"
#include "net/rime.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"

#include "dev/leds.h"

#include <stdio.h>

/* receiver_node_rime_addr configuration */
#include "common-config.h"

#define CHANNEL 128

struct example_neighbor {
    struct example_neighbor *next;
    rimeaddr_t addr;
    struct ctimer ctimer;
};

extern process_event_t serial_line_event_message;

#define NEIGHBOR_TIMEOUT 60 * CLOCK_SECOND
#define MAX_NEIGHBORS 16
LIST(neighbor_table);
MEMB(neighbor_mem, struct example_neighbor, MAX_NEIGHBORS);
/*---------------------------------------------------------------------------*/
PROCESS(example_multihop_process, "multihop example");
AUTOSTART_PROCESSES(&example_multihop_process);
/*---------------------------------------------------------------------------*/
/*
 * This function is called by the ctimer present in each neighbor
 * table entry. The function removes the neighbor from the table
 * because it has become too old.
 */
static void remove_neighbor(void *n)
{
    struct example_neighbor *e = n;

    list_remove(neighbor_table, e);
    memb_free(&neighbor_mem, e);
}

/*---------------------------------------------------------------------------*/
/*
 * This function is called when an incoming announcement arrives. The
 * function checks the neighbor table to see if the neighbor is
 * already present in the list. If the neighbor is not present in the
 * list, a new neighbor table entry is allocated and is added to the
 * neighbor table.
 */
static void received_announcement(struct announcement *a,
        const rimeaddr_t *from,
        uint16_t id, uint16_t value)
{
    struct example_neighbor *e;

    /*  printf("Got announcement from %d.%d, id %d, value %d\n",
        from->u8[0], from->u8[1], id, value); */

    /* We received an announcement from a neighbor so we need to update
       the neighbor list, or add a new entry to the table. */
    for (e = list_head(neighbor_table); e != NULL; e = e->next) {
        if (rimeaddr_cmp(from, &e->addr)) {
            /* Our neighbor was found, so we update the timeout. */
            ctimer_set(&e->ctimer, NEIGHBOR_TIMEOUT,
                    remove_neighbor, e);
            return;
        }
    }

    /* The neighbor was not found in the list, so we add a new entry by
       allocating memory from the neighbor_mem pool, fill in the
       necessary fields, and add it to the list. */
    e = memb_alloc(&neighbor_mem);
    if (e != NULL) {
        rimeaddr_copy(&e->addr, from);
        list_add(neighbor_table, e);
        ctimer_set(&e->ctimer, NEIGHBOR_TIMEOUT, remove_neighbor, e);
    }
}

static struct announcement example_announcement;
/*---------------------------------------------------------------------------*/
/*
 * This function is called at the final recepient of the message.
 */
static void recv(struct multihop_conn *c, const rimeaddr_t *sender,
        const rimeaddr_t * prevhop, uint8_t hops)
{
    printf("multihop message received '%s'\n", (char *)packetbuf_dataptr());
}

/*
 * This function is called to forward a packet. The function picks a
 * random neighbor from the neighbor list and returns its address. The
 * multihop layer sends the packet to this address. If no neighbor is
 * found, the function returns NULL to signal to the multihop layer
 * that the packet should be dropped.
 */
static rimeaddr_t *forward(struct multihop_conn *c,
        const rimeaddr_t * originator,
        const rimeaddr_t * dest, const rimeaddr_t * prevhop,
        uint8_t hops)
{
    int num = 0;
    int i;
    struct example_neighbor *n = NULL;

    /* Find a random neighbor to send to. */
    if (list_length(neighbor_table) > 0) {
        num = random_rand() % list_length(neighbor_table);
        i = 0;
        for (n = list_head(neighbor_table); n != NULL && i != num;
                n = n->next) {
            ++i;
        }
    }

    if (n != NULL) {
        printf ("%d.%d: Forwarding packet to %d.%d (%d in list),"
                "hops %d\n",
                rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1],
                n->addr.u8[0], n->addr.u8[1], num,
                packetbuf_attr(PACKETBUF_ATTR_HOPS));
        return &n->addr;
    } else {
        printf("%d.%d: did not find a neighbor to foward to\n",
                rimeaddr_node_addr.u8[0], rimeaddr_node_addr.u8[1]);
        return NULL;
    }

}

static const struct multihop_callbacks multihop_call = { recv, forward };

static struct multihop_conn multihop;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_multihop_process, ev, data)
{
    PROCESS_EXITHANDLER(multihop_close(&multihop));
    PROCESS_BEGIN();

    memb_init(&neighbor_mem);
    list_init(neighbor_table);

    /* Open a multihop connection on Rime channel CHANNEL. */
    multihop_open(&multihop, CHANNEL, &multihop_call);
    /* Register an announcement with the same announcement ID as the
       Rime channel we use to open the multihop connection above. */
    announcement_register(&example_announcement, CHANNEL,
            received_announcement);
    announcement_set_value(&example_announcement, 0);

    /* Receiver node does nothing else than listening */
    if (rimeaddr_node_addr.u8[0] == receiver_node_rime_addr[0]
            && rimeaddr_node_addr.u8[1] == receiver_node_rime_addr[1]) {
        printf("Receiver node listening\n");
        PROCESS_WAIT_EVENT_UNTIL(0);
    }

    printf("Write a character on serial link to send message\n");
    while (1) {
        rimeaddr_t to;

        /* Wait until we get a sensor event with the button sensor as data. */
        PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);

        printf("UART, send\n");
        packetbuf_copyfrom("Hello", 6);
        /* Set the Rime address of the final receiver */
        to.u8[0] = receiver_node_rime_addr[0];
        to.u8[1] = receiver_node_rime_addr[1];

        multihop_send(&multihop, &to);
    }
    PROCESS_END();
}

/*---------------------------------------------------------------------------*/