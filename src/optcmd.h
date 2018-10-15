/* Copyright 2016 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __OPTCMD_H
#define __OPTCMD_H

#include "ccan/check_type/check_type.h"
#include "ccan/build_assert/build_assert.h"
#include "ccan/cppmagic/cppmagic.h"
#include "config.h"

#include "parsers.h"

#ifndef OPTCMD_MAX_ARGS
#define OPTCMD_MAX_ARGS 10
#endif

#ifndef OPTCMD_MAX_FLAGS
#define OPTCMD_MAX_FLAGS 10
#endif

/*
 * The idea of this module is to assist with easily calling C functions from the
 * command line without requiring shim/boilerplate functions to marshal
 * arguments and flags correctly which can be repetitive and error prone.
 *
 * In general a command consists of positional arguments and non-positional
 * flags which optionally take arguments as shown below:
 *
 * `cmd_name <arg1> <arg2> ... <argN> --flag1=<arg?> --flag2 ... --flagM`
 *
 * It supports default values for the last N postional arguments which allows
 * trailing arguments to be made optional. The above definition allows a
 * function with the following prototype to be called directly:
 *
 * `cmd_name(arg1, arg2, ... argN, flags)`
 *
 * Where `flags` is a struct defined elsewhere containing fields which will
 * match with the flags specified above.
 *
 * TODO:
 *  - Add support for short flags (ie. `-f <arg>`)
 *
 *  - Add a function to free memory. The parsers allocate memory but it's never
 *    freed. Not a big issue though as the program tends to execute one command
 *    and exit.
 */

typedef void * (optcmd_parser_t)(const char *argv);
typedef int (optcmd_cmd_t)(void *[], void *[]);

/*
 * The below data structures are used internally to specify commands along with
 * any arguments and flags. They could be filled out without any of the macro
 * magic further down, however internally the parser results are all cast to
 * (void *) which could result in weird errors if a function expecting a
 * argument of type char is called with an int.
 *
 * Using the macros should guard against these types of errors by ensuring type
 * mismatch errors will result in compiler warnings or errors (although sadly
 * these tend to be rather noisy).
 */
struct optcmd_flag {
	const char *name;
	optcmd_parser_t *arg;
};

struct optcmd_arg {
	optcmd_parser_t *parser;
	const char *def;
};

struct optcmd_cmd {
	const char *cmd;
	optcmd_cmd_t *cmdp;
	struct optcmd_arg args[OPTCMD_MAX_ARGS];
	struct optcmd_flag flags[OPTCMD_MAX_FLAGS];
};

/* Casts the given parser to a generic parser returning void * and taking a
 * char * argument. */
#define OPTCMD_PARSER_CAST(parser_) ((optcmd_parser_t *) parser_)

/* Returns the return type definition of the given parser */
#define OPTCMD_TYPEOF_PARSER(parser_, ...) typeof(*parser_(""))

/* Cast a postional argument to the right type */
#define OPTCMD_CAST_ARG(cnt_, parser_) *(OPTCMD_TYPEOF_PARSER parser_ *) args[cnt_]

/* Returns a positional argument struct */
#define _OPTCMD_ARG(parser_, default_) { .parser = OPTCMD_PARSER_CAST(parser_), .def = default_ }
#define OPTCMD_ARG(pos_) _OPTCMD_ARG pos_

/* Returns the type definition for a postional argument */
#define _OPTCMD_ARG_DEF(parser_, ...) OPTCMD_TYPEOF_PARSER(parser_)
#define OPTCMD_ARG_DEF(pos_) _OPTCMD_ARG_DEF pos_

/* Associate a specific flag name with a parser */
#define _OPTCMD_FLAG(flag_, field_, parser_, ...)		\
	{ .name = flag_, .arg = OPTCMD_PARSER_CAST(parser_) }
#define OPTCMD_FLAG(flag_) _OPTCMD_FLAG flag_

/* Cast parser output to a specific flag field */
#define _OPTCMD_CAST_FLAGS(flag_, field_, parser_, default_)	\
	BUILD_ASSERT(!check_types_match(flag.field_, OPTCMD_TYPEOF_PARSER(parser_)));			\
	flag.field_ = *flags ? *(OPTCMD_TYPEOF_PARSER(parser_) *) *flags : default_; \
	flags++;
#define OPTCMD_CAST_FLAGS(flags_) _OPTCMD_CAST_FLAGS flags_

/*
 * Defines a new command with arguments and flags.
 *  @cmd_name - name of command used on the command line
 *  @cmd_func - pointer to the function to call for this command
 *  @pos - list of positional arguments in the form ((parser, <default value>, "help text"), ...)
 *  @flag - name of flag struct to pass @cmd_func as the last parameter
 *  @flags - list of flags in the form (("--flag-name", <struct field name>, parser, <default value>, "help text"), ...)
 */
#define OPTCMD_DEFINE_CMD_WITH_FLAGS(cmd_name_, cmd_func_, pos_, flag_, flags_)	\
        int cmd_func_(CPPMAGIC_MAP(OPTCMD_ARG_DEF, CPPMAGIC_EVAL pos_), struct flag_); \
	int __##cmd_func_(void *args[], void *flags[])				\
	{								\
		struct flag_ flag;					\
		CPPMAGIC_JOIN(, CPPMAGIC_MAP(OPTCMD_CAST_FLAGS, CPPMAGIC_EVAL flags_)) \
		return cmd_func_(CPPMAGIC_MAP_CNT(OPTCMD_CAST_ARG, CPPMAGIC_EVAL pos_), flag); \
	}								\
									\
	struct optcmd_cmd optcmd_##cmd_name_ = {					\
		.cmd = #cmd_name_,					\
		.cmdp = __##cmd_func_,					\
		.args = { CPPMAGIC_MAP(OPTCMD_ARG, CPPMAGIC_EVAL pos_), {NULL} }, \
		.flags = { CPPMAGIC_MAP(OPTCMD_FLAG, CPPMAGIC_EVAL flags_), {NULL} }, \
	}

/*
 * Defines a new command taking only flags and no positional arguments.
 *  @cmd_name - name of command used on the command line
 *  @cmd_func - pointer to the function to call for this command
 *  @flag - name of flag struct to pass @cmd_func as the last parameter
 *  @flags - list of flags in the form (("--flag-name", <struct field name>, parser, <default value>, "help text"), ...)
 */
#define OPTCMD_DEFINE_CMD_ONLY_FLAGS(cmd_name_, cmd_func_, flag_, flags_) \
        int cmd_func_(struct flag_);					\
	int __##cmd_func_(void *args[], void *flags[])			\
	{								\
		struct flag_ flag;					\
		CPPMAGIC_JOIN(, CPPMAGIC_MAP(OPTCMD_CAST_FLAGS, CPPMAGIC_EVAL flags_)) \
		return cmd_func_(flag);					\
	}								\
									\
	struct optcmd_cmd optcmd_##cmd_name_ = {			\
		.cmd = #cmd_name_,					\
		.cmdp = __##cmd_func_,					\
		.flags = { CPPMAGIC_MAP(OPTCMD_FLAG, CPPMAGIC_EVAL flags_), {NULL} }, \
	}

/*
 * Defines a new command with arguments.
 *  @cmd_name - name of command used on the command line
 *  @cmd_func - pointer to the function to call for this command
 *  @pos - list of positional arguments in the form ((parser, <default value>, "help text"), ...)
 */
#define OPTCMD_DEFINE_CMD_WITH_ARGS(cmd_name_, cmd_func_, pos_)			\
        int cmd_func_(CPPMAGIC_MAP(OPTCMD_ARG_DEF, CPPMAGIC_EVAL pos_)); \
	int __##cmd_func_(void *args[], void *flags[])			\
	{								\
		return cmd_func_(CPPMAGIC_MAP_CNT(OPTCMD_CAST_ARG, CPPMAGIC_EVAL pos_)); \
	}								\
									\
	struct optcmd_cmd optcmd_##cmd_name_ = {					\
		.cmd = #cmd_name_,					\
		.cmdp = __##cmd_func_,					\
		.args = { CPPMAGIC_MAP(OPTCMD_ARG, CPPMAGIC_EVAL pos_), {NULL} }, \
	}

/*
 * Defines a new command taking no arguments or flags.
 *  @cmd_name - name of command used on the command line
 *  @cmd_func - pointer to the function to call for this command
 */
#define OPTCMD_DEFINE_CMD(cmd_name_, cmd_func_)		\
        int cmd_func_(void);				\
	int __##cmd_func_(void *args[], void *flags[])	\
	{						\
		return cmd_func_();			\
	}						\
							\
	struct optcmd_cmd optcmd_##cmd_name_ = {	\
		.cmd = #cmd_name_,			\
		.cmdp = __##cmd_func_,			\
	}

optcmd_cmd_t *optcmd_parse(struct optcmd_cmd *cmd, const char *argv[], int argc,
			   void **args[], void **flags[]);

#endif
