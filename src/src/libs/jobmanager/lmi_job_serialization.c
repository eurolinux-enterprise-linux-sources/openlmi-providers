/*
 * Copyright (C) 2014 Red Hat, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Michal Minar <miminar@redhat.com>
 */

#include <json-glib/json-glib.h>
#include "openlmi.h"
#include "lmi_job.h"

gboolean lmi_job_dump(const LmiJob *job, GOutputStream *stream)
{
    gchar *data = NULL;
    gsize length;
    gchar *jobid = NULL;
    GError *gerror = NULL;
    g_assert(LMI_IS_JOB(job));
    g_assert(G_IS_OUTPUT_STREAM(stream));

    JOB_CRITICAL_BEGIN(job);
    if ((jobid = lmi_job_get_jobid(job)) == NULL) {
        lmi_error("Memory allocation failed");
        goto err;
    }

    if ((data = json_gobject_to_data(G_OBJECT(job), &length)) == NULL) {
        lmi_error("Failed to serialize job \"%s\" to json.", jobid);
        goto err;
    }
    if (!g_output_stream_write_all(stream, data, length, NULL, NULL, &gerror)) {
        lmi_error("Failed to dump job \"%s\" to output stream: %s",
                jobid, gerror->message);
        goto err;
    }
    JOB_CRITICAL_END(job);
    g_free(data);

    lmi_debug("Job \"%s\" dumped to output stream.", jobid);
    g_free(jobid);

    return TRUE;

err:
    g_free(data);
    g_free(jobid);
    JOB_CRITICAL_END(job);
    return FALSE;
}

gboolean lmi_job_dump_to_file(const LmiJob *job, const gchar *file_path)
{
    GFile *file;
    GFileOutputStream *stream;
    GError *gerror = NULL;
    gchar *jobid = NULL;
    gboolean res = false;
    g_assert(LMI_IS_JOB(job));

    JOB_CRITICAL_BEGIN(job);
    if ((file = g_file_new_for_commandline_arg(file_path)) == NULL)
        goto err;
    if ((jobid = lmi_job_get_jobid(job)) == NULL) {
        lmi_error("Memory allocation failed");
        goto err;
    }
    if ((stream = g_file_replace(file, NULL, FALSE, 0, NULL, &gerror)) == NULL) {
        lmi_error("Failed to write job file \"%s\" for job \"%s\": %s",
                file_path, jobid, gerror->message);
        goto err;
    }
    res = lmi_job_dump(job, G_OUTPUT_STREAM(stream));

    if (!g_output_stream_close(G_OUTPUT_STREAM(stream), NULL, &gerror)) {
        lmi_error("Failed to close an output stream of file \"%s\": %s",
                file_path, gerror->message);
        res = FALSE;
    }
    g_object_unref(stream);
err:
    g_clear_error(&gerror);
    g_free(jobid);
    g_object_unref(file);
    JOB_CRITICAL_END(job);
    return res;
}

LmiJob *lmi_job_load(GInputStream *stream, GType job_type)
{
    GObject *object = NULL;
    LmiJob *res = NULL;
    JsonParser *parser = NULL;
    JsonNode *root_node = NULL;
    GError *gerror = NULL;
    gchar *jobid = NULL;
    g_assert(G_IS_INPUT_STREAM(stream));
    g_assert(g_type_is_a(job_type, LMI_TYPE_JOB));

    if ((parser = json_parser_new()) == NULL) {
        lmi_error("Memory allocation failed");
        goto err;
    }
    if (!json_parser_load_from_stream(parser, stream, NULL, &gerror))
    {
        lmi_error("Failed to create parser from input stream.");
        goto err;
    }
    if ((root_node = json_parser_get_root(parser)) == NULL) {
        lmi_error("Failed to parse lmi job from input stream.");
        goto err;
    }
    if ((object = json_gobject_deserialize(job_type, root_node)) == NULL) {
        lmi_error("Failed to deserialize lmi job.");
        goto err;
    }
    g_object_unref(parser);
    res = LMI_JOB(object);
    if ((jobid = lmi_job_get_jobid(res)) == NULL) {
        lmi_error("Memory allocation failed");
        goto err;
    }

    lmi_debug("Successfully parsed and created lmi job \"%s\".", jobid);

    g_free(jobid);
    return res;

err:
    g_clear_object(&object);
    g_clear_object(&parser);
    return res;
}

LmiJob *lmi_job_load_from_file(const gchar *file_path, GType job_type)
{
    GFile *file = NULL;
    GFileInputStream *stream = NULL;
    GError *gerror = NULL;
    LmiJob *job = NULL;

    if ((file = g_file_new_for_commandline_arg(file_path)) == NULL)
        goto err;
    if ((stream = g_file_read(file, NULL, &gerror)) == NULL) {
        lmi_error("Failed to read job file \"%s\": %s",
                file_path, gerror->message);
        goto err;
    }
    job = lmi_job_load(G_INPUT_STREAM(stream), job_type);

    if (!g_input_stream_close(G_INPUT_STREAM(stream), NULL, &gerror)) {
        lmi_error("Failed to close input stream of file \"%s\": %s",
                file_path, gerror->message);
    }
    g_object_unref(stream);
err:
    g_clear_error(&gerror);
    g_clear_object(&file);
    return job;
}
