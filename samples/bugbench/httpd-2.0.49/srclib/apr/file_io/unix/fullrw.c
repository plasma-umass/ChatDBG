/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr_file_io.h"


APR_DECLARE(apr_status_t) apr_file_read_full(apr_file_t *thefile, void *buf,
                                             apr_size_t nbytes,
                                             apr_size_t *bytes_read)
{
    apr_status_t status;
    apr_size_t total_read = 0;

    do {
	apr_size_t amt = nbytes;

	status = apr_file_read(thefile, buf, &amt);
	buf = (char *)buf + amt;
        nbytes -= amt;
        total_read += amt;
    } while (status == APR_SUCCESS && nbytes > 0);

    if (bytes_read != NULL)
        *bytes_read = total_read;

    return status;
}

APR_DECLARE(apr_status_t) apr_file_write_full(apr_file_t *thefile,
                                              const void *buf,
                                              apr_size_t nbytes,
                                              apr_size_t *bytes_written)
{
    apr_status_t status;
    apr_size_t total_written = 0;

    do {
	apr_size_t amt = nbytes;

	status = apr_file_write(thefile, buf, &amt);
	buf = (char *)buf + amt;
        nbytes -= amt;
        total_written += amt;
    } while (status == APR_SUCCESS && nbytes > 0);

    if (bytes_written != NULL)
        *bytes_written = total_written;

    return status;
}
