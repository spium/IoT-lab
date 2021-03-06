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
 * $Id: example-unicast.c,v 1.2 2009/03/12 21:58:21 adamdunkels Exp $
 */

/**
 * \file
 *         Best-effort single-hop unicast example
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "net/rime.h"


#include "dev/leds.h"

#include <stdio.h>

/* receiver_node_rime_addr configuration */
#include "common-config.h"


extern process_event_t serial_line_event_message;

/*---------------------------------------------------------------------------*/
PROCESS(example_unicast_process, "Example unicast");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/
static void recv_uc(struct unicast_conn *c, const rimeaddr_t * from)
{
        printf("unicast message received from %d.%d: %.*s\n",
                        from->u8[0], from->u8[1],
                        packetbuf_datalen(), (char *)packetbuf_dataptr());
}
static const struct unicast_callbacks unicast_callbacks = { recv_uc };

static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_unicast_process, ev, data)
{
        static int i = 0;
        static struct etimer et;

        PROCESS_EXITHANDLER(unicast_close(&uc));
        PROCESS_BEGIN();

        unicast_open(&uc, 128, &unicast_callbacks);

        printf("rimeaddr_node_addr = [%u, %u]\n", rimeaddr_node_addr.u8[0],
                        rimeaddr_node_addr.u8[1]);


        /* Put receiver node in listening mode */
        if (rimeaddr_node_addr.u8[0] == receiver_node_rime_addr[0]
                        && rimeaddr_node_addr.u8[1] == receiver_node_rime_addr[1]) {
                /*
                 * Exit process
                 * Received messages handled by 'recv_uc(...)'
                 */
                printf("Receiver node listening\n");
        } else {
                /* Sending messages */
                printf("Write a character on serial link to send message\n");

                PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
                etimer_set(&et, 1 * CLOCK_SECOND);
                while (1) {
                        char msg[64];
                        int len;
                        rimeaddr_t addr;


                        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

                        len = 1 + sprintf(msg, "hello world #%u", i);
                        i++;

                        packetbuf_copyfrom(msg, len);
                        addr.u8[0] = receiver_node_rime_addr[0];
                        addr.u8[1] = receiver_node_rime_addr[1];

                        unicast_send(&uc, &addr);
                        printf("unicast message sent [%i bytes]\n", len);

                        etimer_reset(&et);
                }
        }
        PROCESS_END();
}

/*---------------------------------------------------------------------------*/
