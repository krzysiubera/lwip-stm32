/*
 * tcp_echo.c
 *
 *  Created on: 8 Nov 2022
 *      Author: krzysiu
 */

#include "tcp_echo.h"

static struct tcp_pcb* tcp_echoserver_pcb;

/* ECHO protocol states */
enum tcp_echoserver_states
{
	ES_NONE = 0,
	ES_ACCEPTED,
	ES_RECEIVED,
	ES_CLOSING
};

/* structure for maintaining connection informations to be passed as argument
   to LwIP callbacks*/
struct tcp_echoserver_struct
{
	uint8_t state;			/* current connection state */
	uint8_t retries;
	struct tcp_pcb *pcb;	/* pointer on the current tcp_pcb */
	struct pbuf *p;			/* pointer on the received/to be transmitted pbuf */
};

/* callback functions - they are called internally */
static err_t tcp_echoserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_echoserver_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_echoserver_error(void *arg, err_t err);
static err_t tcp_echoserver_poll(void *arg, struct tcp_pcb *tpcb);
static err_t tcp_echoserver_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);

/* functions */
static void tcp_echoserver_send(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es); //send function
static void tcp_echoserver_connection_close(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es); //close function

/**
  * @brief  Initializes the tcp echo server
  * @param  None
  * @retval None
  */
err_t tcp_echoserver_init(void)
{
	/* Create new tcp pcb */
	tcp_echoserver_pcb = tcp_new();

	/* Check if memory was allocated */
	if (tcp_echoserver_pcb == NULL)
	{
		memp_free(MEMP_TCP_PCB, tcp_echoserver_pcb);
		return ERR_MEM;
	}

	err_t err;

	/* Bind echo_pcb to port ECHO_SERVER_LISTEN_PORT */
	err = tcp_bind(tcp_echoserver_pcb, IP_ADDR_ANY, ECHO_SERVER_LISTEN_PORT);

	/* Check if port was binded */
	if (err != ERR_OK)
	{
		memp_free(MEMP_TCP_PCB, tcp_echoserver_pcb);
		return err;
	}

	/* Start TCP listening for echo_pcb */
	tcp_echoserver_pcb = tcp_listen(tcp_echoserver_pcb);

	/* Initialize LwIP tcp_accept callback function */
	tcp_accept(tcp_echoserver_pcb, tcp_echoserver_accept);

	return ERR_OK;
}

/**
  * @brief  This function is the implementation of tcp_accept LwIP callback
  * @param  arg: not used
  * @param  newpcb: pointer on tcp_pcb struct for the newly created tcp connection
  * @param  err: not used
  * @retval err_t: error status
  */
static err_t tcp_echoserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	struct tcp_echoserver_struct *es;

	/* Remove warnings for unused arguments */
	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(err);

	/* Set priority for the newly accepted TCP connection newpcb */
	tcp_setprio(newpcb, TCP_PRIO_NORMAL);

	/* Allocate structure es to maintain TCP connection informations */
	es = (struct tcp_echoserver_struct*) mem_malloc(sizeof(struct tcp_echoserver_struct));

	/* Check if allocation went fine */
	if (es == NULL)
	{
		/* Close TCP connection */
		tcp_echoserver_connection_close(newpcb, es);

		/* Return memory error */
		return ERR_MEM;
	}

	es->state = ES_ACCEPTED; 		/* set connection as accepted */
	es->pcb = newpcb;				/* set PCB pointer */
	es->retries = 0;				/* clear retries counter */
	es->p = NULL; 					/* clear buffer pointer */

	/* Pass newly allocated es structure as argument to newpcb */
	tcp_arg(newpcb, es);

	/* Initialize lwIP tco_recv callback function for newpcb.
	 * This callback handles all the data traffic with the remote client*/
	tcp_recv(newpcb, tcp_echoserver_recv);

	/* Initialize lwIP tcp_err callback function for newpcb
	 * This callback handles TCP errors*/
	tcp_err(newpcb, tcp_echoserver_error);

	/* Initialize lwIP tcp_poll callback function for newpcb
	 * This callback handles periodic application tasks
	 * (such as checking if the application data remains to be transmitted)*/
	tcp_poll(newpcb, tcp_echoserver_poll, 0);

	return ERR_OK;
}

/**
  * @brief  This function is the implementation for tcp_recv LwIP callback
  * @param  arg: pointer on a argument for the tcp_pcb connection
  * @param  tpcb: pointer on the tcp_pcb connection
  * @param  pbuf: pointer on the received pbuf
  * @param  err: error information regarding the reveived pbuf
  * @retval err_t: error code
  */
static err_t tcp_echoserver_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	struct tcp_echoserver_struct *es;
	err_t ret_err;

	LWIP_ASSERT("arg != NULL", arg != NULL); //check argument
	es = (struct tcp_echoserver_struct*) arg;

	/* if we receive an empty TCP frame - close connection */
	if (p == NULL)
	{
		/* remote host closed connection */
		es->state = ES_CLOSING;
		if (es->p == NULL)
		{
			/* done with sending - close connection */
			tcp_echoserver_connection_close(tpcb, es);
		}
		else
		{
			/* we're not done with sending data */
			/* acknowledge received packet */
			tcp_sent(tpcb, tcp_echoserver_sent);

			/* send remaining data */
			tcp_echoserver_send(tpcb, es);
		}
		ret_err = ERR_OK;
	}
	/* else : a non empty frame was received from client but for some reason err != ERR_OK */
	else if (err != ERR_OK) //error when receiving
	{
		/* free received pbuf */
		if (p != NULL)
		{
			es->p = NULL;
			pbuf_free(p);
		}
		ret_err = err;
	}
	/* First data chunk in p->payload */
	else if (es->state == ES_ACCEPTED)
	{
		/* change state to received */
		es->state = ES_RECEIVED;

		/* store reference to incoming pbuf (chain) */
		es->p = p;

		/* initialize LwIP tcp_sent callback function */
		tcp_sent(tpcb, tcp_echoserver_sent);

		/* send back the received data (echo) */
		tcp_echoserver_send(tpcb, es);

		ret_err = ERR_OK;
	}
	/* more data received from client and previous data has been already sent*/
	else if (es->state == ES_RECEIVED) //additional data receiving
	{
		/* No data to send */
		if (es->p == NULL)
		{
			/* Set buffer pointer */
			es->p = p;

			/* Send back received data */
			tcp_echoserver_send(tpcb, es);
		}
		/* buffer is not empty, there's data to send */
		else
		{
			/* chain pbufs to the end of what we recv'ed previously
			* This buffer will be called by the poll callback */
			struct pbuf *ptr = es->p;
			pbuf_chain(ptr, p);
		}
		ret_err = ERR_OK;
	}
	/* receiving data when connection is closing */
	else if (es->state == ES_CLOSING)
	{
		/* odd case, remote side closing twice, trash data */
		tcp_recved(tpcb, p->tot_len);
		es->p = NULL;
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	/* undefined condition */
	else
	{
		/* unknown es->state, trash data  */
		tcp_recved(tpcb, p->tot_len);	//advertise window size
		es->p = NULL;
		pbuf_free(p);
		ret_err = ERR_OK;
	}
	return ret_err;
}

/**
  * @brief  This function implements the tcp_err callback function (called
  *         when a fatal tcp_connection error occurs.
  * @param  arg: pointer on argument parameter
  * @param  err: not used
  * @retval None
  */
static void tcp_echoserver_error(void *arg, err_t err)
{
	struct tcp_echoserver_struct *es;
	LWIP_UNUSED_ARG(err);

	es = (struct tcp_echoserver_struct*) arg;
	if (es != NULL)
	{
		/* free es structure */
		mem_free(es);
	}

	/* turn on blue LED */
	HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_SET);
}

/**
  * @brief  This function implements the tcp_poll LwIP callback function
  * @param  arg: pointer on argument passed to callback
  * @param  tpcb: pointer on the tcp_pcb for the current tcp connection
  * @retval err_t: error code
  */
static err_t tcp_echoserver_poll(void *arg, struct tcp_pcb *tpcb)
{
	struct tcp_echoserver_struct *es;
	es = (struct tcp_echoserver_struct*) arg;

	/* if there is no es structure */
	if (es == NULL)
	{
		/* Abort connection */
		tcp_abort(tpcb);
		return ERR_ABRT;
	}
	/* if there is still data to send */
	if (es->p != NULL)
	{
		/* register send callback */
		tcp_sent(tpcb, tcp_echoserver_sent);

		/* there is a remaining pbuf (chain) , try to send data */
		tcp_echoserver_send(tpcb, es);
	}
	/* no data to send */
	else
	{
		if (es->state == ES_CLOSING)
		{
			/* Close TCP connection */
			tcp_echoserver_connection_close(tpcb, es);
		}
	}

	return ERR_OK;
}

/**
  * @brief  This function implements the tcp_sent LwIP callback (called when ACK
  *         is received from remote host for sent data)
  * @param  None
  * @retval None
  */
static err_t tcp_echoserver_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	struct tcp_echoserver_struct *es;
	LWIP_UNUSED_ARG(len);

	es = (struct tcp_echoserver_struct*) arg;
	es->retries = 0;

	if (es->p != NULL)
	{
		/* Still got pbufs to send */
		tcp_sent(tpcb, tcp_echoserver_sent);
		tcp_echoserver_send(tpcb, es);
	}
	else
	{
		/* if no more data to send and client closed connection*/
		if (es->state == ES_CLOSING)
		{
			tcp_echoserver_connection_close(tpcb, es);
		}
	}
	return ERR_OK;
}

/**
  * @brief  This function is used to send data for tcp connection
  * @param  tpcb: pointer on the tcp_pcb connection
  * @param  es: pointer on echo_state structure
  * @retval None
  */
static void tcp_echoserver_send(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es)
{
	struct pbuf *ptr;
	err_t wr_err = ERR_OK;

	/* while no error, data to send, data size is smaller than the size of the send buffer */
	while ((wr_err == ERR_OK) && (es->p != NULL)
	  && (es->p->len <= tcp_sndbuf(tpcb)))
	{
		/* get pointer on pbuf from es structure */
		ptr = es->p;

		/* enqueue data for transmission */
		wr_err = tcp_write(tpcb, ptr->payload, ptr->len, TCP_WRITE_FLAG_COPY);

		if (wr_err == ERR_OK)
		{
			u16_t plen;
			u8_t freed;

			plen = ptr->len;

			/* continue with next pbuf in chain (if any) */
			es->p = ptr->next;

			/* there's chained buffer to send */
			if (es->p != NULL)
			{
				/* increment reference count foe es->p */
				pbuf_ref(es->p);
			}

			/* chop first pbuf from chain */
			do
			{
				/* try to free pbuf */
				freed = pbuf_free(ptr);
			}
			while (freed == 0);

			/* we can read more data now */
			tcp_recved(tpcb, plen);
		}
		else if(wr_err == ERR_MEM)
		{
		    /* we are low on memory, try later / harder, defer to poll */
		    es->p = ptr;
		}
		else
		{
		 /* other problem ?? */
		}
	}
}

/**
  * @brief  This functions closes the tcp connection
  * @param  tcp_pcb: pointer on the tcp connection
  * @param  es: pointer on echo_state structure
  * @retval None
  */
static void tcp_echoserver_connection_close(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es)
{
	/* clear callback functions */
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);

	/* delete es structure */
	if (es != NULL)
	{
		mem_free(es);
	}

	/* close tcp connection */
	tcp_close(tpcb);
}

