/*
 * Copyright  2008-2009 INRIA/SensTools
 *
 * <dev-team@sentools.info>
 *
 * This software is a set of libraries designed to develop applications
 * for the WSN430 embedded hardware platform.
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */

#ifndef _TDMA_NODE_H
#define _TDMA_NODE_H

/**
 * The MAC address associated to this node.
 */
extern uint16_t mac_addr;
/**
 * Create the MAC task.
 * \param xSPIMutex the mutex to access the radio SPI
 */
void mac_create_task(xSemaphoreHandle xSPIMutex);

// Management
/**
 * Set a callback function pointer, to be called when a new node has associated.
 * \param handler the function pointer.
 */
void mac_set_node_associated_handler(void (*handler)(uint16_t node));

/**
 * Set a callback function pointer, to be called when new data is received from a node.
 * \param handler the function pointer
 */
void mac_set_data_received_handler(void (*handler)(uint16_t node, uint8_t* data, uint16_t length));

/**
 * Set a callback function pointer, to be called when a beacon is sent.
 * \param handler the function pointer
 */
void mac_set_beacon_handler(void (*handler)(uint8_t id, uint16_t timestamp));

/**
 * Send data to a node. Maximum length is 15 bytes per send.
 * \param node the destination node address
 * \param data a pointer to the data to send
 * \param length the number of bytes to send
 * \return 0 if send was not possible, 1 if it was
 */
uint16_t mac_send(uint16_t node, uint8_t* data, uint16_t length);

#endif
