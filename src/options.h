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

/* Default backend on this platform */
enum backend default_backend(void);

/* Print all possible backends on this platform */
void print_backends(FILE *stream);

/* The default (perhaps only) target for this backend */
const char *default_target(enum backend backend);

/* Print all possible targets on this platform */
void print_targets(FILE *stream);
