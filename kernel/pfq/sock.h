/***************************************************************
 *
 * (C) 2011-16 Nicola Bonelli <nicola@pfq.io>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 ****************************************************************/

#ifndef PFQ_SOCK_H
#define PFQ_SOCK_H

#include <pfq/atomic.h>
#include <pfq/define.h>
#include <pfq/endpoint.h>
#include <pfq/kcompat.h>
#include <pfq/pool.h>
#include <pfq/shmem.h>
#include <pfq/sock.h>
#include <pfq/stats.h>
#include <pfq/thread.h>
#include <pfq/types.h>

#include <linux/wait.h>

#ifdef __KERNEL__
#include <net/sock.h>
#endif


extern void pfq_sock_init_once(void);
extern void pfq_sock_fini_once(void);
extern void pfq_sock_init_waitqueue_head(wait_queue_head_t *queue);
extern void pfq_sock_destruct(struct sock *sk);

#define for_each_sk_mbuff(hdr, end, fix) \
        for(; (hdr < (struct pfq_pkthdr *)end); \
               hdr = PFQ_SHARED_QUEUE_NEXT_PKTHDR(hdr, fix))


struct pfq_queue_info
{
	atomic_long_t		addr;			/* (pfq_shared_tx_queue *) */
	void			*shmem_addr;
	int			ifindex;		/* ifindex */
	int			queue;			/* queue */
};


static inline
void pfq_txq_info_init(struct pfq_queue_info *info)
{
	atomic_long_set(&info->addr, 0);
	info->shmem_addr = NULL;
	info->ifindex = -1;
	info->queue = -1;
}


static inline
void pfq_rxq_info_init(struct pfq_queue_info *info)
{
        atomic_long_set(&info->addr, 0);
        info->shmem_addr = NULL;
}


struct pfq_sock
{
        struct sock		sk;
        pfq_id_t		id;

	int			egress_type;
        int			egress_index;
        int			egress_queue;
	int			weight;
	int			tstamp;

	size_t			caplen;

	size_t			rx_queue_len;
	size_t			rx_slot_size;

	size_t			tx_queue_len;
	size_t			tx_slot_size;

	wait_queue_head_t	waitqueue;

        size_t			txq_num_async;

	struct pfq_queue_info	tx_async[Q_MAX_TX_QUEUES];
	struct pfq_queue_info	tx;
	struct pfq_queue_info	rx;

	struct pfq_shmem_descr  shmem;

        pfq_sock_stats_t __percpu *stats;

} ____pfq_cacheline_aligned;


/* queue info */

static inline
struct pfq_queue_info *
pfq_sock_get_rx_queue_info(struct pfq_sock *so)
{
	return &so->rx;
}

static inline
struct pfq_queue_info *
pfq_sock_get_tx_queue_info(struct pfq_sock *so, int index)
{
	if (index == -1)
	    return &so->tx;
	return &so->tx_async[index];
}

/* queues */

static inline
struct pfq_shared_rx_queue *
pfq_sock_shared_rx_queue(struct pfq_sock *so)
{
	return (struct pfq_shared_rx_queue *)atomic_long_read(&so->rx.addr);
}

static inline
struct pfq_shared_tx_queue *
pfq_sock_shared_tx_queue(struct pfq_sock *so, int index)
{
	if (index == -1)
		return (struct pfq_shared_tx_queue *)atomic_long_read(&so->tx.addr);
	return (struct pfq_shared_tx_queue *)atomic_long_read(&so->tx_async[index].addr);
}

/* memory mapped queues */

static inline
struct pfq_shared_queue *
pfq_sock_shared_queue(struct pfq_sock *p)
{
        return (struct pfq_shared_queue *) atomic_long_read(&p->shmem.addr);
}


static inline struct pfq_sock *
pfq_sk(struct sock *sk)
{
        return (struct pfq_sock *)(sk);
}


extern pfq_id_t pfq_sock_get_free_id(struct pfq_sock * so);
extern int	pfq_sock_init(struct pfq_sock *so, pfq_id_t id, size_t caplen, size_t maxlen);
extern struct	pfq_sock * pfq_sock_get_by_id(pfq_id_t id);
extern int	pfq_sock_counter(void);
extern void	pfq_sock_release_id(pfq_id_t id);
extern int	pfq_sock_tx_bind(struct pfq_sock *so, int tid, int if_index, int queue);
extern int	pfq_sock_tx_unbind(struct pfq_sock *so);

extern int	pfq_sock_enable(struct pfq_sock *so, struct pfq_so_enable *mem);
extern int	pfq_sock_disable(struct pfq_sock *so);


#endif /* PFQ_SOCK_H */
