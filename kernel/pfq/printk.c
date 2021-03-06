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

#include <lang/engine.h>
#include <lang/module.h>

#include <pfq/group.h>
#include <pfq/printk.h>

#include <linux/pf_q.h>


void
pr_devel_group(pfq_gid_t gid)
{
	struct pfq_group * g;
	g = pfq_group_get(gid);
	if (g != NULL) {

		pr_devel("[PFQ] group %d { enabled=%d, policy=%d, pid=%d, owner-id=%d ...}\n",
				gid,
				g->enabled,
				g->policy,
				g->pid,
				g->owner);
	}
}


void
pr_devel_buffer(const unsigned char *buff, size_t len)
{
	pr_devel("[PFQ] %zu [%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x"
		          " %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x"
		          " %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x"
		          " %2x %2x %2x %2x...]\n", len,
			  buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7],
			  buff[8], buff[9], buff[10], buff[11], buff[12], buff[13], buff[14], buff[15],
			  buff[16], buff[17], buff[18], buff[19], buff[20], buff[21], buff[22], buff[23],
			  buff[24], buff[25], buff[26], buff[27], buff[28], buff[29], buff[30], buff[31],
			  buff[32], buff[33]);
}


size_t
snprintf_functional_node(char *buffer, size_t size, struct pfq_lang_functional_node const *node, size_t index)
{
        size_t n, len = 0;

	len += (size_t)snprintf(buffer, (size_t)size, "%4zu@%p: %pF { ", index, node, node->fun.run);

	for(n = 0; n < sizeof(node->fun.arg)/sizeof(node->fun.arg[0]); n++)
	{
		if (size <= len)
			return len;

		if (node->fun.arg[n].nelem != -1U) { /* vector */

			if (node->fun.arg[n].value)
				len += (size_t)snprintf(buffer + len, size - len, "%p[%zu] ",
						(void *)node->fun.arg[n].value, node->fun.arg[n].nelem);
		}
		else {
			if ((node->fun.arg[n].value & 0xffff) == (unsigned)(node->fun.arg[n].value))
				len += (size_t)snprintf(buffer + len, size - len, "%zu ",(size_t)node->fun.arg[n].value);
			else
				len += (size_t)snprintf(buffer + len, size - len, "%p ",(void *)node->fun.arg[n].value);
		}
	}

	if (size <= len)
		return len;

	if (node->fun.next)
		len += (size_t)snprintf(buffer + len, size - len, "} -> next:%p", node->fun.next);
	else
		len += (size_t)snprintf(buffer + len, size - len, "}");

	return len;
}


static void
pr_devel_functional_node(struct pfq_lang_functional_node const *node, size_t index)
{
	char buffer[256];
	snprintf_functional_node(buffer, sizeof(buffer), node, index);
	pr_devel("%s\n", buffer);
}


void
pr_devel_computation_tree(struct pfq_lang_computation_tree const *tree)
{
        size_t n;
	if (tree == NULL) {
		pr_devel("[PFQ] computation (unspecified)\n");
		return;
	}
        pr_devel("[PFQ] binary computation: size=%zu entry_point=%p\n", tree->size, tree->entry_point);
        for(n = 0; n < tree->size; n++)
        {
                pr_devel_functional_node(&tree->node[n], n);
        }
}


static void
pr_devel_functional_descr(struct pfq_lang_functional_descr const *descr, size_t index)
{
	char buffer[256];

        const char *symbol, *signature;
        size_t n, nargs, len = 0, size = sizeof(buffer);

	if (descr->symbol == NULL) {
		pr_devel("%zu   NULL :: ???\n", index);
		return;
	}

        symbol    = strdup_user(descr->symbol);
	signature = pfq_lang_signature_by_user_symbol(descr->symbol);
	nargs     = pfq_lang_number_of_arguments(descr);

	len += (size_t)snprintf(buffer, size, "%3zu   %s :: %s - nargs:%zu [", index, symbol, signature, nargs);

        for(n = 0; n < sizeof(descr->arg)/sizeof(descr->arg[0]) && n < nargs; n++)
	{
		if (size <= len)
			return;

		if (is_arg_function(&descr->arg[n])) {

			if (descr->arg[n].size)
				len += (size_t)snprintf(buffer + len, size - len, "fun(%zu) ", descr->arg[n].size);
		}
		else if (is_arg_vector(&descr->arg[n])) {

			len += (size_t)snprintf(buffer + len, size - len, "pod_%zu[%zu] ",
					descr->arg[n].size, descr->arg[n].nelem);
		}
		else if (is_arg_data(&descr->arg[n])) {

			len += (size_t)snprintf(buffer + len, size - len, "pod_%zu ", descr->arg[n].size);
		}
		else if (is_arg_string(&descr->arg[n])) {
			char * tmp = strdup_user(descr->arg[n].addr);
			len += (size_t)snprintf(buffer + len, size - len, "'%s' ", tmp);
			kfree(tmp);
		}
		else if (is_arg_vector_str(&descr->arg[n])) {
			char * tmp = strdup_user(descr->arg[n].addr);
			len += (size_t)snprintf(buffer + len, size - len, "'str[%zu]' ", descr->arg[n].nelem);
			kfree(tmp);
		}
		else if (!is_arg_null(&descr->arg[n])) {
			len += (size_t)snprintf(buffer + len, size - len, "??? ");
		}
	}

	if (descr->next != -1)
		pr_devel("%s] next(%zu)\n", buffer, descr->next);
	else
		pr_devel("%s]\n", buffer);

        kfree((void *)symbol);
}


void
pr_devel_computation_descr(struct pfq_lang_computation_descr const *descr)
{
	size_t n;
	if (descr == NULL) {
		pr_devel("[PFQ] computation (unspecified)\n");
		return;
	}
        pr_devel("[PFQ] ATS descriptor: size=%zu entry_point=%zu\n", descr->size, descr->entry_point);
        for(n = 0; n < descr->size; n++)
        {
                pr_devel_functional_descr(&descr->fun[n], n);
        }
}

