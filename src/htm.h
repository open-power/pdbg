/* Copyright 2017 IBM Corp.
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
#include <inttypes.h>
#include <stdio.h>

#include <target.h>

int run_htm_start(int optind, int argc, char *argv[]);
int run_htm_stop(int optind, int argc, char *argv[]);
int run_htm_status(int optind, int argc, char *argv[]);
int run_htm_reset(int optind, int argc, char *argv[]);
int run_htm_dump(int optind, int argc, char *argv[]);
int run_htm_trace(int optind, int argc, char *argv[]);
int run_htm_analyse(int optind, int argc, char *argv[]);
