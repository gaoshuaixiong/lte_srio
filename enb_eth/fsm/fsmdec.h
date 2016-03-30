#include "fsmcore.h"

#ifndef DEF_FSM_DEC
#define DEF_FSM_DEC


// define STATE_VAR as the type of starte variable struct
// define FSM_MAIN as the name of fsm main


typedef struct {
	int		_fsm_num_calls;
	const char*	_fsm_func_name;
}fsm_fstack_info;


#define FIN_CORE(fname)														\
	static fsm_fstack_info  _fsm_fstack_static_info = {0,#fname};			

#define FIN(fname) 															\
	FIN_CORE(fname)

#define FOUT 				\
	{						\
    return;					\
	}
#define FRET(rval)			\
	{						\
	return rval;			\
	}



#define FSM_ENTER(pr_name_str)											\
	int	_fsm_num_true_cond, _fsm_true_cond_index, _fsm_cond_index;					\
	int* 	_fsm_current_state_ptr;										\
	FIN(pr_name_str);											\
	_fsm_current_state_ptr = fsm_current_state_ptr_get();	

/* Main switch statement for process model code. */


#define FSM_BLOCK_SWITCH		switch (*_fsm_current_state_ptr)


#define nofunc

#define FSM_STATE_FORCED(state_id,state_name_str,enter_exec,exit_exec)	    \
	FSM_STATE_ENTER_FORCED(state_id,state_name_str)		\
	{enter_exec;}										\
	FSM_STATE_EXIT_FORCED(state_id,state_name_str)		\
	{exit_exec;}

#define FSM_STATE_UNFORCED(state_id,state_name_str,enter_exec,exit_exec)	\
	FSM_STATE_ENTER_UNFORCED(state_id,state_name_str)	\
	{enter_exec;}										\
	FSM_EXIT(2*state_id+1)								\
	FSM_STATE_EXIT_UNFORCED(state_id,state_name_str)	\
	{exit_exec;}

#define FSM_STATE_ENTER_FORCED(sn,state_name_str)	\
	FSM_STATE_ENTER(sn,state_name_str)

#define FSM_STATE_ENTER_UNFORCED(sn,state_name_str)	\
	FSM_STATE_ENTER(sn,state_name_str)

#define FSM_STATE_EXIT_FORCED(sn,state_name_str)	\
	*_fsm_current_state_ptr = 2 * sn + 1;	\
	FSM_STATE_EXIT(sn, state_name_str)

#define FSM_STATE_EXIT_UNFORCED(sn,state_name_str)	\
	FSM_STATE_EXIT(sn, state_name_str)

#define FSM_STATE_ENTER(bn,state_name_str)			\
	case (2 * bn):									\
		{											\
		_fsm_label_state ## bn:						\
		FSM_BLOCK_NAME_SET(state_name_str)			\
		}															

#define FSM_STATE_EXIT(sn,state_name_str)			\
	case (sn * 2 + 1):								\
		{											\
		FSM_BLOCK_NAME_SET(state_name_str)			\
		}

#define FSM_BLOCK_NAME_SET(block_name_str)						\
		_fsm_fstack_static_info._fsm_func_name = block_name_str;

#define FSM_EXIT(nb)								\
	*_fsm_current_state_ptr = nb;			\
	FOUT;

#define FSM_TRANSIT_FORCE(dest_state, exec, cond_str, exec_str, fin_name_str )	\
	{														\
	FSM_TRANSIT_BLOCK_NAME_SET(fin_name_str)				\
	{exec;}													\
	*_fsm_current_state_ptr = 2 * dest_state;		\
	FSM_TRANSIT(dest_state)					\
	}

#define FSM_TRANSIT_BLOCK_NAME_SET(fin_name_str)			\
	_fsm_fstack_static_info._fsm_func_name = fin_name_str;

#define FSM_COND_TEST_IN(state_name_str)	\
	_fsm_cond_index = 0;					\
	_fsm_num_true_cond = 0;					\
	_fsm_true_cond_index = -1;		

#define FSM_TEST_COND(cond)						\
	if (cond)									\
		{										\
		_fsm_num_true_cond++;					\
		_fsm_true_cond_index = _fsm_cond_index;	\
		}										\
    _fsm_cond_index++;

#define FSM_COND_TEST_OUT(state_name_str)				\
	if(_fsm_num_true_cond > 1)							\
	{													\
		ErrorLog(state_name_str,"more condition ture")	\
		_fsm_true_cond_index = -1;						\
	}													\
	else if(_fsm_num_true_cond == 0)			\
		_fsm_true_cond_index = -1;			

#define FSM_TRANSIT_SWITCH	switch (_fsm_true_cond_index)



#define FSM_TEST_LOGIC(st_name_str)								\
	if (_fsm_num_true_cond != 1)								\
		sim_pro_err_transit (st_name_str, _fsm_num_true_cond);


#define FSM_CASE_TRANSIT(trans_index, dest_state, exec, fin_name_str)	\
	case trans_index:											\
		{														\
		FSM_TRANSIT_BLOCK_NAME_SET(fin_name_str)				\
		{exec;}													\
		*_fsm_current_state_ptr = 2 * dest_state;		\
		FSM_TRANSIT(dest_state)									\
		}

#define FSM_TRANSIT(dest_state)				\
	goto _fsm_label_state ## dest_state;					


#define FSM_CASE_DEFAULT(dest_state, exec, fin_name_str)		\
		default:						\
			{						\
			FSM_TRANSIT_BLOCK_NAME_SET(fin_name_str)	\
			{exec;}						\
			*_fsm_current_state_ptr = 2 * dest_state;	\
			FSM_TRANSIT(dest_state)				\
			}

#define ErrorLog(state_str,err_str)    fsm_printf("ERROR in %s: %s\n",state_str,err_str);

#define SV_PTR_GET(type)	\
	type *sv_ptr;		\
	sv_ptr = fsm_sv_ptr_get()
	
#define SV(x)	sv_ptr->x


#endif
