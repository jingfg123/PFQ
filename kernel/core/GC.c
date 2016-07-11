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
#include <core/qbuff.h>
#include <core/GC.h>


void GC_reset(struct GC_data *gc)
{
	size_t n;
	for(n = 0; n < gc->pool.len; ++n)
	{
		GC_log_init(&gc->log[n]);
	}
	gc->pool.len = 0;
}


struct qbuff *
GC_make_buff(struct GC_data *gc, void *addr)
{
	struct qbuff * ret;

	if (gc->pool.len >= Q_CORE_BUFF_QUEUE_LEN) {
		ret = NULL;
	}
	else {
		ret = & gc->pool.queue[gc->pool.len];
		ret->addr = addr;
                ret->log  = &gc->log[gc->pool.len++];
	}

	return ret;
}


void
GC_get_lazy_endpoints(struct GC_data *gc, struct core_endpoint_info *ts)
{
	size_t n, i;

	ts->num = 0;
        ts->cnt_total = 0;

	for(n = 0; n < gc->pool.len; ++n)
	{
		for(i = 0; i < gc->log[n].num_devs; i++)
		{
			core_add_dev_to_endpoints(gc->log[n].dev[i], ts);
		}
	}
}
