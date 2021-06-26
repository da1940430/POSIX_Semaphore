all:main.out

main.out: binary_sems.c binary_sems.h get_num.c get_num.h error_functions.c error_functions.h semun.h svshm_xfr.h main.c tlpi_hdr.h
	gcc binary_sems.c binary_sems.h get_num.c get_num.h error_functions.c error_functions.h semun.h svshm_xfr.h main.c tlpi_hdr.h -o main.out -pthread