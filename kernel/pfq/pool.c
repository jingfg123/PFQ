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

#include <core/percpu.h>
#include <core/global.h>

#include <pfq/percpu.h>
#include <pfq/pool.h>
#include <pfq/printk.h>


void pfq_skb_pool_enable(bool value)
{
	int cpu;

	smp_wmb();
	for_each_possible_cpu(cpu)
	{
		struct pfq_percpu_pool *pool = per_cpu_ptr(global->percpu_pool, cpu);
		if (pool)
			atomic_set(&pool->enable, value);
	}
	smp_wmb();
}


int pfq_skb_pool_init_all(void)
{
	int cpu;
	for_each_possible_cpu(cpu)
	{
		struct pfq_percpu_pool *pool = per_cpu_ptr(global->percpu_pool, cpu);
		if (pool) {
			spin_lock_init(&pool->tx_pool_lock);
			if (pfq_skb_pool_init(&pool->tx_pool, global->skb_pool_size) != 0)
				return -ENOMEM;
			if (pfq_skb_pool_init(&pool->rx_pool, global->skb_pool_size) != 0)
				return -ENOMEM;
		}
	}

	return 0;
}


int pfq_skb_pool_free_all(void)
{
	int cpu, total = 0;

	printk(KERN_INFO "[PFQ] flushing skbuff memory pool...\n");
	for_each_possible_cpu(cpu)
	{
		struct pfq_percpu_pool *pool = per_cpu_ptr(global->percpu_pool, cpu);
		if (pool) {
			total += pfq_skb_pool_free(&pool->rx_pool);
			total += pfq_skb_pool_free(&pool->tx_pool);
		}
	}

	return total;
}


int pfq_skb_pool_flush_all(void)
{
	int cpu, total = 0;

	printk(KERN_INFO "[PFQ] flushing skbuff memory pool...\n");
	for_each_possible_cpu(cpu)
	{
		struct pfq_percpu_pool *pool = per_cpu_ptr(global->percpu_pool, cpu);
		if (pool) {
			total += pfq_skb_pool_flush(&pool->rx_pool);
			total += pfq_skb_pool_flush(&pool->tx_pool);
		}
	}

	return total;
}


int pfq_skb_pool_init (struct pfq_skb_pool *pool, size_t size)
{
	if (pool) {
		if (size > 0) {
			pool->skbs = kzalloc(sizeof(struct skb *) * size, GFP_KERNEL);
			if (pool->skbs == NULL) {
				printk(KERN_ERR "[PFQ] pfq_skb_pool_init: out of memory!\n");
				return -ENOMEM;
			}
		}
		else {
			pool->skbs = NULL;
		}

		pool->size  = size;
		pool->p_idx = 0;
		pool->c_idx = 0;
	}
	return 0;
}


size_t
pfq_skb_pool_flush(struct pfq_skb_pool *pool)
{
	size_t n, total = 0;
	if (pool) {
		for(n = 0; n < pool->size; n++)
		{
			if (pool->skbs[n]) {
				total++;
				sparse_inc(global->percpu_mem_stats, os_free);
				kfree_skb(pool->skbs[n]);
				pool->skbs[n] = NULL;
			}
		}

		pool->p_idx = 0;
		pool->c_idx = 0;
	}
	return total;
}


size_t pfq_skb_pool_free(struct pfq_skb_pool *pool)
{
	size_t total = 0;
	if (pool) {
		total = pfq_skb_pool_flush(pool);
		kfree(pool->skbs);
		pool->skbs = NULL;
		pool->size = 0;
	}
	return total;
}


struct core_pool_stat
pfq_get_skb_pool_stats(void)
{
        struct core_pool_stat ret =
        {
		sparse_read(global->percpu_mem_stats, os_alloc),
		sparse_read(global->percpu_mem_stats, os_free),

		sparse_read(global->percpu_mem_stats, pool_alloc),
		sparse_read(global->percpu_mem_stats, pool_free),
		sparse_read(global->percpu_mem_stats, pool_push),
		sparse_read(global->percpu_mem_stats, pool_pop),

                sparse_read(global->percpu_mem_stats, err_norecyl),
                sparse_read(global->percpu_mem_stats, err_pop),
                sparse_read(global->percpu_mem_stats, err_push),
                sparse_read(global->percpu_mem_stats, err_intdis),
                sparse_read(global->percpu_mem_stats, err_shared),
                sparse_read(global->percpu_mem_stats, err_cloned),
                sparse_read(global->percpu_mem_stats, err_memory),
	};
	return ret;
}

