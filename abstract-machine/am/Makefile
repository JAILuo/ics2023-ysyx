NAME     := am
SRCS      = $(addprefix src/, $(AM_SRCS))
INC_PATH += $(AM_HOME)/am/src

include $(AM_HOME)/Makefile

log:
	make -nB \
        | grep -ve '^\(\#\|echo\|mkdir\|make\)' \
        | sed "s#$(AM_HOME)#\AM#g" \
        | sed "s#$(PWD)#.#g" \
        #> compile.log


