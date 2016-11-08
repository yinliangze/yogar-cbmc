/*
 * eog.h
 *
 *  Created on: Jan 4, 2016
 *      Author: ylz86
 * Description: describe an event order graph, used for judging whether a counterexample is feasible.
 */

#ifndef CBMC_EOG_H_
#define CBMC_EOG_H_

#include <stddef.h>
#include <cassert>
#include <list>
#include <vector>
#include <iostream>
#include <map>
#include <util/time_stopping.h>
#include <string.h>

#include "../goto-symex/symex_target_equation.h"
#include "bit_int.h"

class node;
class edge;
class pa_son_set;
class na_info;

typedef symex_target_equationt::SSA_stept eventt;
typedef symex_target_equationt::SSA_stepst eventst;
typedef eventst::const_iterator event_it;
typedef std::list<node*> nodest;
typedef std::vector<node*> nodevt;
typedef std::list<edge*> edgest;
typedef std::map<node*, edgest> pa_son_mapt;  // first is the parent or son node, second is the reason rf edges
typedef std::map<irep_idt, nodevt> address_rw_mapt;
typedef std::map< irep_idt, std::vector<int> > address_nodes_mapt;
typedef std::map< node*, nodevt > node_edge_mapt;
typedef std::map<edge*, na_info> na_info_mapt;

extern time_periodt ttt;

class pa_son_set {
public:
	bit_int pa_son_info;

	pa_son_set(unsigned nodes_num) {
		pa_son_info.init(nodes_num);
	}

	void clear(unsigned nodes_num) {
		pa_son_info.clear();
	}

	~pa_son_set() {}
};

class node {
public:
	int m_id;
	eventt* m_event;

	edgest m_input_pos;
	edgest m_input_epos;
	edgest m_input_rfs;
	edgest m_input_nas;

	edgest m_output_pos;
	edgest m_output_epos;
	edgest m_output_rfs;
	edgest m_output_nas;

	bool m_search_flag;    // true if the node has been searched, false otherwise
	pa_son_set* parents;
	pa_son_set* sons;
	bit_int po_sons;

	node (int id, eventt* e): m_event(e) {
		m_id = id;
		m_search_flag = false;
		parents = NULL;
		sons = NULL;
	}

	node (eventt* e): m_event(e) {
		m_id = -1;
		m_search_flag = false;
		parents = NULL;
		sons = NULL;
	}

	void clear(int nodes_num) {
		m_id = -1;

		m_input_pos.clear();
		m_input_epos.clear();
		m_input_rfs.clear();
		m_input_nas.clear();

		m_output_pos.clear();
		m_output_epos.clear();
		m_output_rfs.clear();
		m_output_nas.clear();

		m_search_flag = false;

		parents->clear(nodes_num);
		sons->clear(nodes_num);
	}

	~node() {
		m_input_pos.clear();
		m_input_epos.clear();
		m_input_rfs.clear();
		m_input_nas.clear();

		m_output_pos.clear();
		m_output_epos.clear();
		m_output_rfs.clear();
		m_output_nas.clear();

		if (parents != NULL)
			delete parents;
		if (sons != NULL)
			delete sons;
	}

	bool no_input_edge() {
		return m_input_pos.empty() && m_input_epos.empty() && m_input_rfs.empty() && m_input_nas.empty();
	}

	bool is_w() {
		return m_event->is_shared_write();
	}

	bool is_r() {
		return m_event->is_shared_read();
	}

	bool same_address(node * n) {
		return m_event->original_lhs_object.get_identifier() == n->m_event->original_lhs_object.get_identifier();
	}

	bool same_thread(node* n) {
		return m_event->source.thread_nr == n->m_event->source.thread_nr;
	}

	irep_idt address() const
	{
		return m_event->original_lhs_object.get_identifier();
	}

	void const output() {
		if (m_event->is_shared_read() || m_event->is_shared_write())
			std::cout << m_event->ssa_lhs.get_identifier();
		else if (m_event->is_spawn())
			std::cout << &(*(m_event));
		std::cout << "\n";
	}

	void const outputx() {
		if (m_event->is_shared_read() || m_event->is_shared_write())
			std::cout << m_event->ssa_lhs.get_identifier();
		else if (m_event->is_spawn())
			std::cout << &(*(m_event));
		std::cout << " ";
	}

	void get_parents(std::vector<int>& vec) {
		parents->pa_son_info.get_one_bits(vec);
	}

	void get_sons(std::vector<int>& vec) {
		sons->pa_son_info.get_one_bits(vec);
	}
};

class edge {
public:
	node* m_src;
	node* m_dst;

	edgest m_reasons;  // for na edge, it is the reason rfs for the edge
	bool m_cycle_valid;  // true if it is valid in construct a cycle, false if it is duplicate with po

	enum edge_type {
		PO,    // normal program-order edge
		EPO,   // equal program-order edge, for mutex variables
		RF,    // read from edge
		NA     // new add edge
	};
	edge_type m_type;

	edge(node* src, node* dst, edge_type type): m_src(src), m_dst(dst), m_type(type), m_cycle_valid(true) {
		if (type == RF) {
			m_reasons.push_back(this);
		}
	}

	void output() {
		std::cout << "(";
		if (m_src->m_event->is_shared_read() || m_src->m_event->is_shared_write())
			std::cout << m_src->m_event->ssa_lhs.get_identifier();
		else if (m_src->m_event->is_spawn())
			std::cout << &(*(m_src->m_event));

		std::cout << ", " ;
		if (m_dst->m_event->is_shared_read() || m_dst->m_event->is_shared_write())
			std::cout << m_dst->m_event->ssa_lhs.get_identifier();
		else if (m_dst->m_event->is_spawn())
			std::cout <<  &(*(m_dst->m_event));
		std::cout << ")\n";
	}

	void outputx() {
		std::cout << "(";
		if (m_src->m_event->is_shared_read() || m_src->m_event->is_shared_write())
			std::cout << m_src->m_event->ssa_lhs.get_identifier();
		else if (m_src->m_event->is_spawn())
			std::cout << &(*(m_src->m_event));

		std::cout << ", " ;
		if (m_dst->m_event->is_shared_read() || m_dst->m_event->is_shared_write())
			std::cout << m_dst->m_event->ssa_lhs.get_identifier();
		else if (m_dst->m_event->is_spawn())
			std::cout <<  &(*(m_dst->m_event));
		std::cout << ")";
	}
};

class na_info {
public:
	bool flag;   // flag = false for case 1,2, flag = true for case 3,4

	// for case 1, 2
	node* less_src_parent;
	node* less_src;
	node* less_dst;
	node* less_dst_son;
	edge* less_e;
	edge* rf;

	na_info(){}

	na_info(node*_src_p, node* _src, node* _dst, node* _dst_s, edge* _e, edge* _rf):
		less_src_parent(_src_p),
		less_src(_src),
		less_dst(_dst),
		less_dst_son(_dst_s),
		less_e(_e),
		rf(_rf),
		flag(true)
	{

	}

	na_info(node* _src, node* _dst, edge* _rf):
		less_src_parent(NULL),
		less_src(_src),
		less_dst(_dst),
		less_dst_son(NULL),
		less_e(NULL),
		rf(_rf),
		flag(false)
	{

	}
};

class eog {
public:
	time_periodt totaltime;
	int reason_num;
	unsigned nodes_num;

	nodevt m_all_nodes;	// all original nodes

	nodest m_nodes;    	// node list
	edgest m_pos;		// po list
	edgest m_epos;		// epo list
	edgest m_rfs;		// rf list
	edgest m_nas;		// new added edge list

	nodevt m_vnodes;	// vector of nodes

	std::map<const eventt*, node*> event_node_map;

	bool optimize;

	// used for find all cycles
	std::vector<node*> m_trace;
	std::vector<edge*> m_trace_edge;
	std::vector< std::vector<edge*> > m_reasons;

	address_nodes_mapt m_addr_nodes_map_w;
	address_nodes_mapt m_addr_nodes_map_r;
	address_nodes_mapt m_addr_nodes_map;

	void initial_addr_nodes_map() {
		for (unsigned i = 0; i < m_vnodes.size(); i++) {
			m_addr_nodes_map[m_vnodes[i]->address()].push_back(i);
			if (m_vnodes[i]->is_r()) {
				m_addr_nodes_map_r[m_vnodes[i]->address()].push_back(i);
				m_addr_nodes_map_r["all"].push_back(i);
			}
			if (m_vnodes[i]->is_w()) {
				m_addr_nodes_map_w[m_vnodes[i]->address()].push_back(i);
				m_addr_nodes_map_w["all"].push_back(i);
			}
		}
	}

	void update_po_sons() {
		for (unsigned i = 0; i < m_vnodes.size(); i++) {
			m_vnodes[i]->po_sons = m_vnodes[i]->sons->pa_son_info;
		}
	}

	void clear();

	int get_edges_num() {
		return m_pos.size() + m_epos.size() + m_rfs.size() + m_nas.size();
	}

	void add_node(eventt* event) {
		node* n = m_all_nodes[event->id];
		n->m_id = nodes_num++;
		m_nodes.push_back(n);
		m_vnodes.push_back(n);
	}

	void add_all_node(eventt* event) {
		node* n = new node(event);
		m_all_nodes.push_back(n);
		event_node_map[event] = n;
	}

	edge* add_edge(const eventt* src, const eventt* dst, edge::edge_type type) {
		assert(type == edge::PO || type == edge::EPO || type == edge::RF);

		node* nsrc = event_node_map[src];
		node* ndst = event_node_map[dst];

		edge* e = new edge(nsrc, ndst, type);

		if (type == edge::PO || type == edge::EPO) {
			add_po_edge(e);
		}
		else if (type == edge::RF) {
			add_rf_edge(e);
		}
		else {
			assert(false);
		}

		return e;
	}

	void add_edge(edge* e) {
		switch(e->m_type) {
		case edge::PO:
			m_pos.push_back(e);
			e->m_src->m_output_pos.push_back(e);
			e->m_dst->m_input_pos.push_back(e);
//			std::cout << "PO: "; e->output();
			break;
		case edge::EPO:
			m_epos.push_back(e);
			e->m_src->m_output_epos.push_back(e);
			e->m_dst->m_input_epos.push_back(e);
//			std::cout << "EPO: "; e->output();
			break;
		case edge::RF:
			m_rfs.push_back(e);
			e->m_src->m_output_rfs.push_back(e);
			e->m_dst->m_input_rfs.push_back(e);
//			if (!e->m_cycle_valid)
//				std::cout << "unvalid ";
//			std::cout << "RF: "; e->output();
			break;
		case edge::NA:
			m_nas.push_back(e);
			e->m_src->m_output_nas.push_back(e);
			e->m_dst->m_input_nas.push_back(e);
//			std::cout << "NA: "; e->output();

//			std::cout << "reasons: ";
//			for (edgest::iterator it = e->m_reasons.begin(); it != e->m_reasons.end(); it++)
//				(*it)->outputx();
//			std::cout << "\n";
			break;
		default:
			assert(false);
		}

	}

	// judge if there exists some circle among the graph
	bool no_circle_judge();

	// compute all cycles of the graph
	void compute_all_cycles();

	// compute rw events of a variable from a pa_son_set
	void get_var_nodes(pa_son_set& set, const std::vector<int>& vec, nodevt& nodes);

	// compute the reason rfs of why dst is a son of src
	void compute_son_reason(node* src, node* dst, edgest& son_reason);

	void merge_reason(node* src_parent, node* src, node* dst, node* dst_son, edge* e, edgest& reason);

	// judge if n2 is a son of n1
	bool is_son_of(const node* n1, const node* n2);

	// judge if n2 is a son of n1
	bool is_po_son_of(node* n1, const node* n2);

	void init_pa_sons();

public:
	eog();
	virtual ~eog();

private:
	// find all cycles starting from a node
	void find_cycle_from_node(node* v);

	// delete all the duplicate and ineffective reasons
	void process_reasons();

	// add a po or na edge to the graph
	void add_po_edge(edge* e);

	void update_pa_son(edge* e);

	void compute_rw_map(node* n, address_rw_mapt& rw_map_dst_son_r, address_rw_mapt& rw_map_dst_son_w);
	bool is_redundant(node* src, node* dst, edgest& to_be_add_edges);
	// add a rf edge to the graph
	void add_rf_edge(edge* e);
	void add_na_edge(node* nsrc, node* ndst, na_info& info, na_info_mapt& na_info_map, edgest& to_be_add_edges);
	void compute_edge_reason(edge* na, na_info& info);
	edgest::iterator edgest_merge(edgest& tmp, edgest& tmp1);
	edgest::iterator edgest_merge(edgest& tmp, edgest& tmp1, edgest& tmp2);
	pa_son_set* update_parent_from(const node* parent_node, pa_son_set& parents_set);
	pa_son_set* update_son_from(const node* son_node, pa_son_set& sons_set);
	pa_son_set* update_node_parent_from(const node* parent_node, pa_son_set& parents_set);
	pa_son_set* update_node_son_from(const node* son_node, pa_son_set& sons_set);
};

#endif /* CBMC_EOG_H_ */
